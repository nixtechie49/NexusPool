#include "pool_connection.hpp"
#include "daemon_connection.hpp"
#include "data_registry.hpp"
#include "LLP/block.h"
#include "LLP/ddos.hpp"
#include "packet.hpp"
#include "prime.hpp"
#include "base58.h"
#include "utils.hpp"

namespace nexuspool
{
Pool_connection::Pool_connection(network::Connection::Sptr&& connection, 
								std::weak_ptr<Daemon_connection> daemon_connection,
								std::weak_ptr<Data_registry> data_registry)
	: m_connection{std::move(connection)}
	, m_daemon_connection{std::move(daemon_connection)}
	, m_data_registry{std::move(data_registry)}
	, m_logger{ spdlog::get("logger") }
	, m_ddos{}
	, m_nxsaddress{""}
	, m_logged_in{false}
	, m_isDDOS{true}
	, m_connection_closed{false}
	, m_remote_address{""}
	, m_num_blocks_requested{0}
{
	auto registry = m_data_registry.lock();
	if (registry)
	{
		m_isDDOS = registry->get_use_ddos();
	}

}

network::Connection::Handler Pool_connection::init_connection_handler()
{
	return [self = shared_from_this()](network::Result::Code result, network::Shared_payload&& receive_buffer)
	{
		if (result == network::Result::connection_declined ||
			result == network::Result::connection_aborted ||
			result == network::Result::connection_error)
		{
			self->m_logger->error("Connection to {0} was not successful. Result: {1}", self->m_remote_address, result);
			self->m_connection = nullptr;		// close connection (socket etc)
			self->m_connection_closed = true;
		}
		else if (result == network::Result::connection_ok)
		{
			self->m_connection->remote_endpoint().address(self->m_remote_address);
			self->m_ddos = std::make_unique<LLP::DDOS_Filter>(30, self->m_remote_address);			
			self->m_logger->info("Connection to {} established", self->m_remote_address);
			
			// add to daemon connection to receive blocks
			auto daemon_connection = self->m_daemon_connection.lock();
			if(!daemon_connection)
			{
				// no daemon available? Can't do anything -> close connection
				self->m_connection = nullptr;		// close connection (socket etc)
				self->m_connection_closed = true;
			}
			else
			{
				daemon_connection->add_pool_connection(self);
			}
			
		}
		else
		{	// data received
			self->process_data(std::move(receive_buffer));
		}
	};
}

void Pool_connection::process_data(network::Shared_payload&& receive_buffer)
{
	if ((*receive_buffer).size() < 5)
	{
		m_logger->error("Received too small buffer from Connection {0}. Size: {1}", m_remote_address, (*receive_buffer).size());
		return;
	}

	Packet packet{ std::move(receive_buffer) };
	if (!packet.is_valid())
	{
		if (m_isDDOS)
			m_ddos->rSCORE += 10;

		m_logger->error("Received packet is invalid, Connection: {}", m_remote_address);
		return;
	}
	
	ddos_invalid_header(packet);	// ddos ban for invalid received packets

	//obtain ptr to data_registry
	auto data_registry = m_data_registry.lock();
	if (!data_registry)
	{
		// can't do anything useful wwithout data registry
		m_logger->error("Couldn't obtain a ptr to Data_registry");
		return;
	}	

	if (packet.m_header == Packet::LOGIN)
	{
		/** Multiply DDOS Score for Multiple Logins after Successful Login. **/
		if (m_logged_in)
		{
			if (m_isDDOS)
				m_ddos->rSCORE += 10;

			return;
		}

		m_nxsaddress = bytes2string(*packet.m_data);
		Core::NexusAddress cAddress(m_nxsaddress);

		m_stats.m_address = m_nxsaddress;
		//Core::STATSCOLLECTOR.IncConnectionCount(ADDRESS, GUID);

		if (!cAddress.IsValid())
		{
			m_logger->warn("Bad Account {}", m_nxsaddress);
			if (m_isDDOS)
				m_ddos->Ban(m_logger, "Invalid Nexus Address on Login");

			return;
		}

		//m_logger->info("Pool Login: {0}, ({1} connections)", ADDRESS, numConnections); Core::STATSCOLLECTOR.GetConnectionCount(ADDRESS));
		if (!data_registry->m_account_db->HasKey(m_nxsaddress))
		{
			LLD::Account cNewAccount(m_nxsaddress);
			data_registry->m_account_db->UpdateRecord(cNewAccount);

			m_logger->info("\n+++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
			m_logger->info("[ACCOUNT] New Account {}\n", m_nxsaddress);
			m_logger->info("\n+++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
		}


		if(data_registry->m_banned_users->is_account_banned(m_nxsaddress))
		{
			m_logger->warn("[ACCOUNT] Account: {} is banned\n", m_nxsaddress);

			// check to see whether this IP is in the banned IP list
			// if not then add it so that we can perma ban the IP
			if (!data_registry->m_banned_users->is_ip_address_banned(m_remote_address))
			{
				data_registry->m_banned_users->add_banned_ip_address(m_remote_address);
			}

			m_ddos->Ban(m_logger, "Account is Banned");

			m_logged_in = false;

		}
		else
		{
			m_logged_in = true;

			//PS
			// QUICK HACK these are google cloud and amazon AWS IP ranges which we should allow
			// so don't add CHECK to debug.log 
			if ((m_remote_address.find("104.") != 0) && (m_remote_address.find("130.") != 0)
				&& (m_remote_address.find("23.") != 0) && (m_remote_address.find("54.") != 0) && (m_remote_address.find("52.") != 0))
				m_logger->info("[ACCOUNT] Account: CHECK Address: {0} IP: {1} ", m_nxsaddress, m_remote_address); // this allows you to grep the debug.log for CHECK and eyeball the frequency and ip addresses      

		}
		return;
	}

	/** Reject Requests if not Logged In. **/
	if (!m_logged_in)
	{
		/** Amplify rScore for Requests before Logged in [Prevent DDOS this way]. **/
		if (m_isDDOS)
			m_ddos->rSCORE += 10;

		m_logger->warn("Not Logged In. Rejected Request.\n");

		return;
	}
	
	auto daemon_connection = m_daemon_connection.lock();
	if(!daemon_connection)
	{
		// no daemon available? Can't do anything -> close connection
		m_connection = nullptr;		// close connection (socket etc)
		m_connection_closed = true;
		return;
	}

	/** New block Process:
		Keeps a map of requested blocks for this connection.
		Clears map once new block is submitted successfully. **/
	if (packet.m_header == Packet::GET_BLOCK)
	{
		daemon_connection->request_block(++m_num_blocks_requested, m_remote_address);

		if (m_ddos)
			m_ddos->rSCORE += 1;
	}
	/** Send the Current Account balance to Pool Miner. **/
	else if(packet.m_header == Packet::GET_BALANCE)
	{
		Packet response; 
		response = response.get_packet(Packet::ACCOUNT_BALANCE);
		response.m_length = 8;
		response.m_data = std::make_shared<std::vector<uint8_t>>(uint2bytes64(data_registry->m_account_db->GetRecord(m_nxsaddress).nAccountBalance));
			
		m_logger->debug("Pool: Account {} --> Balance Requested", m_nxsaddress);
		m_connection->transmit(response.get_bytes());
	}		
	/** Send the Current Account balance to Pool Miner. **/
	else if(packet.m_header == Packet::GET_PAYOUT)
	{
		Packet response;
		response = response.get_packet(Packet::PENDING_PAYOUT);
		response.m_length = 8;
		
		uint64_t pending_payout = 0;
		Coinbase_mt const& coinbase_global = data_registry->get_coinbase();
		if(coinbase_global.find_address(m_nxsaddress))
		{
			pending_payout = coinbase_global.get_map_outputs(m_nxsaddress);
		}
			
		response.m_data = std::make_shared<std::vector<uint8_t>>(uint2bytes64(pending_payout));
			
		m_logger->debug("Pool: Account {} --> Payout Data Requested", m_nxsaddress);
		m_connection->transmit(response.get_bytes());
	}
			
	/** Handle a Ping from the Pool Miner. **/
	else if(packet.m_header == Packet::PING)
	{
		Packet response;
		response = response.get_packet(Packet::PING);
		m_connection->transmit(response.get_bytes());
	}
	else if(packet.m_header == Packet::SUBMIT_PPS)
	{ 
		// grab the current PPS from the miner
		double PPS = bytes2double(std::vector<uint8_t>(packet.m_data->begin(), packet.m_data->end() - 8));	
		double WPS = bytes2double(std::vector<uint8_t>(packet.m_data->begin() +8, packet.m_data->end()));	
		
	//	Core::STATSCOLLECTOR.UpdateConnectionData(m_nxsaddress, GUID, PPS, WPS);

		Packet response;
		response = response.get_packet(Packet::PING);
		m_connection->transmit(response.get_bytes());
	}
	/** Submit Share Process:
		Accepts a new block Merkle and nNonce for submit.
		This is to correlate where in memory the actual
		block is from MAP_BLOCKS. **/
	else if(packet.m_header == Packet::SUBMIT_SHARE)
	{
		uint1024 hashPrimeOrigin;
		hashPrimeOrigin.SetBytes(std::vector<uint8_t>(packet.m_data->begin(), packet.m_data->end() - 8));			
			
		/** Don't Accept a Share with no Correlated Block. **/
		if(!m_blocks_requested.count(hashPrimeOrigin))
		{
			Packet response;
			response = response.get_packet(Packet::REJECT);
			m_connection->transmit(response.get_bytes());

			m_logger->info("Pool: Block Not Found {}", hashPrimeOrigin.ToString().substr(0, 30));

			Packet response_block;
			response_block = response_block.get_packet(Packet::NEW_BLOCK);
			m_connection->transmit(response_block.get_bytes());
			
			if(m_isDDOS)
				m_ddos->rSCORE += 1;
			
			return;
		}
		
		/** Reject the Share if it is not of the Most Recent Block. **/
		if(m_blocks_requested[hashPrimeOrigin].nHeight != data_registry->m_best_height)
		{
			m_logger->info("Pool: Rejected Share - Share is Stale, submitted: {0} current: {1}", 
				m_blocks_requested[hashPrimeOrigin].nHeight, data_registry->m_best_height);
			
			Packet response;
			response = response.get_packet(Packet::STALE);
			m_connection->transmit(response.get_bytes());

			Packet response_block;
			response_block = response_block.get_packet(Packet::NEW_BLOCK);
			m_connection->transmit(response_block.get_bytes());
			
			/** Be Lenient on Stale Shares [But Still amplify score above normal 1 per request.] **/
			if(m_isDDOS)
				m_ddos->rSCORE += 2;
				
			return;
		}			
			
		/** Get the Prime Number Found. **/
		uint64_t nNonce = bytes2uint64(std::vector<uint8_t>(packet.m_data->end() - 8, packet.m_data->end()));
		uint1024 hashPrime = hashPrimeOrigin + nNonce;	

		size_t primes_count = 0;
		{
			std::lock_guard<std::mutex> lk(m_primes_mutex);
			primes_count = m_primes.count(hashPrime);
		}
	
		/** Check the Share that was just submitted. **/
		if(primes_count != 0)
		{
			m_logger->info("Pool: Duplicate Share {}", hashPrime.ToString().substr(0, 30));
			Packet response;
			response = response.get_packet(Packet::REJECT);
			m_connection->transmit(response.get_bytes());
				
			/** Give a Heavy Score for Duplicates. [To Amplify the already existing ++ per Request.] **/
			if(m_isDDOS)
				m_ddos->rSCORE += 5;
				
			return;
		}	
		
		/** Check the Difficulty of the Share. **/
		double nDifficulty = gmp_verification(hashPrime);
		
		if(set_bits(nDifficulty) >= data_registry->get_min_share())
		{
			{
				std::lock_guard<std::mutex> lk(m_primes_mutex);
				m_primes.insert(hashPrime);		// unique entry because of set		
			}
	
			// statistics, found chains
			//if(nDifficulty < 8)
			//{
			//	Core::nDifficultyShares[(unsigned int) floor(nDifficulty - 3)]++;
			//}
			
			LLD::Account cAccount = data_registry->m_account_db->GetRecord(m_nxsaddress);
			uint64_t weight = pow(25.0, nDifficulty - 3.0);
			cAccount.nRoundShares += weight;
			data_registry->m_account_db->UpdateRecord(cAccount);
			
			m_logger->debug("Pool: Share Accepted of Difficulty {0} | Weight {1}", nDifficulty, weight);
			
			/** If the share is above the difficulty, give block finder bonus and add to Submit Stack. **/
			if(nDifficulty >= get_difficulty(m_blocks_requested[hashPrimeOrigin].nBits))
			{
				m_blocks_requested[hashPrimeOrigin].nNonce = nNonce;
				if(daemon_connection->submit_block(m_blocks_requested[hashPrimeOrigin], m_remote_address))
				{					
					Packet response;
					response = response.get_packet(Packet::BLOCK);
					m_connection->transmit(response.get_bytes());
				}
			}
			else
			{
				Packet response;
				response = response.get_packet(Packet::ACCEPT);
				m_connection->transmit(response.get_bytes());
			}				
		}
		else
		{
			m_logger->info("Pool: Share Below Difficulty {}", nDifficulty);
			
			/** Give Heavy Score for Below Difficulty Shares. Would require at least 4 per Second to fulfill a score of 20. **/
			if(m_isDDOS)
				m_ddos->rSCORE += 5;
			
			Packet response;
			response = response.get_packet(Packet::REJECT);
			m_connection->transmit(response.get_bytes());
		}
	}
}

void Pool_connection::new_round()
{
	m_primes.clear();
	m_blocks_requested.clear();
}

void Pool_connection::ddos_invalid_header(Packet const& packet)
{
	if(m_isDDOS)
	{
		if(packet.m_length > 136)
			m_ddos->Ban(m_logger, "Max Packet Size Exceeded");
		
		if(packet.m_header == Packet::SUBMIT_SHARE && packet.m_length != 136)
			m_ddos->Ban(m_logger, "Packet of Share not 136 Bytes");
			
		if(packet.m_header == Packet::LOGIN && packet.m_length > 55)
			m_ddos->Ban(m_logger, "Login Message too Large");

		if(packet.m_header == Packet::SUBMIT_PPS && packet.m_length != 16)
			m_ddos->Ban(m_logger, "Submit PPS Packet not 16 Bytes");
			
		if(packet.m_header == Packet::BLOCK_DATA)
			m_ddos->Ban(m_logger, "Received Block Data Header. Invalid as Request");
			
		if(packet.m_header == Packet::ACCOUNT_BALANCE)
			m_ddos->Ban(m_logger, "Received Account Balance Header. Inavlid as Request");
			
		if(packet.m_header == Packet::PENDING_PAYOUT)
			m_ddos->Ban(m_logger, "Recieved Pending Payout Header. Invald as Request");
			
		if(packet.m_header == Packet::ACCEPT)
			m_ddos->Ban(m_logger, "Received Share Accepted Header. Invalid as Request");
		
		if(packet.m_header == Packet::REJECT)
			m_ddos->Ban(m_logger, "Received Share Rejected Header. Invalid as Request.");
			
		if(packet.m_header == Packet::BLOCK)
			m_ddos->Ban(m_logger, "Received Block Header. Invalid as Request.");
			
		if(packet.m_header == Packet::STALE)
			m_ddos->Ban(m_logger, "Recieved Stale Share Header. Invalid as Request");
	}
}

/** Convert the Header of a Block into a Byte Stream for Reading and Writing Across Sockets. **/
network::Shared_payload Pool_connection::serialize_block(LLP::CBlock* block)
{
	auto data_registry = m_data_registry.lock();
	if (!data_registry)
	{
		return network::Shared_payload{};
	}

	std::vector<uint8_t> hash = block->GetHash().GetBytes();
	std::vector<uint8_t> minimum = uint2bytes(data_registry->get_min_share());
	std::vector<uint8_t> difficulty = uint2bytes(block->nBits);
	std::vector<uint8_t> height = uint2bytes(block->nHeight);

	std::vector<uint8_t> data;
	data.insert(data.end(), hash.begin(), hash.end());
	data.insert(data.end(), minimum.begin(), minimum.end());
	data.insert(data.end(), difficulty.begin(), difficulty.end());
	data.insert(data.end(), height.begin(), height.end());

	return std::make_shared<std::vector<uint8_t>>(data);
}

}
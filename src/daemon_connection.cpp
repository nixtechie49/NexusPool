#include "daemon_connection.hpp"
#include "pool_connection.hpp"
#include "daemon_packet.hpp"
#include "data_registry.hpp"
#include "LLP/block.h"
#include "LLP/ddos.hpp"
#include "packet.hpp"
#include "config.hpp"
#include "base58.h"
#include "utils.hpp"

#include <spdlog/spdlog.h>

namespace nexuspool
{
	Daemon_connection::Daemon_connection(network::Socket::Sptr&& socket, 
										std::weak_ptr<Data_registry> data_registry, 
										chrono::Timer_factory::Sptr timer_factory)
		: m_socket{std::move(socket)}
		, m_data_registry{std::move(data_registry)}
		, m_logger{spdlog::get("logger")}
		, m_timer_factory{std::move(timer_factory)}
		, m_coinbase_pending{false}
	{
		m_maintenance_timer = m_timer_factory->create_timer();
		m_block_timer = m_timer_factory->create_timer();
		m_orphan_check_timer = m_timer_factory->create_timer();
	}

	Daemon_connection::~Daemon_connection()
	{
	}

	bool Daemon_connection::connect(network::Endpoint remote_wallet)
	{
		// async
		auto connection = m_socket->connect(std::move(remote_wallet), [self = shared_from_this()](network::Result::Code result, network::Shared_payload&& receive_buffer)
		{
			if (result == network::Result::connection_declined ||
				result == network::Result::connection_aborted ||
				result == network::Result::connection_error)
			{
				self->m_logger->error("Connection to wallet not sucessful. Result: {}", result);
				self->m_connection = nullptr;		// close connection (socket etc)
				// retry connect
			}
			else if (result == network::Result::connection_ok)
			{
				self->m_logger->info("Connection to wallet established");
				self->m_maintenance_timer->start(chrono::Seconds(60), self->maintenance_timer_handler());
				self->m_block_timer->start(chrono::Milliseconds(50), self->block_timer_handler());
				self->m_orphan_check_timer->start(chrono::Seconds(20), self->orphan_check_timer_handler());				

				// set channel
				Daemon_packet daemon_packet;
				Packet packet = daemon_packet.set_channel(1);	// prime channel
				self->m_connection->transmit(packet.get_bytes());
			}
			else
			{	// data received
				self->process_data(std::move(receive_buffer));
			}

		});

		if(!connection)
		{
			return false;
		}

		m_connection = std::move(connection);
		return true;
	}
	
	chrono::Timer::Handler Daemon_connection::maintenance_timer_handler()
	{
		return [self = shared_from_this()](bool canceled) 
		{
			if(canceled)	// don't do anything if the timer has been canceled
			{
				return;
			}
			
			self->cleanup_expired_pool_connections();		
			// start the timer again
			self->m_maintenance_timer->start(chrono::Seconds(60), self->maintenance_timer_handler());
		};
	}

	chrono::Timer::Handler Daemon_connection::block_timer_handler()
	{
		return[self = shared_from_this()](bool canceled)
		{
			if (canceled)	// don't do anything if the timer has been canceled
			{
				return;
			}

			//obtain ptr to data_registry
			auto data_registry = self->m_data_registry.lock();
			if (!data_registry)
			{
				// can't do anything useful wwithout data registry -> timer will not be called again
				self->m_logger->error("Couldn't obtain a ptr to Data_registry");
				return;
			}


			if (self->m_coinbase_pending)
			{
				// new round is pending, no use in submitting old blocks, or getting expired blocks
				self->m_block_timer->start(chrono::Milliseconds(50), self->block_timer_handler());
				return;
			}

			/** Submit a block from Pool Connections if there is one available. **/
			{
				std::lock_guard<std::mutex> lk(self->m_pool_connections_mutex);

				for (auto& pool_data : self->m_pool_connections)
				{
					auto pool_connection = pool_data.m_connection.lock();
					if (!pool_connection)
					{
						continue;
					}

					if (pool_data.m_submit_block)
					{
						//		Core::fSubmittingBlock = true;
						self->m_hash_submit_block = pool_data.m_submit_block->GetHash();

						/** Submit the Block to Network if it is Valid. **/
						if (pool_data.m_submit_block->nHeight == data_registry->m_best_height)
						{
							Daemon_packet packet;
							Packet response = packet.submit_block(pool_data.m_submit_block->hashMerkleRoot, pool_data.m_submit_block->nNonce);
							self->m_connection->transmit(response.get_bytes());

							{
								std::lock_guard<std::mutex> lk(self->m_last_blockfinder_mutex);
								self->m_last_blockfinder = pool_connection->get_nxs_address();
							}

							self->m_logger->info("Daemon: Submitting Block for Address {0} on Connection: {1}",
								pool_connection->get_nxs_address(), pool_connection->get_remote_address());
						}
						else
						{
							self->m_logger->info("Daemon: Stale Block Submitted on Connection: {}", pool_connection->get_remote_address());
						}

						/** Reset the Submission Block Pointer. **/
						pool_data.m_submit_block = nullptr;
						break;
					}
				}
				/** Get Blocks if Requested From Pool Connection. **/
				for (auto& pool_data : self->m_pool_connections)
				{
					while (pool_data.m_blocks_requested > 0)
					{
						pool_data.m_blocks_requested--;
						pool_data.m_blocks_waiting++;

						Daemon_packet daemon_packet;
						Packet response;
						response = daemon_packet.get_block();
						self->m_connection->transmit(response.get_bytes());

						self->m_logger->debug("Daemon: Requesting Block from wallet. Connection has {} block requests left", pool_data.m_blocks_requested);
					}
				}
			}
			// start the timer again
			self->m_block_timer->start(chrono::Milliseconds(50), self->block_timer_handler());
		};
	}

	chrono::Timer::Handler Daemon_connection::orphan_check_timer_handler()
	{
		return[self = shared_from_this()](bool canceled)
		{
			if (canceled)	// don't do anything if the timer has been canceled
			{
				return;
			}

			//obtain ptr to data_registry
			auto data_registry = self->m_data_registry.lock();
			if (!data_registry)
			{
				// can't do anything useful wwithout data registry -> timer will not be called again
				self->m_logger->error("Couldn't obtain a ptr to Data_registry");
				return;
			}

			// Check last 5 blocks every 20 seconds. **/
			std::vector<uint1024> vKeys = data_registry->m_block_db->GetKeys();
			self->m_logger->info("Checking last 5 blocks...\n");
			for (size_t nIndex = 0; nIndex < vKeys.size(); nIndex++)
			{
				LLD::Block cBlock = data_registry->m_block_db->GetRecord(vKeys[nIndex]);
				if (cBlock.nRound >= data_registry->m_current_round - 5 && cBlock.nCoinbaseValue > 0)
				{
					Daemon_packet daemon_packet;
					Packet packet = daemon_packet.check_block(vKeys[nIndex]);
					self->m_connection->transmit(packet.get_bytes());
				}					
			}

			// start the timer again
			self->m_orphan_check_timer->start(chrono::Seconds(20), self->orphan_check_timer_handler());
		};
	}

	void Daemon_connection::add_pool_connection(std::weak_ptr<Pool_connection> connection)
	{
		std::lock_guard<std::mutex> lk(m_pool_connections_mutex);
		m_pool_connections.push_back(Pool_connection_data(std::move(connection), 0, 0));
	}
	
	void Daemon_connection::cleanup_expired_pool_connections()
	{
		std::lock_guard<std::mutex> lk(m_pool_connections_mutex);

		// get rid of expired connections
		for(auto iter = m_pool_connections.begin(); iter != m_pool_connections.end(); ++iter)
		{
			if (iter->m_connection.expired())
			{ 
				iter = m_pool_connections.erase(iter); 
				continue; 
			}
		}
	}
	
	Pool_connection_data& Daemon_connection::find_pool_connection(std::string const& ip_address)
	{
		std::lock_guard<std::mutex> lk(m_pool_connections_mutex);

		auto find_iter = std::find_if(m_pool_connections.begin(), m_pool_connections.end(),
			[&ip_address](const Pool_connection_data& pool_connection_data) 
		{ 
			auto pool_connection = pool_connection_data.m_connection.lock();
			if (!pool_connection)
			{
				return false;
			}
			
			return pool_connection->get_remote_address() == ip_address;
		});

		if (find_iter == m_pool_connections.end())
		{
			// this shouldn't happen
			m_logger->error("Daemon: request_block didn't find ip: {} in stored pool connections.", ip_address);
		}
		
		return (*find_iter);
	}

	void Daemon_connection::request_block(uint16_t blocks_requested, std::string const& ip_address)
	{
		std::lock_guard<std::mutex> lk(m_pool_connections_mutex);

		Pool_connection_data& pool_connection_data = find_pool_connection(ip_address);
		pool_connection_data.m_blocks_requested = blocks_requested;
	}
	
	bool Daemon_connection::submit_block(LLP::CBlock const& block, std::string const& ip_address)
	{
		std::lock_guard<std::mutex> lk(m_pool_connections_mutex);

		Pool_connection_data& pool_connection_data = find_pool_connection(ip_address);
		if(!pool_connection_data.m_submit_block)
		{
			pool_connection_data.m_submit_block = std::make_unique<LLP::CBlock>(block);
			return true;
		}
		
		return false;			
	}

	void Daemon_connection::process_data(network::Shared_payload&& receive_buffer)
	{
		if ((*receive_buffer).size() < 5)
		{
			m_logger->error("Received too small buffer from wallet. Size: {}", (*receive_buffer).size());
			return;
		}

		Packet packet{ std::move(receive_buffer) };
		if (!packet.is_valid())
		{
			// log invalid packet
			m_logger->error("Received packet is invalid");
			return;
		}
		
		//obtain ptr to data_registry
		auto data_registry = m_data_registry.lock();
		if (!data_registry)
		{
			// can't do anything useful wwithout data registry
			m_logger->error("Couldn't obtain a ptr to Data_registry");
			return;
		}

		/** Update the Global Round Height if there is a new Block. **/
		if(packet.m_header == Daemon_packet::BLOCK_HEIGHT)
		{
			uint32_t height = bytes2uint(*packet.m_data);
			m_logger->debug("Daemon: Height Received {}", height);

			if (height > data_registry->m_best_height)
			{
				m_logger->info("Daemon: Nexus Network: New Block [Height] {}", height);
				m_coinbase_pending = true;

			}

			data_registry->m_best_height = height;
		}
		/** Response from Mining LLP that there is a new block. **/
		else if(packet.m_header == Daemon_packet::NEW_ROUND)
		{
			m_logger->info("Daemon: Nexus Network: New Block [Round] {}", data_registry->m_best_height);
			m_coinbase_pending = true;
		}
		/** After the Coinbase Transaction is Set on Master Thread, Check the Blocks for Orphans. **/
		else if (packet.m_header == Daemon_packet::COINBASE_SET)
		{
			m_logger->info("Daemon: Coinbase Transaction Set");
			m_coinbase_pending = false;

			/** Commit Shares to Database on New Block. **/
			data_registry->m_account_db->WriteToDisk();
		}
		/** If the Coinbase Tx Fails to Update, get data again and attempt to rebuild it. **/
		else if (packet.m_header == Daemon_packet::COINBASE_FAIL)
		{
			m_logger->error("Daemon: Coinbase Transaction Not Set. Retrying...");

			m_coinbase_pending = true;
		}
		/** Update the Current Round Rewards. **/
		else if(packet.m_header == Daemon_packet::BLOCK_REWARD)
		{
			uint64_t round_reward = bytes2uint64(*packet.m_data);
			m_logger->info("Daemon: Received Reward of {}", round_reward / 1000000.0);

			if (data_registry->m_current_round > 1 && m_coinbase_pending)
			{
				Coinbase_mt& coinbase = data_registry->get_coinbase();
				coinbase.get_pool_fee() = (round_reward * data_registry->get_config().get_pool_fee());
				coinbase.reset(round_reward - coinbase.get_pool_fee());


				std::vector<std::string> vAccounts = data_registry->get_sorted_accounts();
				for (size_t nIndex = 0; nIndex < vAccounts.size(); nIndex++)
				{
					/** Read the Account Record from the Database. **/
					LLD::Account cAccount = data_registry->m_account_db->GetRecord(vAccounts[nIndex]);

					/** Only add a Payout if Above the 0 NIRO Threshold. **/
					if (cAccount.nAccountBalance > 0)
					{
						uint64_t nValue = coinbase.add_transaction(vAccounts[nIndex], cAccount.nAccountBalance);
						if (nValue < 0)
						{
							coinbase.add_transaction(vAccounts[nIndex], cAccount.nAccountBalance + nValue);
							break;
						}
					}
				}

				/** Add NIRO Block Finder Bonus [On top of Larger Weight] on Rounds where Coinbase is Incomplete. **/
				if (!coinbase.is_complete())
				{
					uint64_t nBonus = coinbase.get_remainder();
					{
						std::lock_guard<std::mutex> lk(m_last_blockfinder_mutex);
						coinbase.add_transaction(m_last_blockfinder, nBonus);
					}
					
				}

				coinbase.print(m_logger);
				Daemon_packet daemon_packet;
				Packet packet = daemon_packet.set_coinbase(coinbase.serialize());
				m_connection->transmit(packet.get_bytes());
			}
			else
			{
				m_coinbase_pending = false;
			}
			m_logger->info("DAEMON: Round {0} Block {1} Rewards {2} NXS", data_registry->m_current_round, data_registry->m_best_height, round_reward / 1000000.0);
		}

		// OPRHAN check
		else if(packet.m_header == Daemon_packet::ORPHAN_BLOCK)
		{
			uint1024 hashOrphan;
			hashOrphan.SetBytes(*packet.m_data);

			m_logger->info("Daemon: Block {0} - Round {1} Has Been Orphaned\n", hashOrphan.ToString().substr(0, 20), data_registry->m_block_db->GetRecord(hashOrphan).nRound);
			data_registry->get_payout_manager().refund_payouts(data_registry, hashOrphan);

			m_coinbase_pending = true;
		}
		else if(packet.m_header == Daemon_packet::GOOD_BLOCK)
		{
			uint1024 hashBlock;
			hashBlock.SetBytes(*packet.m_data);

			m_logger->info("Daemon: Block {0} - Round: {1} Is in Main Chain\n", hashBlock.ToString().substr(0, 20), data_registry->m_block_db->GetRecord(hashBlock).nRound);
		}

		// BLOCK stuff
		///** If a Block was Accepted by the Network, start a new Round. **/
		else if (packet.m_header == Daemon_packet::GOOD)
		{
			m_logger->info("Daemon: Block {} accepted.", m_hash_submit_block.ToString().substr(0, 20));
			Core::NewRound();

		//	Core::fSubmittingBlock = false;
		}
		else if (packet.m_header == Daemon_packet::FAIL)
		{
			m_logger->info("Daemon: Block rejected");
			//Core::fSubmittingBlock = false;
		}
	//	else if (packet.m_header == Daemon_packet::COINBASE_FAIL)
	//	{
		//	m_logger->error("Daemon: Coinbase Transaction Not Set");

		//	Core::fNewRound = false;
		//	Core::fSubmittingBlock = false;
		//	Core::fCoinbasePending = true;

		//	fNewBlock = false;
		//	fCoinbasePending = false;
		//}
		///** Add a Block to the Stack if it is received by the Daemon Connection. **/
		else if (packet.m_header == Daemon_packet::BLOCK_DATA)
		{
			LLP::CBlock block = deserialize_block(packet.m_data);
			if (block.nHeight == data_registry->m_best_height)
			{
				{ 
					std::lock_guard<std::mutex> lk(m_pool_connections_mutex);
					
					for(auto& pool_data : m_pool_connections)
					{
						auto pool_connection = pool_data.m_connection.lock();
						if (!pool_connection)
						{
							continue;	// the pool_connection doesn't exist anymore. Give the block data another connection
						}
						
						if(pool_data.m_blocks_waiting > 0)
						{
							pool_data.m_blocks_waiting--;
							m_logger->info("Daemon: Block Received Height = {0} Assigned to connection {1}", 
											block.nHeight, pool_connection->get_remote_address());
											
								//				CONNECTIONS[nIndex]->AddBlock(BLOCK);
							return;
						}
					}
				}
			}
			else
			{
				m_logger->info("Daemon: Block Obsolete Height = {}, Skipping over.", block.nHeight);
			}
		}
		
		///** Don't Request Anything if Waiting for New Round to Reset. **/
		if (m_coinbase_pending || Core::fSubmittingBlock || Core::fNewRound || fCoinbasePending)
			return;


		///** Reset The Daemon if there is a New Block. **/
		//if (fNewBlock)
		//{
		//	fNewBlock = false;

		//	NewBlock();
		//	//printf("[DAEMON] Niro Network: New Block | Reset Daemon Handle %u\n", ID);

		//	/** Set the new Coinbase Transaction. **/
		//	if (Core::nCurrentRound > 1)
		//	{
		//		fCoinbasePending = true;
		//		CLIENT->SetCoinbase();

		//		continue;
		//	}
		//}

		//}
		//}
	}


	/** Convert the Header of a Block into a Byte Stream for Reading and Writing Across Sockets. **/
	LLP::CBlock Daemon_connection::deserialize_block(network::Shared_payload data)
	{
		LLP::CBlock block;
		block.nVersion = bytes2uint(std::vector<uint8_t>(data->begin(), data->begin() + 4));

		block.hashPrevBlock.SetBytes(std::vector<uint8_t>(data->begin() + 4, data->begin() + 132));
		block.hashMerkleRoot.SetBytes(std::vector<uint8_t>(data->begin() + 132, data->end() - 20));

		block.nChannel = bytes2uint(std::vector<uint8_t>(data->end() - 20, data->end() - 16));
		block.nHeight = bytes2uint(std::vector<uint8_t>(data->end() - 16, data->end() - 12));
		block.nBits = bytes2uint(std::vector<uint8_t>(data->end() - 12, data->end() - 8));
		block.nNonce = bytes2uint64(std::vector<uint8_t>(data->end() - 8, data->end()));

		return block;
	}
}
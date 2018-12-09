#include "pool_connection.hpp"
#include "LLP/block.h"
#include "LLP/ddos.hpp"
#include "packet.hpp"
#include "base58.h"
#include "utils.hpp"

namespace nexuspool
{
Pool_connection::Pool_connection(network::Connection::Sptr&& connection)
	: m_connection{std::move(connection)}
	, m_ddos{}
	, m_nxsaddress{""}
	, m_logged_in{false}
	, m_isDDOS{true}
{}

//void Pool_connection::connection_handler(network::Result::Code result, network::Shared_payload&& receive_buffer)
//{
//
//}

void Pool_connection::process_data(network::Shared_payload&& receive_buffer)
{
	if ((*receive_buffer).size() < 5)
	{
		// log error
		return;
	}

	Packet packet{ std::move(receive_buffer) };
	if (!packet.is_valid())
	{
		// log invalid packet
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

		Core::STATSCOLLECTOR.IncConnectionCount(ADDRESS, GUID);

		if (!cAddress.IsValid())
		{
			printf("[THREAD] Pool LLP: Bad Account %s\n", ADDRESS.c_str());
			if (m_isDDOS)
				m_ddos->Ban("Invalid Nexus Address on Login");

			return;
		}

		std::string ip_address = GetIPAddress();

		printf("[THREAD] Pool Login: %s\t IP:%s\t (%d connections)\n", ADDRESS.c_str(), ip_address.c_str(), Core::STATSCOLLECTOR.GetConnectionCount(ADDRESS));
		if (!Core::AccountDB.HasKey(ADDRESS))
		{
			LLD::Account cNewAccount(ADDRESS);
			Core::AccountDB.UpdateRecord(cNewAccount);

			printf("\n+++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
			printf("[ACCOUNT] New Account %s\n", ADDRESS.c_str());
			printf("\n+++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
		}


		if (IsBannedAccount(ADDRESS))
		{
			printf("[ACCOUNT] Account: %s is banned\n", ADDRESS.c_str());

			// check to see whether this IP is in the banned IP list
			// if not then add it so that we can perma ban the IP
			if (!IsBannedIPAddress(ip_address))
			{
				SaveBannedIPAddress(ip_address);
			}

			m_ddos->Ban("Account is Banned");

			m_logged_in = false;

		}
		else
		{
			m_logged_in = true;

			//PS
			// QUICK HACK these are google cloud and amazon AWS IP ranges which we should allow
			// so don't add CHECK to debug.log 
			if (!boost::starts_with(ip_address, "104.") && !boost::starts_with(ip_address, "130.")
				&& !boost::starts_with(ip_address, "23.") && !boost::starts_with(ip_address, "54.") && !boost::starts_with(ip_address, "52."))
				printf("[ACCOUNT] Account: CHECK Address: %s  IP: %s\n", ADDRESS.c_str(), ip_address.c_str()); // this allows you to grep the debug.log for CHECK and eyeball the frequency and ip addresses

		}
		return;
	}


	/** Reject Requests if not Logged In. **/
	if (!m_logged_in)
	{
		/** Amplify rScore for Requests before Logged in [Prevent DDOS this way]. **/
		if (m_isDDOS)
			m_ddos->rSCORE += 10;

		printf("[THREAD] Pool LLP: Not Logged In. Rejected Request.\n");

		return;
	}

	/** New block Process:
		Keeps a map of requested blocks for this connection.
		Clears map once new block is submitted successfully. **/
	if (packet.m_header == Packet::GET_BLOCK)
	{
		LOCK(BLOCK_MUTEX);
		nBlockRequests++;

		if (m_ddos)
			m_ddos->rSCORE += 1;

		return;
	}


}

/** Convert the Header of a Block into a Byte Stream for Reading and Writing Across Sockets. **/
network::Shared_payload Pool_connection::serialize_block(LLP::CBlock* block)
{
	std::vector<uint8_t> hash = block->GetHash().GetBytes();
	std::vector<uint8_t> minimum = uint2bytes(Core::nMinimumShare);
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
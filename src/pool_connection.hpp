#ifndef NEXUSPOOL_POOL_CONNECTION_HPP
#define NEXUSPOOL_POOL_CONNECTION_HPP

#include "network/connection.hpp"
#include "LLP/ddos.hpp"
#include <spdlog.h>

#include <string>
#include <memory>

namespace LLP 
{ 
	class CBlock; 
}

namespace nexuspool
{
	// shared by server and daemon connection
	class Pool_connection : public std::enable_shared_from_this<Pool_connection>
	{
	public:

		using Uptr = std::unique_ptr<Pool_connection>;

		Pool_connection(network::Connection::Sptr&& connection, uint32_t min_share);

		// Called async when connection got established or data is received
		network::Connection::Handler connection_handler = [this](network::Result::Code result, network::Shared_payload&& receive_buffer) 
		{ 
			if (result == network::Result::connection_declined ||
				result == network::Result::connection_aborted ||
				result == network::Result::connection_error)
			{
				m_logger->error("Connection to {0} was not successful. Result: {1}", m_remote_address, result);
				m_connection = nullptr;		// close connection (socket etc)
			}
			else if (result == network::Result::connection_ok)
			{				
				m_connection->remote_endpoint().address(m_remote_address);
				m_ddos = std::make_unique<LLP::DDOS_Filter>(30, m_remote_address);
				m_logger->info("Connection to {} established", m_remote_address);
				// add to daemon connection to receive blocks
			}
			else
			{	// data received
				process_data(std::move(receive_buffer));
			}

		};


	private:

		void process_data(network::Shared_payload&& receive_buffer);
		network::Shared_payload serialize_block(LLP::CBlock* block);

		network::Connection::Sptr m_connection;		// network connection
		std::shared_ptr<spdlog::logger> m_logger;
		std::unique_ptr<LLP::DDOS_Filter> m_ddos;
		std::string m_nxsaddress;
		bool m_logged_in;
		bool m_isDDOS;
		uint32_t m_min_share;
		std::string m_remote_address;
	};

}


#endif // NEXUSPOOL_POOL_CONNECTION_HPP
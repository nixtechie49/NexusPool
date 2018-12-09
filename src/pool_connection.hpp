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
	class DDOS_Filter; 
}

namespace nexuspool
{

	class Pool_connection
	{
	public:

		using Uptr = std::unique_ptr<Pool_connection>;

		Pool_connection(network::Connection::Sptr&& connection);

		// Called async when connection got established or data is received
		network::Connection::Handler connection_handler = [this](network::Result::Code result, network::Shared_payload&& receive_buffer) 
		{ 
			if (result == network::Result::connection_declined ||
				result == network::Result::connection_aborted ||
				result == network::Result::connection_error)
			{
				//log
				m_connection = nullptr;		// close connection (socket etc)
			}
			else if (result == network::Result::connection_ok)
			{
				// log
				std::string remote_address;
				m_connection->remote_endpoint().address(remote_address);
				m_ddos = std::make_unique<LLP::DDOS_Filter>(30, std::move(remote_address));
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
		std::unique_ptr<LLP::DDOS_Filter> m_ddos;
		std::string m_nxsaddress;
		bool m_logged_in;
		bool m_isDDOS;
	};

}


#endif // NEXUSPOOL_POOL_CONNECTION_HPP
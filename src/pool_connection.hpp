#ifndef NEXUSPOOL_POOL_CONNECTION_HPP
#define NEXUSPOOL_POOL_CONNECTION_HPP

#include "network/connection.hpp"
#include "LLP/ddos.hpp"
#include "stats_collector.hpp"
#include <spdlog/spdlog.h>

#include <string>
#include <memory>

namespace LLP 
{ 
	class CBlock; 
}

namespace nexuspool
{
	class Data_registry;

	// shared by server and daemon connection
	class Pool_connection : public std::enable_shared_from_this<Pool_connection>
	{
	public:

		Pool_connection(network::Connection::Sptr&& connection, std::shared_ptr<Data_registry> data_registry);

		// The returned handler is called async when connection got established or data is received
		network::Connection::Handler init_connection_handler();


	private:

		void process_data(network::Shared_payload&& receive_buffer);
		// Serialize a NXS block to byte stream
		network::Shared_payload serialize_block(LLP::CBlock* block);

		network::Connection::Sptr m_connection;		// network connection
		std::weak_ptr<Data_registry> m_data_registry;	// global data access
		std::shared_ptr<spdlog::logger> m_logger;
		std::unique_ptr<LLP::DDOS_Filter> m_ddos;
		std::string m_nxsaddress;
		bool m_logged_in;
		bool m_isDDOS;	// use ddos or not
		std::string m_remote_address;	// remote ip address
		Stats_connection m_stats;	// statistics per connection. Will be consumed by Stats_collector
	};

}


#endif // NEXUSPOOL_POOL_CONNECTION_HPP
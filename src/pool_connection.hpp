#ifndef NEXUSPOOL_POOL_CONNECTION_HPP
#define NEXUSPOOL_POOL_CONNECTION_HPP

#include "network/connection.hpp"
#include "LLP/ddos.hpp"
#include "stats_collector.hpp"
#include <spdlog/spdlog.h>

#include <string>
#include <memory>
#include <atomic>

namespace LLP 
{ 
	class CBlock; 
}

namespace nexuspool
{
	class Data_registry;
	class Daemon_connection;
	class Packet;

	// shared by server and daemon connection
	class Pool_connection : public std::enable_shared_from_this<Pool_connection>
	{
	public:

		Pool_connection(network::Connection::Sptr&& connection, 
						std::weak_ptr<Daemon_connection> daemon_connection,
						std::weak_ptr<Data_registry> data_registry);

		// The returned handler is called async when connection got established or data is received
		network::Connection::Handler init_connection_handler();
		
		// indicate if the network connection has been closed or not
		bool closed() const { return m_connection_closed; }


	private:

		void process_data(network::Shared_payload&& receive_buffer);
		// checks received packet for invalid header and data. If ddos is enabled this results into a ban
		void ddos_invalid_header(Packet const& packet);
		// Serialize a NXS block to byte stream
		network::Shared_payload serialize_block(LLP::CBlock* block);

		network::Connection::Sptr m_connection;		// network connection
		std::weak_ptr<Daemon_connection> m_daemon_connection; 
		std::weak_ptr<Data_registry> m_data_registry;	// global data access
		std::shared_ptr<spdlog::logger> m_logger;
		std::unique_ptr<LLP::DDOS_Filter> m_ddos;
		std::string m_nxsaddress;
		bool m_logged_in;
		bool m_isDDOS;	// use ddos or not
		std::atomic<bool> m_connection_closed;	// indicator for server if the network connection has been closed
		std::string m_remote_address;	// remote ip address
		Stats_connection m_stats;	// statistics per connection. Will be consumed by Stats_collector
	};

}


#endif // NEXUSPOOL_POOL_CONNECTION_HPP
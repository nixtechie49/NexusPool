#ifndef NEXUSPOOL_DAEMON_CONNECTION_HPP
#define NEXUSPOOL_DAEMON_CONNECTION_HPP

#include "network/connection.hpp"
#include "network/socket.hpp"
#include "LLP/block.h"
#include "chrono/timer.hpp"
#include <spdlog/spdlog.h>

#include <list>
#include <vector>
#include <memory>
#include <string>
#include <mutex>

namespace LLP
{
	class CBlock;
	class DDOS_Filter;
}

namespace nexuspool
{
	class Data_registry;
	class Pool_connection;
	struct Pool_connection_data
	{
		Pool_connection_data() : m_connection{}, m_blocks_requested{0}, m_blocks_waiting{0}, m_submit_block{} 
		{}
		Pool_connection_data(std::weak_ptr<Pool_connection> connection,
							uint16_t blocks_requested,
							uint16_t blocks_waiting)
			: m_connection{std::move(connection)}
			, m_blocks_requested{blocks_requested}
			, m_blocks_waiting{blocks_waiting} 
			, m_submit_block{}
		{}
		
		std::weak_ptr<Pool_connection> m_connection;
		uint16_t m_blocks_requested;
		uint16_t m_blocks_waiting;
		std::unique_ptr<LLP::CBlock> m_submit_block;
		
	};

	class Daemon_connection : public std::enable_shared_from_this<Daemon_connection>
	{
	public:
		using Uptr = std::unique_ptr<Daemon_connection>;

		Daemon_connection(network::Socket::Sptr&& socket, 
						std::weak_ptr<Data_registry> data_registry,
						chrono::Timer::Uptr&& maintenance_timer);
		bool connect(network::Endpoint remote_wallet);
		void add_pool_connection(std::weak_ptr<Pool_connection> connection);	// called from pool_connections (different threads)
		void request_block(uint16_t blocks_requested, std::string const& ip_address);
		
		// returns true if there wasn't a submit_block for that connection already
		bool submit_block(LLP::CBlock const& block, std::string const& ip_address);	

	private:

		void process_data(network::Shared_payload&& receive_buffer);
		LLP::CBlock deserialize_block(network::Shared_payload data);
		chrono::Timer::Handler maintenance_timer_handler();
		void cleanup_expired_pool_connections();
		Pool_connection_data& find_pool_connection(std::string const& ip_address);

		network::Socket::Sptr m_socket;
		network::Connection::Sptr m_connection;		// network connection
		std::weak_ptr<Data_registry> m_data_registry;	// global data access
		chrono::Timer::Uptr m_maintenance_timer;	// timer to delete non existing pool_connections from list
		std::shared_ptr<spdlog::logger> m_logger;
		std::vector<Pool_connection_data> m_pool_connections;
		std::mutex m_pool_connections_mutex;
	};

}


#endif // NEXUSPOOL_DAEMON_CONNECTION_HPP
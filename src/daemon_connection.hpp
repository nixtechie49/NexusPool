#ifndef NEXUSPOOL_DAEMON_CONNECTION_HPP
#define NEXUSPOOL_DAEMON_CONNECTION_HPP

#include "network/connection.hpp"
#include "network/socket.hpp"
#include "LLP/block.h"
#include <spdlog/spdlog.h>

#include <list>
#include <vector>
#include <memory>
#include <mutex>

namespace LLP
{
	class CBlock;
	class DDOS_Filter;
}

namespace nexuspool
{
	class Pool_connection;

	class Daemon_connection : public std::enable_shared_from_this<Daemon_connection>
	{
	public:
		using Uptr = std::unique_ptr<Daemon_connection>;

		Daemon_connection(network::Socket::Sptr&& socket);
		bool connect(network::Endpoint remote_wallet);
		void add_pool_connection(std::weak_ptr<Pool_connection> connection);	// called from pool_connections (different threads)

	private:

		void process_data(network::Shared_payload&& receive_buffer);
		LLP::CBlock deserialize_block(network::Shared_payload data);

		network::Socket::Sptr m_socket;
		network::Connection::Sptr m_connection;		// network connection
		std::shared_ptr<spdlog::logger> m_logger;
		std::vector<std::weak_ptr<Pool_connection>> m_pool_connections;
		std::mutex m_pool_connections_mutex;
	};

}


#endif // NEXUSPOOL_DAEMON_CONNECTION_HPP
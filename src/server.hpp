#ifndef NEXUSPOOL_SERVER_HPP
#define NEXUSPOOL_SERVER_HPP

#include "config.hpp"
#include "database/database.hpp"
#include <spdlog.h>

#include <thread>
#include <vector>
#include <memory>

namespace asio { class io_context; }
namespace nexuspool
{
namespace network { class Socket; class Component; }

class Pool_connection;

class Server
{
public:

	Server();
	~Server();

	bool init();
	void run();

private:

	std::vector<std::unique_ptr<Pool_connection>> m_pool_connections;
	std::unique_ptr<network::Component> m_network_component;
	std::shared_ptr<network::Socket> m_listen_socket;
	std::shared_ptr<::asio::io_context> m_io_context;
	std::vector<std::thread> m_io_threads;
	Config m_config;

	database::Database::Uptr m_database;


};

}


#endif // NEXUSPOOL_SERVER_HPP
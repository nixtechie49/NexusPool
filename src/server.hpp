#ifndef NEXUSPOOL_SERVER_HPP
#define NEXUSPOOL_SERVER_HPP

#include "config.hpp"
#include "chrono/timer.hpp"

#include <thread>
#include <vector>
#include <memory>
#include <mutex>

namespace asio { class io_context; }
namespace nexuspool
{
namespace network { class Socket; class Component; }
namespace chrono { class Timer; }

class Data_registry;
class Pool_connection;
class Daemon_connection;

class Server
{
public:

	Server();
	~Server();

	bool init();
	void run();

private:

	chrono::Timer::Handler maintenance_timer_handler();
	
	std::mutex m_pool_connections_mutex;
	std::vector<std::shared_ptr<Pool_connection>> m_pool_connections;
	std::shared_ptr<Daemon_connection> m_daemon_connection;
	
	std::unique_ptr<chrono::Timer> m_maintenance_timer;

	std::unique_ptr<network::Component> m_network_component;
	std::shared_ptr<network::Socket> m_listen_socket;
	std::shared_ptr<::asio::io_context> m_io_context;
	std::vector<std::thread> m_io_threads;

	Config m_config;

	std::shared_ptr<Data_registry> m_data_registry;
};

}


#endif // NEXUSPOOL_SERVER_HPP
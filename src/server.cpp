#include "server.hpp"
#include "data_registry.hpp"
#include "pool_connection.hpp"
#include "daemon_connection.hpp"
#include "network/create_component.hpp"
#include "network/component.hpp"

#include "chrono/timer_factory.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <asio/signal_set.hpp>

#include <chrono>

namespace nexuspool
{

Server::Server()
	: m_io_context{std::make_shared<::asio::io_context>()}
	, m_signals{std::make_shared<::asio::signal_set>(*m_io_context)}
{
	// Register to handle the signals that indicate when the server should exit.
// It is safe to register for the same signal multiple times in a program,
// provided all registration for the specified signal is made through Asio.
	m_signals->add(SIGINT);
	m_signals->add(SIGTERM);
#if defined(SIGQUIT)
	m_signals->add(SIGQUIT);
#endif // defined(SIGQUIT)

	m_signals->async_wait([this](auto, auto)
	{
		m_io_context->stop();
		// Wait for all threads in the pool to exit.
		for (std::size_t i = 0; i < m_io_threads.size(); ++i)
			m_io_threads[i].join();
	});
}

Server::~Server()
{
	m_io_context->stop();
	// Wait for all threads in the pool to exit.
	for (std::size_t i = 0; i < m_io_threads.size(); ++i)
		m_io_threads[i].join();
}

bool Server::init()
{
	if (!m_config.read_config())
	{
		return false;
	}		

	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	//if (!m_config.get_logfile().empty())
	//{	// if logfile available, only log level "info" and above to console
	//	console_sink->set_level(spdlog::level::info);
	//}
	// 10 mb logfiles, 5
	//auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(m_config.get_logfile(), 1048576 * 10, 5);
	//spdlog::logger multi_sink("logger", { console_sink, file_sink });
	//spdlog::logger multi_sink("logger", { console_sink });

	auto logger = spdlog::stdout_color_mt("logger");
	logger->set_level(spdlog::level::debug);

	// std::err logger
	auto console_err = spdlog::stderr_color_mt("console_err");

	// global data registry
	chrono::Timer_factory::Sptr timer_factory = std::make_shared<chrono::Timer_factory>(m_io_context);
	m_data_registry = std::make_shared<Data_registry>(m_config, timer_factory);
	m_data_registry->init();

	// network initialisation
	m_network_component = network::create_component(m_io_context);
	auto socket_factory = m_network_component->get_socket_factory();

	network::Endpoint listen_endpoint{ network::Transport_protocol::tcp, "127.0.0.1", m_config.get_pool_port() };
	m_listen_socket = socket_factory->create_socket(listen_endpoint);

	network::Endpoint local_ep{ network::Transport_protocol::tcp, "127.0.0.1", 0 };
	m_daemon_connection = std::make_shared<Daemon_connection>(std::move(socket_factory->create_socket(local_ep)), 
															  m_data_registry,
															  timer_factory);
	
	// timer
	m_maintenance_timer = timer_factory->create_timer();

	return true;
}

void Server::run()
{
	auto logger = spdlog::get("logger");

	// connect daemon to wallet	
	if (!m_daemon_connection->connect(network::Endpoint{ network::Transport_protocol::tcp, m_config.get_wallet_ip(), m_config.get_port() }))
	{
		logger->critical("Couldn't connect to wallet using ip {}", m_config.get_wallet_ip());
		return;
	}
	
	// start the maintenance timer (for cleaning up purposes)
	m_maintenance_timer->start(chrono::Seconds(m_config.get_maintenance_timer_seconds()), maintenance_timer_handler());	
	
	// on listen/accept, save created connection to pool_conenctions and call the connection_handler of created pool connection object
	network::Socket::Connect_handler socket_handler = [this](network::Connection::Sptr&& connection)
	{
		auto pool_connection = std::make_shared<Pool_connection>(std::move(connection), m_daemon_connection, m_data_registry);		
		
		std::lock_guard<std::mutex> lk(m_pool_connections_mutex);
		m_pool_connections.push_back(pool_connection);
		return pool_connection->init_connection_handler();
	};

	m_listen_socket->listen(socket_handler);

	for (std::size_t i = 0; i < m_config.get_connection_threads(); ++i)
	{
		m_io_threads.push_back(std::thread([io_context = m_io_context]() { io_context->run(); }));
	}

	m_io_context->run();
}

chrono::Timer::Handler Server::maintenance_timer_handler()
{
	return [this](bool canceled) 
	{
		if(canceled)	// don't do anything if the timer has been canceled
		{
			return;
		}
		
		// clean disconnected pool_connections
		std::lock_guard<std::mutex> lk(m_pool_connections_mutex);
		
		m_pool_connections.erase(std::remove_if(m_pool_connections.begin(), m_pool_connections.end(),
        [](auto connection) { return connection->closed(); }), m_pool_connections.end());
		
		// start the timer again
		m_maintenance_timer->start(chrono::Seconds(m_config.get_maintenance_timer_seconds()), maintenance_timer_handler());
	};		
}

}
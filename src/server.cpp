#include "server.hpp"
#include "pool_connection.hpp"
#include "daemon_connection.hpp"
#include "network/create_component.hpp"
#include "network/component.hpp"

#include <spdlog.h>
#include <sinks/stdout_color_sinks.h>
#include <sinks/rotating_file_sink.h>
#include <functional>

namespace nexuspool
{

Server::Server()
	: m_io_context{std::make_shared<::asio::io_context>()}
{
}

Server::~Server()
{
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
	if (!m_config.get_logfile().empty())
	{	// if logfile available, only log level "info" and above to console
		console_sink->set_level(spdlog::level::info);
	}
	// 10 mb logfiles, 5
	auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(m_config.get_logfile(), 1048576 * 10, 5);
	spdlog::logger multi_sink("logger", { console_sink, file_sink });

	// std::err logger
	auto console_err = spdlog::stderr_color_mt("console_err");


	// network initialisation
	m_network_component = network::create_component(m_io_context);
	auto socket_factory = m_network_component->get_socket_factory();

	network::Endpoint listen_endpoint{ network::Transport_protocol::tcp, "127.0.0.1", m_config.get_port() };
	m_listen_socket = socket_factory->create_socket(listen_endpoint);

	network::Endpoint local_ep{ network::Transport_protocol::tcp, "127.0.0.1", 0 };
	m_daemon_connection = std::make_unique<Daemon_connection>(std::move(socket_factory->create_socket(local_ep)));

	// database initialisation

	return true;
}

void Server::run()
{
	auto logger = spdlog::get("logger");
	// Create a pool of threads to run all of the io_services.
	std::vector<std::thread> io_threads;
	for (std::size_t i = 0; i < m_config.get_connection_threads(); ++i)
	{
		m_io_threads.push_back(std::thread([io_context = m_io_context]() { io_context->run(); }));
	}

	// connect daemon to wallet	
	if (!m_daemon_connection->connect(network::Endpoint{ network::Transport_protocol::tcp, m_config.get_wallet_ip(), 9325 }))
	{
		logger->critical("Couldn't connect to wallet using ip {}", m_config.get_wallet_ip());
		return;
	}

	// on listen/accept, save created connection to pool_conenctions and call the connection_handler of created pool connection object
	network::Socket::Connect_handler socket_handler = [this](network::Connection::Sptr&& connection)
	{
		m_pool_connections.push_back(std::make_shared<Pool_connection>(std::move(connection), m_config.get_min_share()));
		return m_pool_connections.back()->connection_handler;
	};

	m_listen_socket->listen(socket_handler);
}

}
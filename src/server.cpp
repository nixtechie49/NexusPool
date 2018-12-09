#include "server.hpp"
#include "pool_connection.hpp"
#include "network/create_component.hpp"
#include "network/component.hpp"

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

	// network initialisation
	m_network_component = network::create_component(m_io_context);
	auto socket_factory = m_network_component->get_socket_factory();

	network::Endpoint listen_endpoint{ network::Transport_protocol::tcp, "127.0.0.1", m_config.get_port() };
	m_listen_socket = socket_factory->create_socket(listen_endpoint);

	// database initialisation

	return true;


}

void Server::run()
{
	// Create a pool of threads to run all of the io_services.
	std::vector<std::thread> io_threads;
	for (std::size_t i = 0; i < m_config.get_connection_threads(); ++i)
	{
		m_io_threads.push_back(std::thread([io_context = m_io_context]() { io_context->run(); }));
	}


	// on liste/accept, save created connection to pool_conenctions and call the connection_handler of created pool connection object
	network::Socket::Connect_handler socket_handler = [this](network::Connection::Sptr&& connection)
	{
		m_pool_connections.push_back(std::make_unique<Pool_connection>(std::move(connection)));
		return m_pool_connections.back()->connection_handler;
	};

	m_listen_socket->listen(socket_handler);
}

}
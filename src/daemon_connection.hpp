#ifndef NEXUSPOOL_DAEMON_CONNECTION_HPP
#define NEXUSPOOL_DAEMON_CONNECTION_HPP

#include "network/connection.hpp"
#include "network/socket.hpp"
#include "LLP/block.h"
#include <spdlog/spdlog.h>

#include <list>
#include <memory>

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

		/** Enumeration to interpret Daemon Packets. **/
		enum
		{
			/** DATA PACKETS **/
			BLOCK_DATA = 0,
			SUBMIT_BLOCK = 1,
			BLOCK_HEIGHT = 2,
			SET_CHANNEL = 3,
			BLOCK_REWARD = 4,
			SET_COINBASE = 5,
			GOOD_BLOCK = 6,
			ORPHAN_BLOCK = 7,


			/** DATA REQUESTS **/
			CHECK_BLOCK = 64,



			/** REQUEST PACKETS **/
			GET_BLOCK = 129,
			GET_HEIGHT = 130,
			GET_REWARD = 131,


			/** SERVER COMMANDS **/
			CLEAR_MAP = 132,
			GET_ROUND = 133,


			/** RESPONSE PACKETS **/
			GOOD = 200,
			FAIL = 201,


			/** VALIDATION RESPONSES **/
			COINBASE_SET = 202,
			COINBASE_FAIL = 203,

			NEW_ROUND = 204,
			OLD_ROUND = 205,


			/** GENERIC **/
			PING = 253,
			CLOSE = 254
		};

		Daemon_connection(network::Socket::Sptr&& socket);

		bool connect(network::Endpoint remote_wallet);


	private:

		void process_data(network::Shared_payload&& receive_buffer);
		LLP::CBlock deserialize_block(network::Shared_payload data);

		network::Socket::Sptr m_socket;
		network::Connection::Sptr m_connection;		// network connection
		std::shared_ptr<spdlog::logger> m_logger;
		std::list<std::weak_ptr<Pool_connection>> m_pool_connections;
	};

}


#endif // NEXUSPOOL_DAEMON_CONNECTION_HPP
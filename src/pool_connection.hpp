#ifndef NEXUSPOOL_POOL_CONNECTION_HPP
#define NEXUSPOOL_POOL_CONNECTION_HPP

#include "network/connection.hpp"
#include "LLP/ddos.hpp"
#include "LLP/block.h"
#include "hash/templates.h"
#include "hash/uint1024.h"
#include "stats_collector.hpp"
#include <spdlog/spdlog.h>

#include <string>
#include <memory>
#include <atomic>
#include <set>
#include <map>

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

		// threadsafe because it wil only be written at connection init (before daemon gets it)
		std::string const& get_remote_address() const { return m_remote_address; }	
		std::string const& get_nxs_address() const { return m_nxsaddress; }	

		void new_round(uint32_t new_height);
		void add_block(LLP::CBlock const& block);

	private:

		void process_data(network::Shared_payload&& receive_buffer);
		// checks received packet for invalid header and data. If ddos is enabled this results into a ban
		void ddos_invalid_header(Packet const& packet);

		bool find_hash_in_blocks_requested(uint1024 const& hash_block) const;

		network::Connection::Sptr m_connection;		// network connection
		std::weak_ptr<Daemon_connection> m_daemon_connection; 
		std::weak_ptr<Data_registry> m_data_registry;	// global data access
		std::shared_ptr<spdlog::logger> m_logger;
		std::unique_ptr<LLP::DDOS_Filter> m_ddos;
		std::mutex m_primes_mutex;		// must be protected because daemon_connection can clear the set
		std::set<uint1024> m_primes;	// stores the found shares per round
		mutable std::mutex m_blocks_requested_mutex;
		std::map<uint1024, LLP::CBlock> m_blocks_requested;	// map of requested blocks per round
		std::string m_nxsaddress;
		bool m_logged_in;
		bool m_isDDOS;	// use ddos or not
		uint32_t m_min_share; // minimum difficulty, set by config (given from data_registry)
		std::atomic<bool> m_connection_closed;	// indicator for server if the network connection has been closed
		std::string m_remote_address;	// remote ip address
		std::atomic<uint16_t> m_num_blocks_requested;
		Stats_connection m_stats;	// statistics per connection. Will be consumed by Stats_collector
	};

}


#endif // NEXUSPOOL_POOL_CONNECTION_HPP
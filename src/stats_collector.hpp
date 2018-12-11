#ifndef NEXUSPOOL_STATS_COLLECTOR_HPP
#define NEXUSPOOL_STATS_COLLECTOR_HPP

#include <string>
#include <list>
#include <memory>
#include <mutex>

#include "database/types.hpp"
#include "chrono/timer.hpp"
//#ifdef _WIN32_WINNT
//#include <mpir.h>
//#else
//#include <gmp.h>
//#endif

namespace nexuspool
{
	class Pool_connection;

	// Statistics per pool connection. Keep the data locally, Stats_collector will read them every few seconds
	class Stats_connection
	{
	public:

		std::string m_address;
		database::connection_data m_miner_data;


	};

	/** Class used to generate JSON-formatted pool statistics **/
	class Stats_collector : std::enable_shared_from_this<Stats_collector>
	{
	public:

		Stats_collector(chrono::Timer::Uptr&& persistance_timer);
		~Stats_collector();
		void init();

		void add_pool_connection(std::shared_ptr<Pool_connection> pool_connection);


		void increment_connection_count(std::string address, std::string GUID);
		void decrement_connection_count(std::string address, std::string GUID);
		int get_connectoin_count(std::string address);
		void update_connection_data(std::string address, std::string guid, double PPS, double WPS);

		void update_pool_data();
		void update_account_data(bool update_all = false);
		void clear_account_round_shares();

		void save_current_round();
		void flag_round_as_orphan(uint32_t round);


		void add_account_earnings(std::string account_address,
								uint32_t round,
								uint32_t block_number,
								uint64_t shares,
								uint64_t amount_earned,
								time_t time);
		void DeleteAccountEarnings(std::string account_address, uint32_t round);

		void AddAccountPayment(std::string account_address,
							uint32_t round,
							uint32_t block_number,
							uint64_t amount_paid,
							time_t time);
		void DeleteAccountPayment(std::string account_address, uint32_t round);


	private:

		// use list because connections get removed every now and then
		std::list<std::weak_ptr<Pool_connection>> m_pool_connections;
		std::map<std::string, database::connection_data> CONNECTIONS_BY_ADDRESS;

		chrono::Timer::Uptr m_persistance_timer;

		std::mutex pool_connections_mutex;
		std::mutex POOL_DATA_MUTEX;
		std::mutex ROUND_HISTORY_MUTEX;
		std::mutex ACCOUNT_DATA_MUTEX;
		std::mutex ACCOUNT_EARNINGS_MUTEX;
	};

}
#endif

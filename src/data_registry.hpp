#ifndef NEXUSPOOL_DATA_REGISTRY_HPP
#define NEXUSPOOL_DATA_REGISTRY_HPP

#include "database/database.hpp"
#include "chrono/timer_factory.hpp"
#include "chrono/timer.hpp"
#include "stats_collector.hpp"
#include "coinbase.hpp"
#include "LLD/database.h"
#include "LLD/record.h"
#include "LLP/block.h"
#include "banned_users.hpp"
#include "payout_manager.hpp"
#include <spdlog/spdlog.h>

#include <list>
#include <memory>
#include <atomic>
#include <algorithm>

namespace nexuspool
{
	class Config;

	using Banned_users_file = Banned_users<Banned_persister_file>;

	class Data_registry : public std::enable_shared_from_this<Data_registry>
	{
	public:

		Data_registry(Config& config, chrono::Timer_factory::Sptr timer_factory);
		~Data_registry();

		void init();

		// only one database
		std::unique_ptr<database::Database> m_database;

		// only one statistics collector, shared_ptr because of async timer functionality
		std::shared_ptr<Stats_collector> m_stats_collector;

		// Databases to be used in Memory. Shared_ptr because writing to disk will be handled async with timer
		using Account_db = LLD::Database<std::string, LLD::Account>;
		using Block_db = LLD::Database<uint1024, LLD::Block>;
		std::shared_ptr<Block_db> m_block_db;				// Threadsafe
		std::shared_ptr<Account_db> m_account_db;			// Threadsafe
		std::unique_ptr<Banned_users_file> m_banned_users;	// Threadsafe

			/** Used to Sort the Database Account Keys by Balance. **/
		bool BalanceSort(const std::string& firstElem, const std::string& secondElem) { }


		/** Return List of Accounts Sorted by Balance [Highest Balance First]. **/
		std::vector<std::string> get_sorted_accounts()
		{
			std::vector<std::string> vAccounts = m_account_db->GetKeys();
			std::sort(vAccounts.begin(), vAccounts.end(), [this](std::string const& firstElem, std::string const& secondElem)
			{
				return m_account_db->GetRecord(firstElem).nAccountBalance > m_account_db->GetRecord(secondElem).nAccountBalance;
			});

			return vAccounts;
		}

		bool get_use_ddos() const { return m_use_ddos; }
		uint32_t get_min_share() const { return m_min_share; }
		
		void set_submit_block(LLP::CBlock const& block) { m_submit_block = block; }
		LLP::CBlock const& get_submit_block() const { return m_submit_block; }

		Config const& get_config() const { return m_config; }
		Payout_manager const& get_payout_manager() const { return m_payout_manager; }

		// only access to threadsafe coinbase
		Coinbase_mt& get_coinbase() { return m_coinbase_mt; }
		Coinbase_mt const& get_coinbase() const { return m_coinbase_mt; }
		
		// global data
		std::atomic<uint32_t> m_best_height;
		std::atomic<uint32_t> m_current_round;		

	private:
	
		chrono::Timer::Handler persistance_handler();

		Config& m_config;
		Payout_manager m_payout_manager;
		chrono::Timer_factory::Sptr m_timer_factory;
		chrono::Timer::Uptr m_persistance_timer;
		uint32_t m_min_share;	// not protected -> will be set one time at startup and then only read by multiple connections
		bool m_use_ddos;		// not protected -> will be set one time at startup and then only read by multiple connections
		Coinbase m_coinbase;
		Coinbase_mt m_coinbase_mt;
		LLP::CBlock m_submit_block;
	};

}


#endif // NEXUSPOOL_DATA_REGISTRY_HPP
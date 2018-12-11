#include "data_registry.hpp"
#include "config.hpp"
#include "database/sqlite_database.hpp"

namespace nexuspool
{
	Data_registry::Data_registry(Config& config, chrono::Timer_factory::Sptr&& timer_factory)
		: m_database{}
		, m_stats_collector{std::make_unique<Stats_collector>()}
		, m_block_db{std::make_shared<Block_db>("blk.dat")}
		, m_account_db{std::make_shared<Account_db>("acct.dat")}
		, m_config{ config } 
		, m_timer_factory{std::move(timer_factory)}
		, m_persistance_timer{m_timer_factory->create_timer()}
		, m_min_share{m_config.get_min_share()}
		, m_use_ddos{ m_config.get_use_ddos()}
	{

	}

	void Data_registry::init()
	{
		if (m_config.get_database_type() == "sqlite")
		{
			m_database = std::make_unique<database::Sqlite_database>(m_config.get_database_file());
		}

		m_persistance_timer->start(chrono::Seconds(10), [self = shared_from_this()](bool canceled) 
		{
			if (canceled)
			{
				return;
			}

			self->m_block_db->WriteToDisk();
			self->m_account_db->WriteToDisk();

			// write statistics to db

		});
	}
}
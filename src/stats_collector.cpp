#include "stats_collector.hpp"
#include "pool_connection.hpp"

namespace nexuspool
{
Stats_collector::Stats_collector(chrono::Timer::Uptr&& persistance_timer)
	: m_persistance_timer{std::move(persistance_timer)}
{
}

Stats_collector::~Stats_collector()
{

}

void Stats_collector::init()
{
	m_persistance_timer->start(chrono::Seconds(10), [self = shared_from_this()](bool canceled) 
	{
		if(canceled)
		{
			return;
		}

		// collect data from pool_connections

	});
}

void Stats_collector::add_pool_connection(std::shared_ptr<Pool_connection> pool_connection)
{
	std::lock_guard<std::mutex> lk(pool_connections_mutex);

	m_pool_connections.push_back(std::move(pool_connection));
}
	
void Stats_collector::increment_connection_count(std::string address, std::string GUID)
{

}

void Stats_collector::decrement_connection_count(std::string address, std::string GUID)
{

}

int Stats_collector::get_connectoin_count(std::string address)
{
	return 0;
}

void Stats_collector::update_connection_data(std::string address, std::string guid, double PPS, double WPS)
{

}

void Stats_collector::update_pool_data()
{

}

void Stats_collector::update_account_data(bool update_all)
{

}

void Stats_collector::clear_account_round_shares()
{

}

void Stats_collector::save_current_round()
{

}

void Stats_collector::flag_round_as_orphan(uint32_t round)
{

}

void Stats_collector::add_account_earnings(std::string account_address,
										uint32_t round,
										uint32_t block_number,
										uint64_t shares,
										uint64_t amount_earned,
										time_t time)
{

}

void Stats_collector::DeleteAccountEarnings(std::string account_address, uint32_t round)
{

}

void Stats_collector::AddAccountPayment(std::string account_address,
										uint32_t round,
										uint32_t block_number,
										uint64_t amount_paid,
										time_t time)
{

}

void Stats_collector::DeleteAccountPayment(std::string account_address, uint32_t round)
{

}
}
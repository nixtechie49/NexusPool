#ifndef NEXUSPOOL_CONFIG_HPP
#define NEXUSPOOL_CONFIG_HPP
#include <string>

namespace nexuspool
{
class Config
{
public:
	Config();

	bool read_config();
	void print_config() const;

	std::string const& get_wallet_ip() const { return m_wallet_ip; }
	uint16_t get_port() const { return m_port; }
	uint16_t get_pool_port() const { return m_pool_port; }
	uint32_t get_connection_threads() const { return m_connection_threads; }
	bool get_use_ddos() const { return m_use_ddos; }

	uint32_t get_min_share() const { return m_min_share; }
	uint32_t get_pool_fee() const { return m_pool_fee; }

	std::string const& get_logfile() const { return m_logfile; }
	std::string const& get_database_type() const { return m_database_type; }
	std::string const& get_database_file() const { return m_database_file; }
	
	uint32_t get_maintenance_timer_seconds() const { return m_maintenance_timer_seconds; }	
	uint32_t get_persistance_timer_seconds() const { return m_persistance_timer_seconds; }
	uint32_t get_block_timer_milliseconds() const { return m_block_timer_milliseconds; }	
	uint32_t get_orphan_check_timer_seconds() const { return m_orphan_check_timer_seconds; }
	uint32_t get_request_timer_seconds() const { return m_request_timer_seconds; }	


private:

	std::string  m_wallet_ip;
	uint16_t     m_port;
	uint16_t     m_pool_port;
	uint32_t     m_connection_threads;
	bool         m_use_ddos;
	int          m_r_score;
	int          m_c_score;
	uint32_t     m_min_share;
	uint32_t     m_pool_fee;
	std::string  m_logfile;
	std::string  m_database_type;
	std::string  m_database_file;
	
	uint32_t	 m_maintenance_timer_seconds;
	uint32_t	 m_persistance_timer_seconds;
	uint32_t	 m_block_timer_milliseconds;
	uint32_t	 m_orphan_check_timer_seconds;
	uint32_t	 m_request_timer_seconds;
	

	std::string  strStatsDBServerIP;
	int          nStatsDBServerPort;
	std::string  strStatsDBUsername;
	std::string  strStatsDBPassword;
	// number of second between saves for each account
	int          nSaveConnectionStatsFrequency;
	// number of seconds between each time series for connection stats.
	// this defaults to 300 seconds, which means that even though we capture the realtime stats
	// every nSaveConnectionStatsFrequency seconds, we capture the historical stats every 300 
	int          nSaveConnectionStatsSeriesFrequency; //

};

}


#endif // NEXUSPOOL_CONFIG_HPP
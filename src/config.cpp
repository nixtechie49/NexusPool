
#include "config.hpp"
#include "json/json.hpp"

using json = nlohmann::json;

#include <fstream>
#include <iostream>

namespace nexuspool
{
	Config::Config()
		: m_wallet_ip{ "127.0.0.1" }
		, m_port{ 9549 }
		, m_connection_threads{ 20 }
		, m_use_ddos{true}
		, m_r_score{20}
		, m_c_score{2}
		, m_min_share{ 40000000 }
		, m_pool_fee{1}
		, m_logfile{""}		// no logfile usage, default
		, m_database_type{"sqlite"}	// use sqlite as default db
		, m_database_file{""}
	{
		strStatsDBServerIP = "127.0.0.1";
		nStatsDBServerPort = 3306;
		strStatsDBUsername = "mysql";
		strStatsDBPassword = "mysql";
		nSaveConnectionStatsFrequency = 10;
		nSaveConnectionStatsSeriesFrequency = 300;

	}

	void Config::print_config() const
	{
		std::cout << "Configuration: " << std::endl;
		std::cout << "-------------" << std::endl;
		std::cout << "Wallet IP: " << m_wallet_ip << std::endl;
		std::cout << "Port: " << m_port << std::endl;

		std::cout << "Connection Threads: " << m_connection_threads << std::endl;

		std::cout << "bDDOS: " << m_use_ddos << std::endl;;
		std::cout << "rScore: " << m_r_score << std::endl;
		std::cout << "cScore: " << m_c_score << std::endl;
		std::cout << "Min Share Diff: " << m_min_share << std::endl;
		std::cout << "Pool Fee: " << m_pool_fee << std::endl;

		std::cout << "Logfile: " << m_logfile << std::endl;
		std::cout << "Database: " << m_database_type << std::endl;
		std::cout << "Database File: " << m_database_file << std::endl;

		std::cout << "Stats DB Server IP: " << strStatsDBServerIP << std::endl;
		std::cout << "Stats DB Port: " << nStatsDBServerPort << std::endl;
		std::cout << "Stats DB Username: " << strStatsDBUsername << std::endl;
		std::cout << "Stats DB Password: " << strStatsDBPassword << std::endl;
		std::cout << "Save Connection Stats Frequency: " << nSaveConnectionStatsFrequency << " seconds" << std::endl;
		std::cout << "Save Connection Stats Series Frequency: " << nSaveConnectionStatsSeriesFrequency << " seconds" << std::endl;

		std::cout << "-------------" << std::endl;

	}

	bool Config::read_config()
	{
		std::cout << "Reading config file pool.conf" << std::endl;

		std::ifstream config_file("pool.conf");
		if (!config_file.is_open())
		{
			std::cerr << "Unable to read pool.conf";
			return false;
		}

		json j = json::parse(config_file);
		j.at("wallet_ip").get_to(m_wallet_ip);
		j.at("port").get_to(m_port);

		j.at("connection_threads").get_to(m_connection_threads);
		j.at("enable_ddos").get_to(m_use_ddos);
		j.at("ddos_rscore").get_to(m_r_score);
		j.at("ddos_cscore").get_to(m_c_score);
		j.at("min_share").get_to(m_min_share);
		j.at("pool_fee").get_to(m_pool_fee);

		j.at("logfile").get_to(m_logfile);
		j.at("database_type").get_to(m_database_type);
		j.at("database_file").get_to(m_database_file);

		j.at("stats_db_server_ip").get_to(strStatsDBServerIP);
		j.at("stats_db_server_port").get_to(nStatsDBServerPort);
		j.at("stats_db_username").get_to(strStatsDBUsername);
		j.at("stats_db_password").get_to(strStatsDBPassword);
		j.at("connection_stats_frequency").get_to(nSaveConnectionStatsFrequency);
		j.at("connection_stats_series_frequency").get_to(nSaveConnectionStatsSeriesFrequency);

		print_config();
		// TODO Need to add exception handling here and set bSuccess appropriately
		return true;
	}


} // end namespace
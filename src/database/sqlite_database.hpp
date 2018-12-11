
#ifndef NEXUSPOOL_DATABASE_SQLITE_DATABASE_HPP
#define NEXUSPOOL_DATABASE_SQLITE_DATABASE_HPP

#include "types.hpp"
#include "database.hpp"
#include "sqlite/sqlite3.h"
#include <spdlog/spdlog.h>
#include <memory>
#include <string>

namespace nexuspool {
namespace database {

class Sqlite_database : public Database
{
public:
	Sqlite_database(std::string db_name);
	~Sqlite_database();

	bool init() override;
	void close() override;

	void load_banned_accounts(std::vector<std::string> & banned_accounts) override;
	void save_banned_accounts(std::vector<std::string> const& banned_accounts) override;

	void update_pool_data(pool_data const& pool_data) override;
	void add_round_data(round_data const& round_data) override;
	void flag_round_as_orphan(round_data const& round_data) override;
	void update_account_data(account_data const& account_ata) override;
	void clear_account_round_shares() override;
	void save_connection_data(connection_data const& connection_data) override;
	void add_account_earnings(account_earning_transaction const& account_earning_transaction) override;
	void delete_account_earnings(account_earning_transaction const& account_earning_transaction) override;
	void add_account_payment(account_payment_transaction const& account_earning_transaction) override;
	void delete_account_payment(account_payment_transaction const& account_earning_transaction) override;

private:
	bool run_simple_query(std::string query);
	void exec_statement(sqlite3_stmt *stmt);
	void bind_param(sqlite3_stmt *stmt, const char *name, std::string const &value);
	void bind_param(sqlite3_stmt *stmt, const char *name, int value);
	void bind_param(sqlite3_stmt *stmt, const char *name, int64_t value);
	void bind_param(sqlite3_stmt *stmt, const char *name, double value);

	bool m_init_complete;
	std::string m_db_name;
	sqlite3 *m_handle;
	std::shared_ptr<spdlog::logger> m_logger;

	// prepared statements
	sqlite3_stmt *m_update_pool_data_stmt;
	sqlite3_stmt *m_save_connection_data_stmt;
	sqlite3_stmt *m_add_round_data_stmt;
	sqlite3_stmt *m_flag_round_as_orphan_stmt;
	sqlite3_stmt *m_update_account_data_stmt;
	sqlite3_stmt *m_clear_account_round_shares_stmt;
	sqlite3_stmt *m_add_account_earnings_stmt;
	sqlite3_stmt *m_delete_account_earnings_stmt;
	sqlite3_stmt *m_add_account_payment_stmt;
	sqlite3_stmt *m_delete_account_payment_stmt;
};

} // namespace database
} // namespace nexuspool

#endif

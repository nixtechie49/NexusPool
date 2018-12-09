#include "sqlite_database.hpp"
#include <chrono>
#include <thread>
#include <iostream>

namespace nexuspool {
namespace database {

Sqlite_database::Sqlite_database(std::string db_name)
    : m_handle{nullptr}, m_db_name{std::move(db_name)} {}

Sqlite_database::~Sqlite_database() {
  sqlite3_finalize(m_update_pool_data_stmt);
  sqlite3_finalize(m_save_connection_data_stmt);
  sqlite3_finalize(m_add_round_data_stmt);
  sqlite3_finalize(m_flag_round_as_orphan_stmt);
  sqlite3_finalize(m_update_account_data_stmt);
  sqlite3_finalize(m_clear_account_round_shares_stmt);
  sqlite3_finalize(m_add_account_earnings_stmt);
  sqlite3_finalize(m_delete_account_earnings_stmt);
  sqlite3_finalize(m_add_account_payment_stmt);
  sqlite3_finalize(m_delete_account_payment_stmt);

  sqlite3_close(m_handle);
  sqlite3_shutdown();
}

void Sqlite_database::exec_statement(sqlite3_stmt *stmt) {
  int ret = sqlite3_step(m_update_pool_data_stmt);
  while (ret == SQLITE_BUSY) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ret = sqlite3_step(m_update_pool_data_stmt);
  }
}

bool Sqlite_database::run_simple_query(std::string query) {
  sqlite3_stmt *stmt = NULL;
  int rc = sqlite3_prepare_v2(m_handle, "CREATE TABLE tbl ( str TEXT )", -1,
                              &stmt, NULL);
  if (rc != SQLITE_OK) {
    return false;
  }

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    return false;
  }

  sqlite3_finalize(stmt);

  return true;
}

void Sqlite_database::bind_param(sqlite3_stmt *stmt, const char *name,
                                 std::string const &value) {
  int index = sqlite3_bind_parameter_index(stmt, name);
  if (index == 0) {
    std::cerr << "bind_param index not found" << std::endl;
    return;
  }

  sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_TRANSIENT);
}

void Sqlite_database::bind_param(sqlite3_stmt *stmt, const char *name,
                                 int value) {
  int index = sqlite3_bind_parameter_index(stmt, name);
  if (index == 0) {
    std::cerr << "bind_param index not found" << std::endl;
    return;
  }

  sqlite3_bind_int(stmt, index, value, -1, SQLITE_TRANSIENT);
}

void Sqlite_database::bind_param(sqlite3_stmt *stmt, const char *name,
                                 double value) {
  int index = sqlite3_bind_parameter_index(stmt, name);
  if (index == 0) {
    std::cerr << "bind_param index not found" << std::endl;
    return;
  }

  sqlite3_bind_double(stmt, index, value, -1, SQLITE_TRANSIENT);
}

bool init() {
  char *error_msg = nullptr;
  int result;

  sqlite3_initialize();
  result = sqlite3_open_v2(m_db_name, &m_handle,
                           SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

  if (result) {
    // logger
    std::cerr << "Can't open database: " << sqlite3_errmsg(m_handle)
              << std::endl;
    return false;
  }

  if (!run_simple_query("CREATE DATABASE IF NOT EXISTS nxspool;"))
    return false;
  // if (!run_simple_query("USE nxspool;"))
  //  return false;
  if (!run_simple_query("CREATE TABLE IF NOT EXISTS pool_data\
    (id INTEGER PRIMARY KEY AUTOINCREMENT, \
        round_number INTEGER NOT NULL,\
        block_number INTEGER DEFAULT NULL,\
        round_reward INTEGER DEFAULT NULL,\
        total_shares INTEGER DEFAULT NULL,\
        connection_count INTEGER DEFAULT NULL);"))
    return false;
  if (!run_simple_query("CREATE TABLE IF NOT EXISTS round_history\
    ( round_number INTEGER PRIMARY KEY,\
        block_number INTEGER DEFAULT NULL,\
        block_hash TEXT NOT NULL,\
        round_reward INTEGER DEFAULT NULL,\
        total_shares INTEGER DEFAULT NULL,\
        block_finder TEXT NOT NULL,\
        orphan INTEGER DEFAULT NULL,\
        block_found_time TEXT NOT NULL);"))
    return false;
  if (!run_simple_query("CREATE TABLE IF NOT EXISTS connection_history\
    ( account_address TEXT NOT NULL,\
        series_time TEXT NOT NULL,\
        last_save_time TEXT NOT NULL,\
        connection_count INTEGER DEFAULT NULL,\
        pps REAL DEFAULT NULL,\
        wps REAL DEFAULT NULL,\
        PRIMARY KEY (account_address, series_time));"))
    return false;
  if (!run_simple_query("CREATE TABLE IF NOT EXISTS account_data\
    ( account_address TEXT PRIMARY KEY,\
        last_save_time TEXT NOT NULL,\
        connection_count INTEGER DEFAULT NULL,\
        round_shares INTEGER DEFAULT NULL,\
        balance INTEGER DEFAULT NULL,\
        pending_payout INTEGER DEFAULT NULL);"))
    return false;
  if (!run_simple_query("CREATE TABLE IF NOT EXISTS earnings_history\
    ( account_address TEXT PRIMARY KEY,\
        round_number INTEGER NOT NULL ,\
        block_number INTEGER DEFAULT NULL,\
        round_shares INTEGER DEFAULT NULL,\
        amount_earned INTEGER DEFAULT NULL,\
        datetime TEXT NOT NULL);"))
    return false;
  if (!run_simple_query("CREATE INDEX idx_earnings_history ON earnings_history\
     (account_address, round_number);"))
    return false;
  if (!run_simple_query("CREATE TABLE IF NOT EXISTS payment_history\
    ( account_address TEXT PRIMARY KEY,\
        round_number INTEGER NOT NULL ,\
        block_number INTEGER DEFAULT NULL,\
        amount_paid INTEGER DEFAULT NULL,\
        datetime TEXT NOT NULL);"))
    return false;
  if (!run_simple_query("CREATE INDEX idx_payment_history ON payment_history\
     (account_address, block_number);"))
    return false;

  // create prepared statements
  int ret = sqlite3_prepare_v2(
      m_handle,
      "REPLACE INTO nxspool.pool_data VALUES (:round, :blocknumber, "
      ":roundreward, :totalshares, :connectioncount);",
      -1, &m_update_pool_data_stmt, NULL);
  if (SQLITE_OK != ret) {
    return false;
  }

  ret = sqlite3_prepare_v2(
      m_handle,
      "REPLACE INTO nxspool.connection_history VALUES (':address', "
      "':seriestime', ':time', :connectioncount, :pps, :wps);",
      -1, &m_save_connection_data_stmt, NULL);
  if (SQLITE_OK != ret) {
    return false;
  }

  ret = sqlite3_prepare_v2(
      m_handle,
      "INSERT INTO nxspool.round_history VALUES (:round, :blocknumber,\
      ':hashblock', :roundrewards, :totalshares, ':blockfinder', :orphan, ':blockfoundtime');",
      -1, &m_add_round_data_stmt, NULL);
  if (SQLITE_OK != ret) {
    return false;
  }

  ret = sqlite3_prepare_v2(m_handle,
                           "UPDATE nxspool.round_history SET orphan = 1 WHERE "
                           "round_number = :round;",
                           -1, &m_flag_round_as_orphan_stmt, NULL);
  if (SQLITE_OK != ret) {
    return false;
  }

  ret = sqlite3_prepare_v2(
      m_handle,
      "INSERT INTO nxspool.account_data (account_address, last_save_time, connection_count, round_shares, balance, pending_payout) \
      VALUES (':accountaddress', :currenttime, :connections, :shares, :balance, :pendingpayouts) \ 
      ON CONFLICT(account_address) DO UPDATE SET last_save_time = excluded.last_save_time,\ 
      connection_count = excluded.connection_count
      round_shares = excluded.round_shares
      balance = excluded.balance
      pending_payout = excluded.pending_payout;",-1, &m_update_account_data_stmt, NULL);
  if (SQLITE_OK != ret) {
    return false;
  }

   ret = sqlite3_prepare_v2(
      m_handle,"UPDATE nxspool.account_data SET round_shares = 0 ;",
      -1, &m_clear_account_round_shares_stmt, NULL);
  if (SQLITE_OK != ret) {
    return false;
  }

     ret = sqlite3_prepare_v2(
      m_handle,"INSERT INTO nxspool.earnings_history VALUES (':accountaddress', :round, :blocknumber, :shares, :amountearned, ':time';",
      -1, &m_add_account_earnings_stmt, NULL);
  if (SQLITE_OK != ret) {
    return false;
  }


       ret = sqlite3_prepare_v2(
      m_handle,"DELETE FROM nxspool.earnings_history WHERE account_address=':accountaddress' AND round_number= :round;",
      -1, &m_delete_account_earnings_stmt, NULL);
  if (SQLITE_OK != ret) {
    return false;
  }

         ret = sqlite3_prepare_v2(
      m_handle,"INSERT INTO nxspool.payment_history VALUES (':accountaddress', :round, :blocknumber, :amountpaid, ':time');",
      -1, &m_add_account_payment_stmt, NULL);
  if (SQLITE_OK != ret) {
    return false;
  }

           ret = sqlite3_prepare_v2(
      m_handle,"DELETE FROM nxspool.payment_history WHERE account_address=':accountaddress' AND round_number= :round;",
      -1, &m_delete_account_payment_stmt, NULL);
  if (SQLITE_OK != ret) {
    return false;
  }


return true;
}

void Sqlite_database::update_pool_data(PoolData const &lPoolData) {
  bind_param(m_update_pool_data_stmt, "round", ip);
  bind_param(m_update_pool_data_stmt, "blocknumber", ip);
  bind_param(m_update_pool_data_stmt, "roundreward", ip);
  bind_param(m_update_pool_data_stmt, "totalshares", ip);
  bind_param(m_update_pool_data_stmt, "connectioncount", ip);

  exec_statement(m_update_pool_data_stmt);
}

void Sqlite_database::add_round_data(RoundData const &lRoundData) {

  bind_param(m_save_connection_data_stmt, "address", ip);
  bind_param(m_save_connection_data_stmt, "seriestime", ip);
  bind_param(m_save_connection_data_stmt, "time", ip);
  bind_param(m_save_connection_data_stmt, "connectioncount", ip);
  bind_param(m_save_connection_data_stmt, "pps", ip);
  bind_param(m_save_connection_data_stmt, "wps", ip);

  exec_statement(m_save_connection_data_stmt);
}

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

// Get current date/time, format is YYYY-MM-DD HH:mm:ss
inline std::string const current_date_time() {
  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H-%M-%S");
  return oss.str();
}

} // namespace database
} // namespace nexuspool
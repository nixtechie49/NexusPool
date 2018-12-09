
#ifndef NEXUSPOOL_DATABASE_CONNECTION_HPP
#define NEXUSPOOL_DATABASE_CONNECTION_HPP

#include <memory>
#include <string>

namespace nexuspool {
namespace database {

enum class Result { success = 0, error };

struct Config {
  std::string db_name;
  std::string ip;
  uint16_t port;
  std::string username;
  std::string password;
};

class Database_connection {
public:
  using Database_connection::Sptr = std::shared_ptr<Database_connection>;

  virtual ~Database_connection() = default;

  virtual Result open(Config const &database_config) = 0;
  virtual void close() = 0;

  virtual Result query(std::string const &query) = 0;
  virtual Result prepared_query(std::string const &query) = 0;
};

} // namespace database
} // namespace nexuspool

#endif

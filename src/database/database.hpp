
#ifndef NEXUSPOOL_DATABASE_DATABASE_HPP
#define NEXUSPOOL_DATABASE_DATABASE_HPP

#include <memory>

namespace nexuspool {
namespace database {

struct pool_data;
struct account_data;
struct round_data;
struct connection_data;
struct account_earning_transaction;
struct account_payment_transaction;

class Database
{
public:

	using Uptr = std::unique_ptr<Database>;

	virtual ~Database() = default;

	virtual bool init() = 0;

	virtual void update_pool_data(pool_data const& pool_data) = 0;

	// round data
	virtual void add_round_data(round_data const& round_data) = 0;
	virtual void flag_round_as_orphan(round_data const& round_data) = 0;

	// account data
	/** Should Insert or Update the account data **/
	virtual void update_account_data(account_data const& account_ata) = 0;
	/** Should set the round shares for all accounts to 0**/
	virtual void clear_account_round_shares() = 0;
	virtual void save_connection_data(connection_data const& connection_data) = 0;

	// account earnings
	/** This maybe called multiple times for a given round (in the case of
	* additional bonuses) **/
	virtual void add_account_earnings(account_earning_transaction const& account_earning_transaction) = 0;
	virtual void delete_account_earnings(account_earning_transaction const& account_earning_transaction) = 0;

	// account payments
	virtual void add_account_payment(account_payment_transaction const& account_earning_transaction) = 0;
	virtual void delete_account_payment(account_payment_transaction const& account_earning_transaction) = 0;
};

} // namespace database
} // namespace nexuspool

#endif

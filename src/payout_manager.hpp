#ifndef NEXUSPOOL_PAYOUT_MANAGER_HPP
#define NEXUSPOOL_PAYOUT_MANAGER_HPP

#include <memory>
#include <string>
#include "hash/uint1024.h"

namespace spdlog { class logger; }

namespace nexuspool
{
	class Data_registry;

	class Payout_manager
	{
	public:

		Payout_manager();

		// Clear the Shares for the Current Round.
		void clear_shares(std::shared_ptr<Data_registry> data_registry) const;
		// Update the Account Balances from Weights in Current Round.
		void update_balances(std::shared_ptr<Data_registry> data_registry, uint64_t reward,
					uint1024 hash_submit_block, std::string const& last_blockfinder) const;

		void refund_payouts(std::shared_ptr<Data_registry> data_registry, uint1024 hash_block) const;


	private:
		// Get the Total Weight of Shares from the Current Round. 
		uint64_t total_weight(std::shared_ptr<Data_registry> data_registry) const;

		std::shared_ptr<spdlog::logger> m_logger;
	};

}


#endif // NEXUSPOOL_PAYOUT_MANAGER_HPP
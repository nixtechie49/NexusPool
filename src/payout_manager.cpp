#include "payout_manager.hpp"
#include "data_registry.hpp"
#include "config.hpp"
#include <spdlog/spdlog.h>

namespace nexuspool
{
	Payout_manager::Payout_manager()
		: m_logger{spdlog::get("logger")}
	{

	}

	uint64_t Payout_manager::total_weight(std::shared_ptr<Data_registry> data_registry) const
	{
		uint64_t weight = 0;

		std::vector<std::string> vKeys = data_registry->m_account_db->GetKeys();
		for (size_t nIndex = 0; nIndex < vKeys.size(); nIndex++)
		{
			weight += data_registry->m_account_db->GetRecord(vKeys[nIndex]).nRoundShares;
		}
		
		return weight;
	}

	/** Clear the Shares for the Current Round. **/
	void Payout_manager::clear_shares(std::shared_ptr<Data_registry> data_registry) const
	{
		std::vector<std::string> vKeys = data_registry->m_account_db->GetKeys();
		for (size_t nIndex = 0; nIndex < vKeys.size(); nIndex++)
		{
			LLD::Account cAccount = data_registry->m_account_db->GetRecord(vKeys[nIndex]);
			cAccount.nRoundShares = 0;
			data_registry->m_account_db->UpdateRecord(cAccount);
		}

	//	STATSCOLLECTOR.ClearAccountRoundShares();
	}

	// Update the Account Balances from Weights in Current Round.
	void Payout_manager::update_balances(std::shared_ptr<Data_registry> data_registry, uint64_t reward, uint1024 hash_submit_block, std::string const& last_blockfinder) const
	{
		m_logger->info("\n---------------------------------------------------\n\n");
		time_t tTime = time(0);

		/** Create a New Block Record to Track Payouts. **/
		LLD::Block cBlock(hash_submit_block);
		cBlock.cCoinbase = data_registry->get_coinbase().get_coinbase();
		cBlock.nCoinbaseValue = cBlock.cCoinbase.nMaxValue + cBlock.cCoinbase.nPoolFee;
		cBlock.nRound = data_registry->m_current_round;


		/** Make Sure Miners Aren't Credited the Pool Fee [2% for Now]. **/
		reward -= (reward * data_registry->get_config().get_pool_fee());

		/** The 2% Block Finder Bonus from Previous Round. **/
		if (last_blockfinder != "")
		{
			uint64_t credit = static_cast<uint64_t>(reward * 0.02);
			LLD::Account cAccount = data_registry->m_account_db->GetRecord(last_blockfinder);
			cAccount.nAccountBalance += credit;
			data_registry->m_account_db->UpdateRecord(cAccount);

			cBlock.cCredits.AddCredit(last_blockfinder, credit);

		//	STATSCOLLECTOR.AddAccountEarnings(last_blockfinder, data_registry->m_current_round, data_registry->m_best_height, 0, credit, tTime);

			m_logger->info("[ACCOUNT] Block Finder Bonus to {0} of {1} NXS\n", last_blockfinder, credit / 1000000.0);

			reward -= credit;
		}


		/** Calculate the Total Weight of the Last Round. **/
		uint64_t nTotalWeight = total_weight(data_registry);
		uint64_t nTotalReward = 0;

		/** Update the Accounts in the Database Handle. **/
		std::vector<std::string> vKeys = data_registry->m_account_db->GetKeys();
		for (size_t nIndex = 0; nIndex < vKeys.size(); nIndex++)
		{
			LLD::Account cAccount = data_registry->m_account_db->GetRecord(vKeys[nIndex]);

			/** Update the Account Balance After Payout was Successful. **/
			if (data_registry->get_coinbase().find_address(vKeys[nIndex]))
			{
				auto map_output = data_registry->get_coinbase().get_map_outputs(vKeys[nIndex]);
				if (map_output > cAccount.nAccountBalance)
					cAccount.nAccountBalance = 0;
				else
					cAccount.nAccountBalance -= map_output;

				data_registry->m_account_db->UpdateRecord(cAccount);

				//save payment stats
			//	STATSCOLLECTOR.AddAccountPayment(vKeys[nIndex], data_registry->m_current_round, data_registry->m_best_height, map_output, tTime);

			}

			/** Don't Credit Account if there are no Shares for this Round. **/
			if (cAccount.nRoundShares == 0)
				continue;

			/** Credit the Account from its Weight. **/
			uint32_t nCredit = static_cast<uint32_t>(((double)cAccount.nRoundShares / nTotalWeight) * reward);

			cAccount.nAccountBalance += nCredit;
			nTotalReward += nCredit;

			/** Update the New Credits in the Block Database. **/
			cBlock.cCredits.AddCredit(vKeys[nIndex], nCredit);

			/** Commit the New Account Record to Database. **/
			data_registry->m_account_db->UpdateRecord(cAccount);

			// save stats
		//	STATSCOLLECTOR.AddAccountEarnings(vKeys[nIndex], data_registry->m_current_round, data_registry->m_best_height, cAccount.nRoundShares, nCredit, tTime);

			m_logger->info("[ACCOUNT] Account: {0} | Credit: {1} NXS | Balance: {3} NXS\n", cAccount.cKey, nCredit / 1000000.0, cAccount.nAccountBalance / 1000000.0);
		}

		/** Any Remainder of the Rewards give to Blockfinder. **/
		if (reward > nTotalReward)
		{
			LLD::Account cAccount = data_registry->m_account_db->GetRecord(last_blockfinder);
			cAccount.nAccountBalance += (reward - nTotalReward);
			data_registry->m_account_db->UpdateRecord(cAccount);

		//	STATSCOLLECTOR.AddAccountEarnings(last_blockfinder, data_registry->m_current_round, data_registry->m_best_height, 0, (reward - nTotalReward), tTime);

			m_logger->info("[ACCOUNT] Block Finder Additional Credit to {0} of {1} NXS\n", last_blockfinder, (reward - nTotalReward) / 1000000.0);
		}

		/** Commit the New Block Record to the Block Database. **/
		data_registry->m_block_db->UpdateRecord(cBlock);
		data_registry->m_block_db->WriteToDisk();
		cBlock.cCredits.Print(m_logger);

		/** Commit the Account Changes to Database. **/
		data_registry->m_account_db->WriteToDisk();

		m_logger->info("[ACCOUNT] Balances Updated. Total Round Weight = {0} | Total Round Reward: {1} NXS\n", nTotalWeight / 1000000.0, reward / 1000000.0);
		m_logger->info("\n---------------------------------------------------\n\n");
	}

	/** Refund the Payouts if the Block was Orphaned. **/
	void Payout_manager::refund_payouts(std::shared_ptr<Data_registry> data_registry, uint1024 hash_block) const
	{
		/** Get a Record from the Database. **/
		LLD::Block cBlock = data_registry->m_block_db->GetRecord(hash_block);

		if (cBlock.nCoinbaseValue == 0)
			return; // already processed

		uint32_t nRoundToRefund = cBlock.nRound;

	//	STATSCOLLECTOR.FlagRoundAsOrphan(nRoundToRefund);

		/** Set the Flag that Block is an Orphan. **/
		cBlock.nCoinbaseValue = 0;

		m_logger->info("\n---------------------------------------------------\n\n");
		/** Clear out the Credits for the Round. **/
		for (std::map<std::string, uint64>::const_iterator nIterator = cBlock.cCredits.mapCredits.begin(); nIterator != cBlock.cCredits.mapCredits.end(); nIterator++)
		{
			LLD::Account cAccount = data_registry->m_account_db->GetRecord(nIterator->first);
			if (nIterator->second > cAccount.nAccountBalance)
				cAccount.nAccountBalance = 0;
			else
				cAccount.nAccountBalance -= nIterator->second;

			m_logger->info("[MASTER] Account {0} Removed {1} Credits\n", nIterator->first, nIterator->second / 1000000.0);
			data_registry->m_account_db->UpdateRecord(cAccount);

//			STATSCOLLECTOR.DeleteAccountEarnings(nIterator->first, nRoundToRefund);
		}

		/** Refund the Payouts for the Round. **/
		for (std::map<std::string, uint64>::const_iterator nIterator = cBlock.cCoinbase.mapOutputs.begin(); nIterator != cBlock.cCoinbase.mapOutputs.end(); nIterator++)
		{
			LLD::Account cAccount = data_registry->m_account_db->GetRecord(nIterator->first);
			cAccount.nAccountBalance += nIterator->second;

			m_logger->info("[MASTER] Account {0} Refunded {1} NXS\n", nIterator->first, nIterator->second / 1000000.0);
			data_registry->m_account_db->UpdateRecord(cAccount);

		//	STATSCOLLECTOR.DeleteAccountPayment(nIterator->first, nRoundToRefund);
		}

		m_logger->info("\n---------------------------------------------------\n\n");

		/** Update the Record into the Database. **/
		data_registry->m_block_db->UpdateRecord(cBlock);
		data_registry->m_block_db->WriteToDisk();
	}
}
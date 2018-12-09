
#ifndef NEXUSPOOL_DATABASE_TYPES_HPP
#define NEXUSPOOL_DATABASE_TYPES_HPP

#include <memory>
#include <string>

namespace nexuspool {
namespace database {

struct pool_data
{
public:
	pool_data()
	{
		nRound = Core::nCurrentRound;
		nBlockNumber = Core::nBestHeight;
		nRoundReward = Core::nRoundReward;
		nTotalShares = Core::TotalWeight();
		nConnectionCount = Core::nConnections;

	}

	uint32_t nRound;
	uint32_t nBlockNumber;
	uint64_t nRoundReward;
	uint64_t nTotalShares;
	uint32_t nConnectionCount;
};

struct account_data
{
	std::string m_account_address;
	int m_connections;
	uint64_t m_shares;
	uint64_t m_balance;
	uint64_t m_pending_payout;
};

struct round_data
{
	explicit round_data(int round_to_set) : m_round{ round_to_set } {}
	round_data()
	{
		round = Core::nCurrentRound;
		nBlockNumber = Core::nBestHeight;
		hashBlock = Core::hashBlockSubmission.ToString();
		nRoundReward = Core::nRoundReward;
		nTotalShares = Core::TotalWeight();
		strBlockFinder = Core::LAST_ROUND_BLOCKFINDER;
		bOrphan = false;
		tBlockFoundTime = time(0);
	}


	uint32_t m_round;
	uint32_t nBlockNumber;
	std::string hashBlock;
	uint64_t nRoundReward;
	uint64_t nTotalShares;
	std::string strBlockFinder;
	bool bOrphan;
	time_t tBlockFoundTime;
};

struct connection_data
{
public:
	connection_data() : m_address{""} {}
	connection_data(std::string address) : m_address{std::move(address)}, LASTSAVETIMER.Start();
}

	double GetTotalPPS() const
	{
		double PPS = 0;
		for (std::map<std::string, ConnectionMinerData >::const_iterator iter = MINER_DATA.begin(); iter != MINER_DATA.end(); ++iter)
		{
			PPS += iter->second.PPS;
		}

		return PPS;
	}
	double GetTotalWPS() const
	{
		double WPS = 0;
		for (std::map<std::string, ConnectionMinerData >::const_iterator iter = MINER_DATA.begin(); iter != MINER_DATA.end(); ++iter)
		{
			WPS += iter->second.WPS;
		}

		return WPS;
	}

	std::string m_address;
	std::map<std::string, ConnectionMinerData > MINER_DATA;

	/** Used to track when this connection data was last persisted**/
	LLP::Timer LASTSAVETIMER;
};

struct account_earning_transaction
{
	std::string m_account_address;
	uint32_t m_round;
	uint32_t m_block_number;
	uint64_t m_shares;
	uint64_t m_amount_earned;
	time_t m_time;

};

struct account_payment_transaction
{
	std::string m_account_address;
	uint32_t m_round;
	uint32_t m_block_number;
	uint32_t m_amount_paid;
	time_t m_time;

};

} // namespace database
} // namespace nexuspool

#endif

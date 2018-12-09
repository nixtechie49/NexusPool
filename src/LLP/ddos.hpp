#ifndef NEXUS_LLP_DDOS_HPP
#define NEXUS_LLP_DDOS_HPP

#include "timer.hpp"

#include <vector>
#include <memory>
#include <algorithm>


namespace LLP {

	/** Class that tracks DDOS attempts on LLP Servers.
		Uses a Timer to calculate Request Score [rScore] and Connection Score [cScore] as a unit of Score / Second.
		Pointer stored by Connection class and Server Listener DDOS_MAP. **/
	class DDOS_Score
	{
		std::vector< std::pair<bool, int> > SCORE;
		Timer TIMER;
		uint64_t nIterator;

		/** Reset the Timer and the Score Flags to be Overwritten. **/
		void Reset()
		{
			for (int i = 0; i < SCORE.size(); i++)
				SCORE[i].first = false;

			TIMER.Reset();
			nIterator = 0;
		}

	public:

		/** Construct a DDOS Score of Moving Average Timespan. **/
		DDOS_Score(int nTimespan)
		{
			for (int i = 0; i < nTimespan; i++)
				SCORE.push_back(std::make_pair(true, 0));

			TIMER.Start();
			nIterator = 0;
		}


		/** Flush the DDOS Score to 0. **/
		void Flush()
		{
			Reset();
			for (int i = 0; i < SCORE.size(); i++)
				SCORE[i].second = 0;
		}


		/** Access the DDOS Score from the Moving Average. **/
		int Score()
		{
			int nMovingAverage = 0;
			for (int i = 0; i < SCORE.size(); i++)
				nMovingAverage += SCORE[i].second;

			return (nMovingAverage / static_cast<int>(SCORE.size()));
		}


		/** Increase the Score by nScore. Operates on the Moving Average to Increment Score per Second. **/
		DDOS_Score & operator+=(const int& nScore)
		{
			auto nTime = TIMER.Elapsed();

			/** If the Time has been greater than Moving Average Timespan, Set to Add Score on Time Overlap. **/
			if (nTime >= SCORE.size())
			{
				Reset();
				nTime = nTime % SCORE.size();
			}

			/** Iterate as many seconds as needed, flagging that each has been iterated. **/
			for (auto i = nIterator; i <= nTime; i++)
			{
				if (!SCORE[i].first)
				{
					SCORE[i].first = true;
					SCORE[i].second = 0;
				}
			}

			/** Update the Moving Average Iterator and Score for that Second Instance. **/
			SCORE[nTime].second += nScore;
			nIterator = nTime;

			return *this;
		}
	};


	/** Filter to Contain DDOS Scores and Handle DDOS Bans. **/
	class DDOS_Filter
	{
		Timer TIMER;
		unsigned int BANTIME, 
			TOTALBANS;

	public:
		DDOS_Score rSCORE, cSCORE;
		std::string IPADDRESS;
		DDOS_Filter(unsigned int nTimespan, std::string strIPAddress) 
			: rSCORE(nTimespan), 
			cSCORE(nTimespan), 
			IPADDRESS(std::move(strIPAddress)),
			BANTIME(0), TOTALBANS(0) 
		{ }

		/** Ban a Connection, and Flush its Scores. **/
		void Ban(std::string strMessage = "Score Threshold Ban")
		{
			if (Banned())
				return;

			TIMER.Start();
			TOTALBANS++;

			uint32_t test1, test2;
			(std::max)(test1, test2);

			BANTIME = (std::max)(TOTALBANS * (rSCORE.Score() + 1) * (cSCORE.Score() + 1), TOTALBANS * 1200u);

			printf("XXXXX DDOS Filter Address = %s cScore = %i rScore = %i Banned for %u Seconds. Violation: %s\n", IPADDRESS.c_str(), cSCORE.Score(), rSCORE.Score(), BANTIME, strMessage.c_str());

			cSCORE.Flush();
			rSCORE.Flush();
		}

		/** Check if Connection is Still Banned. **/
		bool Banned() { return (TIMER.Elapsed() < BANTIME); }
	};

}

#endif
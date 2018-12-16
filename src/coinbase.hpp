#ifndef COINSHIELD_COINBASE_H
#define COINSHIELD_COINBASE_H

#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <utility>

#include "utils.hpp"

namespace nexuspool
{
	/** Class to Create and Manage the Pool Payout Coinbase Tx. **/
	class Coinbase
	{
		/** Add Output to the Transaction. **/
		void AddOutput(std::string nAddress, uint64_t nValue)
		{
			if (!mapOutputs.count(nAddress))
				mapOutputs[nAddress] = 0;

			mapOutputs[nAddress] += nValue;
			nCurrentValue += nValue;
		}

	public:

		/** The Transaction Outputs to be Serialized to Mining LLP. **/
		std::map<std::string, uint64_t> mapOutputs;

		/** The Value of the Coinbase Transaction thus Far. **/
		uint64_t nCurrentValue;

		/** The Value of this current Coinbase Payout. **/
		uint64_t nMaxValue, nPoolFee;


		/** Constructor to Class. **/
		Coinbase() { nMaxValue = 0; nPoolFee = 0; nCurrentValue = 0; }
		Coinbase(uint64_t nReward) { nMaxValue = nReward; nPoolFee = 0; nCurrentValue = 0; }
		Coinbase(std::vector<unsigned char> vData, uint64_t nValue) { Deserialize(vData, nValue); }


		/** Add a Transaction to the Output. Returns Remaining Funds in Coinbase ( - if Too Much). **/
		uint64_t AddTransaction(std::string nAddress, uint64_t nValue)
		{
			if (nValue + nCurrentValue > nMaxValue)
				return (nMaxValue - (nValue + nCurrentValue));

			AddOutput(nAddress, nValue);
			return nMaxValue - nCurrentValue;
		}

		/** Return the Remainding NIRO to complete the Transaction. **/
		uint64_t GetRemainder() { return nMaxValue - nCurrentValue; };


		/** Flag to Know if the Coinbase Tx has been built Successfully. **/
		bool IsComplete() { return nCurrentValue == nMaxValue; }


		/** Flag to know if the Coinbase is Empty. **/
		bool IsEmpty() { return nCurrentValue == 0; }


		/** Convert this Coinbase Transaction into a Byte Stream. **/
		std::vector<unsigned char> Serialize()
		{
			std::vector<unsigned char> vData;
			std::vector<unsigned char> vPoolFee = uint2bytes64(nPoolFee);

			/** First byte of Serialization Packet is the Number of Records. **/
			vData.push_back((unsigned char)mapOutputs.size());
			vData.insert(vData.end(), vPoolFee.begin(), vPoolFee.end());


			for (std::map<std::string, uint64_t>::iterator nIterator = mapOutputs.begin(); nIterator != mapOutputs.end(); nIterator++)
			{
				/** Serialize the Address String and uint64 nValue. **/
				std::vector<unsigned char> vAddress = string2bytes(nIterator->first);
				std::vector<unsigned char> vValue = uint2bytes64(nIterator->second);

				/** Give a 1 Byte Data Length for Each Address in case they differ. **/
				vData.push_back((unsigned char)vAddress.size());

				/** Insert the Address and Coinbase Values. **/
				vData.insert(vData.end(), vAddress.begin(), vAddress.end());
				vData.insert(vData.end(), vValue.begin(), vValue.end());
			}

			return vData;
		}


		/** Deserialize the Coinbase Transaction. **/
		void Deserialize(std::vector<unsigned char> vData, uint64_t nValue)
		{
			/** Set the Max Value for this Transaction. **/
			nMaxValue = nValue;

			/** First byte of Serialization Packet is the Number of Records. **/
			unsigned int nSize = vData[0], nIterator = 9;

			/** Bytes 1 - 8 is the Pool Fee for that Round. **/
			nPoolFee = bytes2uint64(vData, 1);
			nCurrentValue = nPoolFee;

			/** Loop through every Record. **/
			for (unsigned int nIndex = 0; nIndex < nSize; nIndex++)
			{
				/** De-Serialize the Address String and uint64 nValue. **/
				unsigned int nLength = vData[nIterator];

				std::string strAddress = bytes2string(std::vector<unsigned char>(vData.begin() + nIterator + 1, vData.begin() + nIterator + 1 + nLength));
				uint64_t nValue = bytes2uint64(std::vector<unsigned char>(vData.begin() + nIterator + 1 + nLength, vData.begin() + nIterator + 1 + nLength + 8));

				/** Add the Transaction as an Output. **/
				mapOutputs[strAddress] = nValue;

				/** Increment the Iterator. **/
				nIterator += (nLength + 9);

				/** Increment the Current Value. **/
				nCurrentValue += nValue;
			}
		}


		/** Reset the Coinbase Transaction for a new Round. **/
		void Reset(uint64_t nMax)
		{
			nMaxValue = nMax;
			nCurrentValue = 0;
			mapOutputs.clear();
		}


		/** Output the Transactions in the Coinbase Container. **/
		void Print()
		{
			printf("\n\n++++++++++++++++++++++++++++ COINBASE ++++++++++++++++++++++++++++++++\n\n");
			for (std::map<std::string, uint64_t>::iterator nIterator = mapOutputs.begin(); nIterator != mapOutputs.end(); nIterator++)
				printf("Coinbase: %s:%f\n", nIterator->first.c_str(), nIterator->second / 1000000.0);

			printf("\n\nIs Complete: %s\n", IsComplete() ? "TRUE" : "FALSE");
			printf("\n\nMax Value: %llu Current Value: %llu PoolFee: %llu\n", nMaxValue, nCurrentValue, nPoolFee);
			printf("\n\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
		}
	};


	class Credits
	{
	public:
		std::map<std::string, uint64_t> mapCredits;


		/** Constructor to Class. **/
		Credits() {}
		Credits(std::vector<unsigned char> vData) { Deserialize(vData); }


		/** Add a Transaction to the Output. Returns Remaining Funds in Coinbase ( - if Too Much). **/
		void AddCredit(std::string nAddress, uint64_t nValue)
		{
			if (!mapCredits.count(nAddress))
				mapCredits[nAddress] = 0;

			mapCredits[nAddress] += nValue;
		}


		/** Convert this Coinbase Transaction into a Byte Stream. **/
		std::vector<unsigned char> Serialize()
		{
			std::vector<unsigned char> vData;

			/** First byte of Serialization Packet is the Number of Records. **/
			vData.push_back((unsigned char)mapCredits.size());

			for (std::map<std::string, uint64_t>::iterator nIterator = mapCredits.begin(); nIterator != mapCredits.end(); nIterator++)
			{
				/** Serialize the Address String and uint64 nValue. **/
				std::vector<unsigned char> vAddress = string2bytes(nIterator->first);
				std::vector<unsigned char> vValue = uint2bytes64(nIterator->second);

				/** Give a 1 Byte Data Length for Each Address in case they differ. **/
				vData.push_back((unsigned char)vAddress.size());

				/** Insert the Address and Coinbase Values. **/
				vData.insert(vData.end(), vAddress.begin(), vAddress.end());
				vData.insert(vData.end(), vValue.begin(), vValue.end());
			}

			return vData;
		}


		/** Deserialize the Coinbase Transaction. **/
		void Deserialize(std::vector<unsigned char> vData)
		{
			/** First byte of Serialization Packet is the Number of Records. **/
			unsigned int nSize = vData[0], nIterator = 1;

			/** Loop through every Record. **/
			for (unsigned int nIndex = 0; nIndex < nSize; nIndex++)
			{
				/** De-Serialize the Address String and uint64 nValue. **/
				unsigned int nLength = vData[nIterator];

				std::string strAddress = bytes2string(std::vector<unsigned char>(vData.begin() + nIterator + 1, vData.begin() + nIterator + 1 + nLength));
				uint64_t nValue = bytes2uint64(std::vector<unsigned char>(vData.begin() + nIterator + 1 + nLength, vData.begin() + nIterator + 1 + nLength + 8));

				/** Add the Transaction as an Output. **/
				mapCredits[strAddress] = nValue;

				/** Increment the Iterator. **/
				nIterator += (nLength + 9);
			}
		}


		/** Output the Transactions in the Coinbase Container. **/
		void Print()
		{
			printf("\n\n ++++++++++++++++++++++++++++++++ CREDITS ++++++++++++++++++++++++++++++++++++++++++ \n\n");
			for (std::map<std::string, uint64_t>::iterator nIterator = mapCredits.begin(); nIterator != mapCredits.end(); nIterator++)
				printf("Credit: %s:%f\n", nIterator->first.c_str(), nIterator->second / 1000000.0);

			printf("\n\n +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ \n\n");
		}
	};

	// Threadsafe wrapper for Coinbase
	class Coinbase_mt
	{
	public:

		explicit Coinbase_mt(Coinbase& coinbase) : m_coinbase{ coinbase } {}

		/** Reset the Coinbase Transaction for a new Round. **/
		void reset(uint64_t nMax)
		{
			std::lock_guard<std::mutex> lk(m_map_outputs_mutex);
			m_coinbase.Reset(nMax);
		}

		/** Return the Remainding NIRO to complete the Transaction. **/
		uint64_t get_remainder() 
		{
			std::lock_guard<std::mutex> lk(m_map_outputs_mutex);
			return m_coinbase.GetRemainder();
		};

		uint64_t add_transaction(std::string nAddress, uint64_t nValue)
		{
			std::lock_guard<std::mutex> lk(m_map_outputs_mutex);
			return m_coinbase.AddTransaction(std::move(nAddress), nValue);
		}

		std::vector<uint8_t> serialize()
		{
			std::lock_guard<std::mutex> lk(m_map_outputs_mutex);
			return m_coinbase.Serialize();
		}

		bool find_address(std::string const& nxs_address)
		{
			std::lock_guard<std::mutex> lk(m_map_outputs_mutex);
			return (m_coinbase.mapOutputs.find(nxs_address) != m_coinbase.mapOutputs.end());
		}

		uint64_t get_map_outputs(std::string const& nxs_address)
		{
			std::lock_guard<std::mutex> lk(m_map_outputs_mutex);
			return m_coinbase.mapOutputs[nxs_address];
		}

		bool is_complete() 
		{
			std::lock_guard<std::mutex> lk(m_map_outputs_mutex);
			return m_coinbase.IsComplete();
		}

	private:

		Coinbase& m_coinbase;
		std::mutex m_map_outputs_mutex;

	};

}

#endif
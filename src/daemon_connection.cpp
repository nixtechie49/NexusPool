#include "daemon_connection.hpp"
#include "pool_connection.hpp"
#include "LLP/block.h"
#include "LLP/ddos.hpp"
#include "packet.hpp"
#include "base58.h"
#include "utils.hpp"

#include <spdlog/spdlog.h>

namespace nexuspool
{
	Daemon_connection::Daemon_connection(network::Socket::Sptr&& socket)
		: m_socket{std::move(socket)}
		, m_logger{spdlog::get("logger")}
	{
	}
	//void Pool_connection::connection_handler(network::Result::Code result, network::Shared_payload&& receive_buffer)
	//{
	//
	//}

	bool Daemon_connection::connect(network::Endpoint remote_wallet)
	{
		// async
		auto connection = m_socket->connect(std::move(remote_wallet), [self = shared_from_this()](network::Result::Code result, network::Shared_payload&& receive_buffer)
		{
			if (result == network::Result::connection_declined ||
				result == network::Result::connection_aborted ||
				result == network::Result::connection_error)
			{
				self->m_logger->error("Connection to wallet not sucessful. Result: {}", result);
				self->m_connection = nullptr;		// close connection (socket etc)
				// retry connect
			}
			else if (result == network::Result::connection_ok)
			{
				self->m_logger->info("Connection to wallet established");
			}
			else
			{	// data received
				self->process_data(std::move(receive_buffer));
			}

		});

		if(!connection)
		{
			return false;
		}

		m_connection = std::move(connection);
		return true;
	}

	void Daemon_connection::process_data(network::Shared_payload&& receive_buffer)
	{
		if ((*receive_buffer).size() < 5)
		{
			m_logger->error("Received too small buffer from wallet. Size: {}", (*receive_buffer).size());
			return;
		}

		Packet packet{ std::move(receive_buffer) };
		if (!packet.is_valid())
		{
			// log invalid packet
			m_logger->error("Received packet is invalid");
			return;
		}

		///** If a Block was Accepted by the Network, start a new Round. **/
		//if (packet.m_header == GOOD)
		//{
		//	printf("[DAEMON] Block %s Accepted by Niro Network on Handle %u\n", Core::hashBlockSubmission.ToString().substr(0, 20).c_str(), ID);
		//	Core::NewRound();

		//	Core::fSubmittingBlock = false;
		//}
		//else if (packet.m_header == FAIL)
		//{
		//	printf("[DAEMON] Block Rejected by Niro Network on Handle %u\n", ID);
		//	Core::fSubmittingBlock = false;
		//}
		//else if (packet.m_header == COINBASE_SET)
		//{
		//	//printf("[DAEMON] Coinbase Transaction Set on Handle %u\n", ID);

		//	fCoinbasePending = false;
		//}
		//else if (packet.m_header == COINBASE_FAIL)
		//{
		//	printf("[DAEMON] ERROR: Coinbase Transaction Not Set on Handle %u\n", ID);

		//	Core::fNewRound = false;
		//	Core::fSubmittingBlock = false;
		//	Core::fCoinbasePending = true;

		//	fNewBlock = false;
		//	fCoinbasePending = false;
		//}
		///** Add a Block to the Stack if it is received by the Daemon Connection. **/
		//else if (packet.m_header == BLOCK_DATA)
		//{
		//	LLP::CBlock block = deserialize_block(packet.m_data);
		//	if (block->nHeight == Core::nBestHeight)
		//	{
		//		{ LOCK(CONNECTION_MUTEX);
		//		for (int nIndex = 0; nIndex < CONNECTIONS.size(); nIndex++)
		//		{
		//			if (!CONNECTIONS[nIndex])
		//				continue;

		//			if (CONNECTIONS[nIndex]->nBlocksWaiting > 0)
		//			{
		//				CONNECTIONS[nIndex]->AddBlock(BLOCK);

		//				//printf("[DAEMON] Block Received Height = %u on Handle %u Assigned to %u\n", nBestHeight, ID, nIndex);
		//				break;
		//			}
		//		}
		//		}

		//	}
		//	else
		//		printf("[DAEMON] Block Obsolete Height = %u, Skipping over on Handle %u\n", BLOCK->nHeight, ID);
		//}
		//
		///** Don't Request Anything if Waiting for New Round to Reset. **/
		//if (Core::fCoinbasePending || Core::fSubmittingBlock || Core::fNewRound || fCoinbasePending)
		//	continue;


		///** Reset The Daemon if there is a New Block. **/
		//if (fNewBlock)
		//{
		//	fNewBlock = false;

		//	NewBlock();
		//	//printf("[DAEMON] Niro Network: New Block | Reset Daemon Handle %u\n", ID);

		//	/** Set the new Coinbase Transaction. **/
		//	if (Core::nCurrentRound > 1)
		//	{
		//		fCoinbasePending = true;
		//		CLIENT->SetCoinbase();

		//		continue;
		//	}
		//}


		//{ LOCK(CONNECTION_MUTEX);
		//for (int nIndex = 0; nIndex < CONNECTIONS.size(); nIndex++)
		//{
		//	if (!CONNECTIONS[nIndex])
		//		continue;

		//	/** Submit a block from Pool Connection if there is one available. **/
		//	{ LOCK(CONNECTIONS[nIndex]->SUBMISSION_MUTEX);
		//	if (CONNECTIONS[nIndex]->SUBMISSION_BLOCK)
		//	{
		//		Core::fSubmittingBlock = true;
		//		Core::hashBlockSubmission = CONNECTIONS[nIndex]->SUBMISSION_BLOCK->GetHash();


		//		/** Submit the Block to Network if it is Valid. **/
		//		if (CONNECTIONS[nIndex]->SUBMISSION_BLOCK->nHeight == Core::nBestHeight)
		//		{
		//			CLIENT->SubmitBlock(CONNECTIONS[nIndex]->SUBMISSION_BLOCK->hashMerkleRoot, CONNECTIONS[nIndex]->SUBMISSION_BLOCK->nNonce);

		//			Core::LAST_BLOCK_FOUND_TIMER.Reset();
		//			Core::LAST_ROUND_BLOCKFINDER = CONNECTIONS[nIndex]->ADDRESS;
		//			Core::nLastBlockFound = CONNECTIONS[nIndex]->SUBMISSION_BLOCK->nHeight;

		//			printf("[DAEMON] Submitting Block for Address %s on Handle %u\n", CONNECTIONS[nIndex]->ADDRESS.c_str(), ID);
		//		}
		//		else
		//			printf("[DAEMON] Stale Block Submitted on Handle %u\n", ID);


		//		/** Reset the Submission Block Pointer. **/
		//		CONNECTIONS[nIndex]->SUBMISSION_BLOCK = NULL;
		//		break;
		//	}
		//	}

		//	/** Get Blocks if Requested From Pool Connection. **/
		//	{ LOCK(CONNECTIONS[nIndex]->BLOCK_MUTEX);
		//	while (CONNECTIONS[nIndex]->nBlockRequests > 0)
		//	{
		//		CONNECTIONS[nIndex]->nBlockRequests--;
		//		CONNECTIONS[nIndex]->nBlocksWaiting++;

		//		CLIENT->GetBlock();
		//		//CLIENT->GetBlock();

		//		//printf("[DAEMON] Requesting Block from Daemon Handle %u\n", ID);
		//	}
		//	}
		//}
		//}
	}


	/** Convert the Header of a Block into a Byte Stream for Reading and Writing Across Sockets. **/
	LLP::CBlock Daemon_connection::deserialize_block(network::Shared_payload data)
	{
		LLP::CBlock block;
		block.nVersion = bytes2uint(std::vector<uint8_t>(data->begin(), data->begin() + 4));

		block.hashPrevBlock.SetBytes(std::vector<uint8_t>(data->begin() + 4, data->begin() + 132));
		block.hashMerkleRoot.SetBytes(std::vector<uint8_t>(data->begin() + 132, data->end() - 20));

		block.nChannel = bytes2uint(std::vector<uint8_t>(data->end() - 20, data->end() - 16));
		block.nHeight = bytes2uint(std::vector<uint8_t>(data->end() - 16, data->end() - 12));
		block.nBits = bytes2uint(std::vector<uint8_t>(data->end() - 12, data->end() - 8));
		block.nNonce = bytes2uint64(std::vector<uint8_t>(data->end() - 8, data->end()));

		return block;
	}
}
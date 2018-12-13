#ifndef NEXUSPOOL_DAEMON_PACKET_HPP
#define NEXUSPOOL_DAEMON_PACKET_HPP

#include "packet.hpp"
#include "../hash/uint1024.h"

#include <memory>

namespace nexuspool
{
	// Creates packets for daemon connection
	class Daemon_packet
	{
	public:

		/** Enumeration to interpret Daemon Packets. **/
		enum
		{
			/** DATA PACKETS **/
			BLOCK_DATA = 0,
			SUBMIT_BLOCK = 1,
			BLOCK_HEIGHT = 2,
			SET_CHANNEL = 3,
			BLOCK_REWARD = 4,
			SET_COINBASE = 5,
			GOOD_BLOCK = 6,
			ORPHAN_BLOCK = 7,


			/** DATA REQUESTS **/
			CHECK_BLOCK = 64,



			/** REQUEST PACKETS **/
			GET_BLOCK = 129,
			GET_HEIGHT = 130,
			GET_REWARD = 131,


			/** SERVER COMMANDS **/
			CLEAR_MAP = 132,
			GET_ROUND = 133,


			/** RESPONSE PACKETS **/
			GOOD = 200,
			FAIL = 201,


			/** VALIDATION RESPONSES **/
			COINBASE_SET = 202,
			COINBASE_FAIL = 203,

			NEW_ROUND = 204,
			OLD_ROUND = 205,


			/** GENERIC **/
			PING = 253,
			CLOSE = 254
		};

		// Get a new Block from the Daemon Server
		inline Packet get_block() const
		{
			Packet packet;
			packet.m_header = GET_BLOCK;

			return packet;
		}

		// Check the current Chain Height from the Daemon Server
		inline Packet get_height() const
		{ 
			Packet packet;
			packet.m_header = GET_HEIGHT;

			return packet;
		}

		// Check the current Chain Height from the Daemon Server
		inline Packet get_reward() const
		{ 
			Packet packet;
			packet.m_header = GET_REWARD;

			return packet;
		}

		// Check if it is a new Round
		inline Packet get_round() const
		{
			Packet packet;
			packet.m_header = GET_ROUND;

			return packet;
		}


		// Clear the Blocks Map from the Server 
		inline Packet clear_maps() const
		{
			Packet packet;
			packet.m_header = CLEAR_MAP;

			return packet;
		}

		inline Packet ping() const
		{
			Packet packet;
			packet.m_header = PING;

			return packet;
		}

		// Set the Channel that will be Mined For
		Packet set_channel(uint32_t channel) const;


		// Submit a new block to the Daemon Server
		Packet submit_block(uint512 hash_merkle_root, uint64_t nonce) const;


		/** Send the Completed Coinbase Transaction to Server. **/
		Packet set_coinbase(std::vector<uint8_t> const& coinbase) const;


		/** Get a new Block from the Daemon Server. **/
		Packet check_block(uint1024 hash_block) const;
	};
}

#endif // NEXUSPOOL_DAEMON_PACKET_HPP
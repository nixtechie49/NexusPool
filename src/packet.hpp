#ifndef NEXUS_NEXUSPOOL_PACKET_HPP
#define NEXUS_NEXUSPOOL_PACKET_HPP

#include <vector>
#include <cstdint>
#include "network/types.hpp"

namespace nexuspool
{
    /** Class to handle sending and receiving of LLP Packets. **/
    class Packet
    {
    public:

		enum
		{
			/** DATA PACKETS **/
			LOGIN = 0,
			BLOCK_DATA = 1,
			SUBMIT_SHARE = 2,
			ACCOUNT_BALANCE = 3,
			PENDING_PAYOUT = 4,
			SUBMIT_PPS = 5,

			/** REQUEST PACKETS **/
			GET_BLOCK = 129,
			NEW_BLOCK = 130,
			GET_BALANCE = 131,
			GET_PAYOUT = 132,


			/** RESPONSE PACKETS **/
			ACCEPT = 200,
			REJECT = 201,
			BLOCK = 202,
			STALE = 203,


			/** GENERIC **/
			PING = 253,
			CLOSE = 254
		};

        Packet() : m_header{255}, m_length{0} 
		{
		}
		// creates a packet from received buffer
		explicit Packet(network::Shared_payload buffer)
		{
			uint8_t m_header = (*buffer)[0];
			int32_t m_length = ((*buffer)[1] << 24) + ((*buffer)[2] << 16) + ((*buffer)[3] << 8) + ((*buffer)[4]);

			m_data = std::make_shared<std::vector<uint8_t>>(*buffer);
		}

        /** Components of an LLP Packet.
            BYTE 0       : Header
            BYTE 1 - 5   : Length
            BYTE 6 - End : Data      **/
        uint8_t			m_header;
        uint32_t		m_length;
        network::Shared_payload m_data;

		inline bool is_valid() const
		{
			if ((*m_data).size() < 5)	// header (uint8_t) + length (int)
			{
				return false;
			}

			return ((m_header < 128 && m_length > 0) || (m_header >= 128 && m_header < 255 && m_length == 0));
		}


    };

}

#endif

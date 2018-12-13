#include "daemon_packet.hpp"
#include "utils.hpp"

namespace nexuspool
{ 
	Packet Daemon_packet::set_channel(uint32_t channel) const
	{
		Packet packet;
		packet.m_header = SET_CHANNEL;
		packet.m_length = 4;
		packet.m_data = std::make_shared<std::vector<uint8_t>>(uint2bytes(channel));

		return packet;
	}

	Packet Daemon_packet::set_coinbase(std::vector<uint8_t> const& coinbase) const
	{
		Packet packet;
		packet.m_header = SET_COINBASE;		
		packet.m_data = std::make_shared<std::vector<uint8_t>>(coinbase);
		packet.m_length = packet.m_data->size();

		return packet;
	}

	Packet Daemon_packet::check_block(uint1024 hash_block) const
	{
		Packet packet;
		packet.m_header = CHECK_BLOCK;
		packet.m_data = std::make_shared<std::vector<uint8_t>>(hash_block.GetBytes());
		packet.m_length = packet.m_data->size();

		return packet;
	}

	Packet Daemon_packet::submit_block(uint512 hash_merkle_root, uint64_t nonce) const
	{
		Packet packet;
		packet.m_header = SUBMIT_BLOCK;
		packet.m_length = 72;
		packet.m_data = std::make_shared<std::vector<uint8_t>>(hash_merkle_root.GetBytes());
		std::vector<uint8_t> NONCE = uint2bytes64(nonce);
		packet.m_data->insert(packet.m_data->end(), NONCE.begin(), NONCE.end());

		return packet;
	}
}
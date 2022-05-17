#pragma once

#pragma pack(1)
struct stHeader{
	unsigned char code;
	unsigned short size;
	unsigned char randKey;
	unsigned char checkSum;
};
#pragma pack()


//#define PACKET_PTR_NET_DEBUG

namespace netServer {

	constexpr int MAX_PACKET = 100;

	constexpr unsigned __int64 SESSION_INDEX_MASK       = 0x000000000000FFFF;
	constexpr unsigned __int64 SESSION_ALLOC_COUNT_MASK = 0xFFFFFFFFFFFF0000;

	constexpr unsigned char PACKET_KEY = 0x32;

	constexpr unsigned char PACKET_CODE = 0x77;

	constexpr unsigned short PACKET_MAX_SIZE = 400;

	constexpr unsigned __int64 MTU_SIZE = 1500;
	constexpr unsigned __int64 MSS_SIZE = 1460;
	constexpr unsigned __int64 PACKET_HEADER_SIZE = 40;

};
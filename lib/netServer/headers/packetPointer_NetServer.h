#pragma once

#include "packetPointer/headers/packetPointer.h"

#include "common.h"

class CPacketPtr_Net: public CPacketPointer{
public:

	CPacketPtr_Net();
	CPacketPtr_Net(CPacketPtr_Net& ptr);
	CPacketPtr_Net(CPacketPointer& ptr);

	virtual void setHeader();
	virtual void incoding();
	virtual void decoding();

private:

	#if defined(PACKET_PTR_NET_DEBUG)
		void* returnAdr;
	#endif

};

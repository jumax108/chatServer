
#include "objectFreeListTLS/headers/objectFreeListTLS.h"
#include "headers/packetPointer_NetServer.h"

void CPacketPtr_Net::setHeader() {

	CProtocolBuffer* protocolBuffer = &_packet->_buffer;

	stHeader* header = (stHeader*)protocolBuffer->getBufStart();
	
	if (_packet->_incoded == true) {
		return;
	}

	header->code = netServer::PACKET_CODE;
	header->size = protocolBuffer->getUsedSize() - sizeof(stHeader);
	header->randKey = rand() % 0xFF;

	unsigned char checkSum = 0;
	int size = header->size;
	unsigned char* buffer = (unsigned char*)header;
	buffer += sizeof(stHeader);
	for (int cnt = 0; cnt < size; cnt++) {
		checkSum += buffer[cnt];
	}

	header->checkSum = checkSum;
} 

void CPacketPtr_Net::incoding() {

	if (this->_packet->_incoded == true) {
		return;
	}
	this->_packet->_incoded = true;

	unsigned char d = 0;
	unsigned char p = 0;
	unsigned char e = 0;

	CProtocolBuffer* protocolBuffer = &_packet->_buffer;

	stHeader* header = (stHeader*)protocolBuffer->getBufStart();
	int size = header->size + 1;

	unsigned char* buffer = (unsigned char*)header;
	buffer += sizeof(stHeader) - 1;

	unsigned char randKey = header->randKey;

	for (int idx = 0; idx < size; ++idx) {

		d = buffer[idx];
		p = d ^ (randKey + idx + 1 + p);
		e = p ^ (netServer::PACKET_KEY + idx + 1 + e);
		buffer[idx] = e;
	}


}

void CPacketPtr_Net::decoding() {

	unsigned char d = 0;
	unsigned char p = 0;
	unsigned char e = 0;

	CProtocolBuffer* protocolBuffer = &_packet->_buffer;

	stHeader* header = (stHeader*)protocolBuffer->getBufStart();
	int size = header->size + 1;

	unsigned char* buffer = (unsigned char*)header;
	buffer += sizeof(stHeader) - 1;

	unsigned char randKey = header->randKey;

	for (int idx = size - 1; idx >= 1; idx--) {

		buffer[idx] ^= buffer[idx - 1] + idx + 1 + netServer::PACKET_KEY;

	}
	buffer[0] ^= netServer::PACKET_KEY + 1;

	for (int idx = size - 1; idx >= 1; idx--) {
		buffer[idx] ^= buffer[idx - 1] + idx + 1 + randKey;
	}
	buffer[0] ^= randKey + 1;

}



 CPacketPtr_Net::CPacketPtr_Net() {

	this->_packet->_buffer.moveRear(sizeof(stHeader));

	#if defined(PACKET_PTR_NET_DEBUG)
		returnAdr = _ReturnAddress();

	#endif
}

CPacketPtr_Net::CPacketPtr_Net(CPacketPtr_Net& ptr)
	:CPacketPointer(ptr){

	#if defined(PACKET_PTR_NET_DEBUG)
		returnAdr = ptr.returnAdr;

	#endif
}

CPacketPtr_Net::CPacketPtr_Net(CPacketPointer& ptr)
	:CPacketPointer(ptr) {

#if defined(PACKET_PTR_NET_DEBUG)
	returnAdr = ptr._packet->returnAdr;

#endif

}


#include "objectFreeListTLS/headers/objectFreeListTLS.h"

#include "headers/packetPointer.h"

CObjectFreeListTLS<stPacket> CPacketPointer::
	_freeList(false, false);

CPacketPointer::CPacketPointer(){
	_packet = _freeList.allocObject();
	_packet->_ref = 1;

	#if defined(PACKET_PTR_DEBUG)
		_packet->returnAdr = _ReturnAddress();
		_packet->_packetPtr = this;
	#endif
}

CPacketPointer::CPacketPointer(CPacketPointer& ptr){
	
	_packet = ptr._packet;

	#ifdef PACKET_PTR_REF_AUTO
		InterlockedIncrement((long*)&_packet->_ref);
	#endif
}
CPacketPointer& CPacketPointer::operator=(CPacketPointer& ptr){

	// 갖고 있는 데이터가 있으면 ref 감소
	#ifdef PACKET_PTR_REF_AUTO
		if(_packet != nullptr){
			this->~CPacketPointer();
		}
	#endif

	_packet = ptr._packet;

	#ifdef PACKET_PTR_REF_AUTO
		InterlockedIncrement((long*)&_packet->_ref);
	#endif

	return *this;
}

CPacketPointer::~CPacketPointer(){
	
	#ifndef PACKET_PTR_REF_AUTO
		return ;
	#endif
	
	int ref = InterlockedDecrement((LONG*)&_packet->_ref);
	if(ref == 0){
		_packet->_buffer.clear();
		_packet->_incoded = false;
		_freeList.freeObject(_packet);
		_packet = nullptr;
	}

}

void CPacketPointer::incRef(){

	#ifdef PACKET_PTR_REF_AUTO
		return ;
	#endif
	
	InterlockedIncrement((LONG*)&_packet->_ref);
}

void CPacketPointer::decRef(){
	
	#ifdef PACKET_PTR_REF_AUTO
		return ;
	#endif
	
	int ref = InterlockedDecrement((LONG*)&_packet->_ref);
	if(ref == 0){
		_packet->_buffer.clear();
		_packet->_incoded = false;
		_freeList.freeObject(_packet);
		_packet = nullptr;
	}

}

bool CPacketPointer::checkBufferSize(int size){

	if(_packet->_buffer.getFreeSize() < size){
		return false;
	}

	return true;

}

unsigned __int64 CPacketPointer::getPacketPoolUsage(){

	return CPacketPointer::_freeList.getCapacity();

}

void CPacketPointer::popData(int size, unsigned char* buf){

	_packet->_buffer.popData(size, buf);

}

void CPacketPointer::putData(int size, unsigned char* buf){
	
	_packet->_buffer.putData(size, (char*)buf);

}

CPacketPointer& CPacketPointer::operator<<(char data){
	_packet->_buffer << data;
	return *this;
}

CPacketPointer& CPacketPointer::operator<<(unsigned char data){
	_packet->_buffer << data;
	return *this;
}

CPacketPointer& CPacketPointer::operator<<(wchar_t data){
	_packet->_buffer << data;
	return *this;
}

CPacketPointer& CPacketPointer::operator<<(short data){
	_packet->_buffer << data;
	return *this;
}

CPacketPointer& CPacketPointer::operator<<(unsigned short data){
	_packet->_buffer << data;
	return *this;
}

CPacketPointer& CPacketPointer::operator<<(int data){
	_packet->_buffer << data;
	return *this;
}

CPacketPointer& CPacketPointer::operator<<(unsigned int data){
	_packet->_buffer << data;
	return *this;
}

CPacketPointer& CPacketPointer::operator<<(__int64 data){
	_packet->_buffer << data;
	return *this;
}

CPacketPointer& CPacketPointer::operator<<(unsigned __int64 data){
	_packet->_buffer << data;
	return *this;
}

CPacketPointer& CPacketPointer::operator<<(float data){
	_packet->_buffer << data;
	return *this;
}

CPacketPointer& CPacketPointer::operator<<(double data){
	_packet->_buffer << data;
	return *this;
}

CPacketPointer& CPacketPointer::operator>>(char& data){
	_packet->_buffer >> data;
	return *this;
}

CPacketPointer& CPacketPointer::operator>>(unsigned char& data){
	_packet->_buffer >> data;
	return *this;
}

CPacketPointer& CPacketPointer::operator>>(wchar_t& data){
	_packet->_buffer >> data;
	return *this;
}

CPacketPointer& CPacketPointer::operator>>(short& data){
	_packet->_buffer >> data;
	return *this;
}

CPacketPointer& CPacketPointer::operator>>(unsigned short& data){
	_packet->_buffer >> data;
	return *this;
}

CPacketPointer& CPacketPointer::operator>>(int& data){
	_packet->_buffer >> data;
	return *this;
}

CPacketPointer& CPacketPointer::operator>>(unsigned int& data){
	_packet->_buffer >> data;
	return *this;
}

CPacketPointer& CPacketPointer::operator>>(__int64& data){
	_packet->_buffer >> data;
	return *this;
}

CPacketPointer& CPacketPointer::operator>>(unsigned __int64& data){
	_packet->_buffer >> data;
	return *this;
}

CPacketPointer& CPacketPointer::operator>>(float& data){
	_packet->_buffer >> data;
	return *this;
}

CPacketPointer& CPacketPointer::operator>>(double& data){
	_packet->_buffer >> data;
	return *this;
}
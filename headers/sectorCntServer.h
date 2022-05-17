#pragma once

#include <list>
#include <vector>
#include <unordered_map>

#include "lanServer/headers/lanServer.h"

#include "common.h"

class CSectorCntServer : public CLanServer {

public:

	CSectorCntServer(DWORD delay) {
		_clientID = 0;
		_delay = delay;


	}

	void init() {

		_updateThraed = (HANDLE)_beginthreadex(nullptr, 0, updateFunc, this, 0, nullptr);
	}

	// 클라이언트가 접속을 시도한 상태에서 호출됩니다.
// 반환된 값에 따라 연결을 허용합니다.
// return true = 연결 후, 세션 초기화
// return false = 연결을 끊음
	virtual bool onConnectRequest(unsigned int ip, unsigned short port) {
		return true;
	}
	// 클라이언트가 접속을 완료한 상태에서 호출됩니다.
	virtual void onClientJoin(unsigned int ip, unsigned short port, unsigned __int64 sessionID) {
		_clientID = sessionID;
		printf("VIWER CONNECT\n");
		printf("VIWER CONNECT\n");
		printf("VIWER CONNECT\n");
		printf("VIWER CONNECT\n");
		printf("VIWER CONNECT\n");
		printf("VIWER CONNECT\n");
		printf("VIWER CONNECT\n");
	}
	// 클라이언트의 연결이 해제되면 호출됩니다.
	virtual void onClientLeave(unsigned __int64 sessionID) {
		_clientID = 0;
	}

	// 클라이언트에게 데이터를 전송하면 호출됩니다.
	virtual void onRecv(unsigned __int64 sessionID, CPacketPointer pakcet) {
	}
	// 클라이언트에게서 데이터를 전달받으면 호출됩니다.
	virtual void onSend(unsigned __int64 sessionID, int sendSize) {
		printf("send packet\n");
		printf("send packet\n");
		printf("send packet\n");
		printf("send packet\n");
		printf("send packet\n");
		printf("send packet\n");
		printf("send packet\n");
		printf("send packet\n");
		printf("send packet\n");
	}

	// 에러 상황에서 호출됩니다.
	virtual void onError(int errorCode, const wchar_t* errorMsg) {
	}

	void setUserList(int y, int x, std::list<stUser*>* userList) {
		_sectorUserList[y][x] = userList;
	}

	static unsigned __stdcall updateFunc(void*);

private:

	unsigned __int64 _clientID;

	DWORD _delay;

	HANDLE _updateThraed;

	std::list<stUser*>* _sectorUserList[SECTOR_SIZE][SECTOR_SIZE];

};
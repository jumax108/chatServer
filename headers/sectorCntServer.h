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

	// Ŭ���̾�Ʈ�� ������ �õ��� ���¿��� ȣ��˴ϴ�.
// ��ȯ�� ���� ���� ������ ����մϴ�.
// return true = ���� ��, ���� �ʱ�ȭ
// return false = ������ ����
	virtual bool onConnectRequest(unsigned int ip, unsigned short port) {
		return true;
	}
	// Ŭ���̾�Ʈ�� ������ �Ϸ��� ���¿��� ȣ��˴ϴ�.
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
	// Ŭ���̾�Ʈ�� ������ �����Ǹ� ȣ��˴ϴ�.
	virtual void onClientLeave(unsigned __int64 sessionID) {
		_clientID = 0;
	}

	// Ŭ���̾�Ʈ���� �����͸� �����ϸ� ȣ��˴ϴ�.
	virtual void onRecv(unsigned __int64 sessionID, CPacketPointer pakcet) {
	}
	// Ŭ���̾�Ʈ���Լ� �����͸� ���޹����� ȣ��˴ϴ�.
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

	// ���� ��Ȳ���� ȣ��˴ϴ�.
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
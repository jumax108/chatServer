#pragma once

#include <list>
#include <vector>
#include <unordered_map>

#include "netServer/headers/netServer.h"
#pragma comment(lib, "lib/netServer/netServer")

#include "common.h"


class CChatServer : public CNetServer {

public:

	CChatServer();
	static unsigned __stdcall updateFunc(void* args);

private:

	// Ŭ���̾�Ʈ�� ������ �õ��� ���¿��� ȣ��˴ϴ�.
	// ��ȯ�� ���� ���� ������ ����մϴ�.
	// return true = ���� ��, ���� �ʱ�ȭ
	// return false = ������ ����
	virtual bool onConnectRequest(unsigned int ip, unsigned short port);
	// Ŭ���̾�Ʈ�� ������ �Ϸ��� ���¿��� ȣ��˴ϴ�.
	virtual void onClientJoin(unsigned int ip, unsigned short port, unsigned __int64 sessionID) ;
	// Ŭ���̾�Ʈ�� ������ �����Ǹ� ȣ��˴ϴ�.
	virtual void onClientLeave(unsigned __int64 sessionID);

	// Ŭ���̾�Ʈ���� �����͸� �����ϸ� ȣ��˴ϴ�.
	virtual void onRecv(unsigned __int64 sessionID, CPacketPointer pakcet);
	// Ŭ���̾�Ʈ���Լ� �����͸� ���޹����� ȣ��˴ϴ�.
	virtual void onSend(unsigned __int64 sessionID, int sendSize);

	// ���� ��Ȳ���� ȣ��˴ϴ�.
	virtual void onError(int errorCode, const wchar_t* errorMsg);

	CLockFreeQueue<stJob> _jobQueue;
	HANDLE _jobEvent;

	HANDLE _updateThread;				

	std::unordered_map<unsigned __int64, stUser*> _userTable;

	stSector _sectorArr[SECTOR_SIZE][SECTOR_SIZE];

};
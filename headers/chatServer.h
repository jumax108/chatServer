#pragma once

#include <list>
#include <vector>
#include <unordered_map>

#include "sectorCntServer.h"

#include "netServer/headers/netServer.h"

#include "objectFreeList_stJob.h"
#include "common.h"



class CChatServer : public CNetServer {

public:

	CChatServer();
	
	void init();

	static unsigned __stdcall updateFunc(void* args);

	void stop();

	int getJobCnt() {
		return _jobFreeList.getUsedCount();
	}

	unsigned __int64 _loginMsgCnt;
	unsigned __int64 _sectorMoveMsgCnt;
	unsigned __int64 _chatMsgCnt;

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
	virtual void onRecv(unsigned __int64 sessionID, CPacketPtr_Net pakcet);
	// Ŭ���̾�Ʈ���Լ� �����͸� ���޹����� ȣ��˴ϴ�.
	virtual void onSend(unsigned __int64 sessionID, int sendSize);

	// ���� ��Ȳ���� ȣ��˴ϴ�.
	virtual void onError(int errorCode, const wchar_t* errorMsg);

	CObjectFreeList<stJob> _jobFreeList;
	CLockFreeQueue<stJob*> _jobQueue;
	HANDLE _jobEvent;

	HANDLE _updateThread;				

	std::unordered_map<unsigned __int64, stUser*> _userTable;

	stSector _sectorArr[SECTOR_SIZE][SECTOR_SIZE];

	bool _stop;
	HANDLE _stopEvent;

	CSectorCntServer* _sectorCntServer;

};



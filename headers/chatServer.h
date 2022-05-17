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

	// 클라이언트가 접속을 시도한 상태에서 호출됩니다.
	// 반환된 값에 따라 연결을 허용합니다.
	// return true = 연결 후, 세션 초기화
	// return false = 연결을 끊음
	virtual bool onConnectRequest(unsigned int ip, unsigned short port);
	// 클라이언트가 접속을 완료한 상태에서 호출됩니다.
	virtual void onClientJoin(unsigned int ip, unsigned short port, unsigned __int64 sessionID) ;
	// 클라이언트의 연결이 해제되면 호출됩니다.
	virtual void onClientLeave(unsigned __int64 sessionID);

	// 클라이언트에게 데이터를 전송하면 호출됩니다.
	virtual void onRecv(unsigned __int64 sessionID, CPacketPtr_Net pakcet);
	// 클라이언트에게서 데이터를 전달받으면 호출됩니다.
	virtual void onSend(unsigned __int64 sessionID, int sendSize);

	// 에러 상황에서 호출됩니다.
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



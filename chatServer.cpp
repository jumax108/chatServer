#include "headers/chatServer.h"


template <>
stJob* CObjectFreeList<stJob>::_allocObject(const wchar_t* fileName, int line) {

	InterlockedIncrement((UINT*)&_usedCnt);

	stAllocNode<stJob>* allocNode;

	void* freePtr;
	void* nextPtr;

	unsigned __int64 nodeChangeCnt;

	_nodeChangeCnt += 1;

	do {
		// 원본 데이터 복사
		freePtr = (void*)_freePtr;
		nodeChangeCnt = _nodeChangeCnt;

		allocNode = ((stAllocNode<stJob>*)((unsigned __int64)freePtr & 0x000007FFFFFFFFFF));

		if (allocNode == nullptr) {

			// 추가 할당
			allocNode = (stAllocNode<stJob>*)malloc(sizeof(stAllocNode<stJob>));
			//ZeroMemory(allocNode, sizeof(stAllocNode<stJob>));
				//HeapAlloc(_heap, 0, sizeof(stAllocNode<stJob>));
			allocNode->init();

#if defined(OBJECT_FREE_LIST_DEBUG)
			// 전체 alloc list에 추가
			// 소멸자에서 일괄적으로 메모리 해제하기 위함
			stSimpleListNode* totalAllocNode = (stSimpleListNode*)malloc(sizeof(stSimpleListNode));
				//HeapAlloc(_heap, 0, sizeof(stSimpleListNode));
			stSimpleListNode* totalAllocList;

			do {

				totalAllocList = (stSimpleListNode*)_totalAllocList;

				totalAllocNode->_ptr = allocNode;
				totalAllocNode->_next = totalAllocList;

			} while (InterlockedCompareExchange64((LONG64*)&_totalAllocList, (LONG64)totalAllocNode, (LONG64)totalAllocList) != (LONG64)totalAllocList);
#endif
			_capacity += 1;

			break;

		}

		nextPtr = allocNode->_nextPtr;

	} while (InterlockedCompareExchange64((LONG64*)&_freePtr, (LONG64)nextPtr, (LONG64)freePtr) != (LONG64)freePtr);

	// 소멸자 호출 여부 초기화
	allocNode->_callDestructor = false;

#if defined(OBJECT_FREE_LIST_DEBUG)
	// 노드를 사용중으로 체크함
	allocNode->_allocated = true;

	// 할당 요청한 소스파일과 소스라인을 기록함
	allocNode->_allocSourceFileName = fileName;
	allocNode->_allocLine = line;
#endif

	stJob* data = &allocNode->_data;


	return data;
}

CChatServer::CChatServer():
	_jobFreeList(false, false)
	 {

	_loginMsgCnt = 0;
	_sectorMoveMsgCnt = 0;
	_chatMsgCnt = 0;

	for (int y = 0; y < SECTOR_SIZE; y++) {
		for (int x = 0; x < SECTOR_SIZE; ++x) {

			std::vector<stSector*>* sendRange = &_sectorArr[y][x]._sendRange;

			for (int dy = -1; dy <= 1; ++dy) {
				int ty = dy + y;

				if (ty < 0 || ty == SECTOR_SIZE) {
					continue;
				}

				for (int dx = -1; dx <= 1; ++dx) {
					int tx = dx + x;

					if (tx < 0 || tx == SECTOR_SIZE) {
						continue;
					}

					sendRange->push_back(&_sectorArr[ty][tx]);

				}
			}

		}
	}


	_jobEvent = CreateEvent(nullptr, false, false, nullptr);
	_stopEvent = CreateEvent(nullptr, true, false, nullptr);

	_updateThread = (HANDLE)_beginthreadex(nullptr, 0, CChatServer::updateFunc, this, 0, nullptr);

}

void CChatServer::onError(int errorCode, const wchar_t* errorMsg) {

	CDump::crash();


	switch (errorCode) {
	case 20000:
		break;
	case 30000: {
		unsigned __int64 sessionID = (unsigned __int64)errorMsg;
		stUser* user = _userTable.find(sessionID)->second;// ->_willDisconnect = true;
		user->_willDisconnect = true;
		disconnect(sessionID);
	}
		break;

	}

}

bool CChatServer::onConnectRequest(unsigned int ip, unsigned short port) {

	return !_stop;
}

void CChatServer::onClientJoin(unsigned int ip, unsigned short port, unsigned __int64 sessionID) {

	if (_stop == true) {
		return;
	}

	stJob* job = _jobFreeList.allocObject();

	job->_code = JOB::USER_ENTER;
	job->_sessionID = sessionID;

	_jobQueue.push(job);
	SetEvent(_jobEvent);

}

void CChatServer::onClientLeave(unsigned __int64 sessionID) {
		
	if (_stop == true) {
		return;
	}

	stJob* job = _jobFreeList.allocObject();

	job->_code = JOB::USER_LEAVE;
	job->_sessionID = sessionID;

	_jobQueue.push(job);
	SetEvent(_jobEvent);

}

void CChatServer::onRecv(unsigned __int64 sessionID, CPacketPtr_Net  packet) {

	if (_stop == true) {
		return;
	}

	packet.incRef();

	WORD type;
	packet >> type;

	stJob* job = _jobFreeList.allocObject();

	job->_code = (JOB)type;
	job->_sessionID = sessionID;
	job->_packet = packet;

	_jobQueue.push(job);

	SetEvent(_jobEvent);

}

void CChatServer::onSend(unsigned __int64 sessionID, int sendSize) {

}

unsigned __stdcall CChatServer::updateFunc(void* args) {

	CChatServer* server = (CChatServer*)args;

	HANDLE events[2] = { server->_stopEvent,server->_jobEvent };

	CLockFreeQueue<stJob*>* jobQueue = &server->_jobQueue;
	CObjectFreeList<stJob>* jobFreeList = &server->_jobFreeList;

	stSector (*sectorArr)[SECTOR_SIZE] = server->_sectorArr;

	std::unordered_map<unsigned __int64, stUser*>* userTable = &server->_userTable;
	
	stJob* job = jobFreeList->allocObject();

	unsigned __int64* loginMsgCnt = &server->_loginMsgCnt;
	unsigned __int64* sectorMoveMsgCnt = &server->_sectorMoveMsgCnt;
	unsigned __int64* chatMsgCnt = &server->_chatMsgCnt;

	for (;;) {

		DWORD eventResult = WaitForMultipleObjects(2, events, false, INFINITE);

		int jobQueSize = jobQueue->getSize();

		if (eventResult == WAIT_OBJECT_0 && jobQueSize == 0) {
			break;
		}

		while (jobQueSize > 0) {
			
			jobQueue->pop(&job);
			jobQueSize -= 1;

			switch (job->_code) {

			case JOB::USER_ENTER: {
				stUser* user = new stUser;
				ZeroMemory(user, sizeof(stUser));
				unsigned __int64 sessionID = job->_sessionID;

				user->_sessionID = sessionID;
				user->_sector = nullptr;
				user->_willDisconnect = false;

				user->_login = false;

				userTable->insert(std::pair<unsigned __int64, stUser*>(sessionID, user));
				/*
				{

					unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&user->logCnt) - 1;
					stUser::stLog* log = &user->log[(stUser::LOG_INDEX_TYPE)logCnt];
					log->_logCnt = logCnt;
					log->_msg = L"USER_ENTER";

					if (logCnt > 0xFF) {
						CDump::crash();
					}

				}
				*/
			}
			break;

			case JOB::USER_LEAVE: {

				unsigned __int64 sessionID = job->_sessionID;

				auto userIter = userTable->find(sessionID);
				stUser* user = userIter->second;
				/*
				{

					unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&user->logCnt) - 1;
					stUser::stLog* log = &user->log[(stUser::LOG_INDEX_TYPE)logCnt];
					log->_logCnt = logCnt;
					log->_msg = L"USER_LEAVE";

					if (logCnt > 0xFF) {
						CDump::crash();
					}
				}
				*/
				if (user->_sector != nullptr) {
					user->_sector->_userList.remove(user);
				}
				
				/*		
				if (user->_willDisconnect == false) {
					CDump::crash();
				}
				*/
				userTable->erase(userIter);

				delete user;
			}
			break;

			case JOB::REQ_LOGIN: {

				*loginMsgCnt += 1;

				unsigned __int64 sessionID = job->_sessionID;
				stUser* user = userTable->find(sessionID)->second;

				CPacketPtr_Net packet = job->_packet;

				packet >> user->_accountNo;

				packet.popData(sizeof(WCHAR) * ID_LEN, (unsigned char*)user->_id);
				packet.popData(sizeof(WCHAR) * NICKNAME_LEN, (unsigned char*)user->_nickName);
	
				user->_sector = nullptr;
				user->_login = true;

				CPacketPtr_Net resultPacket;
				resultPacket << (WORD)JOB::RES_LOGIN;
				resultPacket << (BYTE)1;
				resultPacket << user->_accountNo;

				server->sendPacket(sessionID, resultPacket);

				resultPacket.decRef();
				packet.decRef();
				/*
				{

					unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&user->logCnt) - 1;
					stUser::stLog* log = &user->log[(stUser::LOG_INDEX_TYPE)logCnt];
					log->_logCnt = logCnt;
					log->_msg = L"USER_LOGIN";

					if (logCnt > 0xFF) {
						CDump::crash();
					}
				}
				*/
			}
			break;

			case JOB::REQ_MOVE_SECTOR: {

				CPacketPtr_Net resultPacket;

				do {
					*sectorMoveMsgCnt += 1;

					CPacketPtr_Net packet = job->_packet;

					unsigned __int64 sessionID = job->_sessionID;
					stUser* user = userTable->find(sessionID)->second;

					if (user->_login == false) {
						packet.decRef();
						server->disconnect(sessionID);
						break;
					}

					WORD sectorX;
					WORD sectorY;

					__int64 accountNo;

					packet >> accountNo >> sectorX >> sectorY;

					if (user->_sector != nullptr) {
						user->_sector->_userList.remove(user);
					}

					if (sectorY >= SECTOR_SIZE || sectorX >= SECTOR_SIZE) {

						server->disconnect(sessionID);
						packet.decRef();
						break;
					}

					user->_sector = &sectorArr[sectorY][sectorX];


					user->_sector->_userList.push_back(user);

					resultPacket << (WORD)JOB::RES_MOVE_SECTOR;
					resultPacket << user->_accountNo;
					resultPacket << sectorX << sectorY;

					server->sendPacket(sessionID, resultPacket);
					/*
					{

						unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&user->logCnt) - 1;
						stUser::stLog* log = &user->log[(stUser::LOG_INDEX_TYPE)logCnt];
						log->_logCnt = logCnt;
						log->_msg = L"MOVE_SECTOR";

						if (logCnt > 0xFF) {
							CDump::crash();
						}
					}
					*/
					packet.decRef();

				} while (false);

				resultPacket.decRef();
			}
			break;

			case JOB::REQ_SEND_CHAT: {

				CPacketPtr_Net resultPacket;

				do {
					*chatMsgCnt += 1;

					CPacketPtr_Net packet = job->_packet;

					unsigned __int64 sessionID = job->_sessionID;
					auto userIter = userTable->find(sessionID);
					stUser* user = userIter->second;

					if (user->_login == false) {
						packet.decRef();
						server->disconnect(sessionID);
						break;
					}
					if (user->_sector == nullptr) {
						packet.decRef();
						server->disconnect(sessionID);
						break;
					}

					WORD msgLen;
					
					INT64 accountNo;
					packet >> accountNo;

					if (accountNo != user->_accountNo) {
						packet.decRef();
						server->disconnect(sessionID);
						break;
					}				

					packet >> msgLen;

					resultPacket << (WORD)JOB::RES_SEND_CHAT;
					resultPacket << user->_accountNo;
					resultPacket.putData(sizeof(WCHAR) * ID_LEN, (unsigned char*)user->_id);
					resultPacket.putData(sizeof(WCHAR) * NICKNAME_LEN, (unsigned char*)user->_nickName);
					resultPacket << msgLen;
					/*
					{

						unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&user->logCnt) - 1;
						stUser::stLog* log = &user->log[(stUser::LOG_INDEX_TYPE)logCnt];
						log->_logCnt = logCnt;
						log->_msg = L"SEND_CHAT";
						log->msgLen = msgLen;
						memcpy(log->packet, packet.getBufStart(), 99);

						if (logCnt > 0xFF) {
							CDump::crash();
						}
					}
					*/
					if (*packet.getFrontPtr() == '=') {
						user->_willDisconnect = true;
					}


					resultPacket.putData(msgLen, (unsigned char*)packet.getFrontPtr());


					stSector* userSector = user->_sector;
					std::vector<stSector*>* sendRange = &userSector->_sendRange;

					for (auto sectorIter = sendRange->begin(); sectorIter != sendRange->end(); ++sectorIter) {

						std::list<stUser*>* otherUserList = &(*sectorIter)->_userList;
						for (auto otherUserIter = otherUserList->begin(); otherUserIter != otherUserList->end(); ++otherUserIter) {
							server->sendPacket((*otherUserIter)->_sessionID, resultPacket);
						}

					}

					packet.decRef();
				} while (false);

				resultPacket.decRef();
				
			}
			break;

			default: {
				unsigned __int64 sessionID = job->_sessionID;
				server->disconnect(sessionID);
				job->_packet.decRef();

			}
			break;

			}

			jobFreeList->freeObject(job);

			


		}
	}

	return 0;
}


void CChatServer::stop() {

	_stop = true;

	SetEvent(_stopEvent);

	if (WaitForSingleObject(_updateThread, INFINITE) != WAIT_OBJECT_0) {
		CDump::crash();
	}

	CNetServer::stop();

}

void CChatServer::init() {

	_sectorCntServer = new CSectorCntServer(10000);

	for (int y = 0; y < SECTOR_SIZE; ++y) {
		for (int x = 0; x < SECTOR_SIZE; ++x) {
			_sectorCntServer->setUserList(y, x, &_sectorArr[y][x]._userList);
		}
	}

	_sectorCntServer->init();
	_sectorCntServer->start(L"0.0.0.0", 8000, 1, 1, 1, true, 5000, 5000);
}
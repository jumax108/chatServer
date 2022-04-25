#include "headers/chatServer.h"

CChatServer::CChatServer() {

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

	_updateThread = (HANDLE)_beginthreadex(nullptr, 0, CChatServer::updateFunc, this, 0, nullptr);

}

void CChatServer::onError(int errorCode, const wchar_t* errorMsg) {

}

bool CChatServer::onConnectRequest(unsigned int ip, unsigned short port) {

	return true;
}

void CChatServer::onClientJoin(unsigned int ip, unsigned short port, unsigned __int64 sessionID) {

	stJob job;
	job._code = JOB::USER_ENTER;
	job._sessionID = sessionID;

	_jobQueue.push(job);
	SetEvent(_jobEvent);

}

void CChatServer::onClientLeave(unsigned __int64 sessionID) {

	stJob job;
	job._code = JOB::USER_LEAVE;
	job._sessionID = sessionID;

	_jobQueue.push(job);
	SetEvent(_jobEvent);

}

void CChatServer::onRecv(unsigned __int64 sessionID, CPacketPointer packet) {

	WORD type;
	packet >> type;

	stJob job;
	job._code = (JOB)type;
	job._sessionID = sessionID;
	job._packet = packet;

	_jobQueue.push(job);

	SetEvent(_jobEvent);

}

void CChatServer::onSend(unsigned __int64 sessionID, int sendSize) {

}

unsigned __stdcall CChatServer::updateFunc(void* args) {

	CChatServer* server = (CChatServer*)args;
	HANDLE jobEvent = server->_jobEvent;
	CLockFreeQueue<stJob>* jobQueue = &server->_jobQueue;

	stSector (*sectorArr)[SECTOR_SIZE] = server->_sectorArr;

	std::unordered_map<unsigned __int64, stUser*>* userTable = &server->_userTable;

	for (;;) {

		DWORD eventResult = WaitForSingleObject(jobEvent, INFINITE);
		if (eventResult != WAIT_OBJECT_0) {
			CDump::crash();
		}

		int jobQueSize = jobQueue->getSize();

		while (jobQueSize > 0) {
			stJob job;
			jobQueue->pop(&job);
			jobQueSize -= 1;

			switch (job._code) {

			case JOB::USER_ENTER: {
				stUser* user = new stUser;
				unsigned __int64 sessionID = job._sessionID;

				user->_sessionID = sessionID;

				userTable->insert(std::pair<unsigned __int64, stUser*>(sessionID, user));
			}
			break;

			case JOB::USER_LEAVE: {

				unsigned __int64 sessionID = job._sessionID;
				userTable->erase(sessionID);
			}
			break;

			case JOB::REQ_LOGIN: {

				unsigned __int64 sessionID = job._sessionID;
				stUser* user = userTable->find(sessionID)->second;

				CPacketPointer packet = job._packet;

				packet >> user->_accountNo;

				packet.popData(sizeof(WCHAR) * ID_LEN, (unsigned char*)user->_id);
				packet.popData(sizeof(WCHAR) * NICKNAME_LEN, (unsigned char*)user->_nickName);

				user->_sector = nullptr;
				//sectorArr[0][0]._userList.push_back(user);

				CPacketPtr_Net resultPacket;
				resultPacket << (WORD)JOB::RES_LOGIN;
				resultPacket << (BYTE)1;
				resultPacket << user->_accountNo;


				server->sendPacket(sessionID, resultPacket);

			}
			break;

			case JOB::REQ_MOVE_SECTOR: {

				unsigned __int64 sessionID = job._sessionID;
				stUser* user = userTable->find(sessionID)->second;

				WORD sectorX;
				WORD sectorY;

				CPacketPointer packet = job._packet;
				packet >> sectorX >> sectorY;

				if (user->_sector != nullptr) {
					user->_sector->_userList.remove(user);
				}

				user->_sector = &sectorArr[sectorY][sectorX];

				user->_sector->_userList.push_back(user);

				CPacketPtr_Net resultPacket;
				resultPacket << (WORD)JOB::RES_MOVE_SECTOR;
				resultPacket << user->_accountNo;
				resultPacket << sectorX << sectorY;

				server->sendPacket(sessionID, resultPacket);

			}
			break;

			case JOB::REQ_SEND_CHAT: {

				unsigned __int64 sessionID = job._sessionID;
				stUser* user = userTable->find(sessionID)->second;

				CPacketPointer packet = job._packet;
				WORD msgLen;
				{
					INT64 accountNo;
					packet >> accountNo;

					if (accountNo != user->_accountNo) {
						CDump::crash();
					}
				}
				packet >> msgLen;

				CPacketPtr_Net resultPacket;
				resultPacket << (WORD)JOB::RES_SEND_CHAT;
				resultPacket << user->_accountNo;
				resultPacket.putData(sizeof(WCHAR) * ID_LEN, (unsigned char*)user->_id);
				resultPacket.putData(sizeof(WCHAR) * NICKNAME_LEN, (unsigned char*)user->_nickName);
				resultPacket << msgLen;
				resultPacket.putData(sizeof(WCHAR) * msgLen, (unsigned char*)packet.getFrontPtr());

				stSector* userSector = user->_sector;
				std::vector<stSector*>* sendRange = &userSector->_sendRange;
				for (auto sectorIter = sendRange->begin(); sectorIter != sendRange->end(); ++sectorIter) {

					std::list<stUser*>* otherUserList = &(*sectorIter)->_userList;
					for (auto otherUserIter = otherUserList->begin(); otherUserIter != otherUserList->end(); ++otherUserIter) {
						server->sendPacket((*otherUserIter)->_sessionID, resultPacket);
					}

				}

			}
			break;

			}
		}
	}

}
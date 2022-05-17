#include "headers/netServer.h"

//#define SESSION_LOG

CNetServer::CNetServer(){

	_sessionArr = nullptr;
	_sessionNum = 0;
	_sessionCnt = 0;
	_sessionIndexStack = (CLockFreeStack<int>*)malloc(sizeof(CLockFreeStack<int>));

	_acceptThread = NULL;
	_workerThreadNum = 0;
	_workerThread = nullptr;

	_listenSocket = NULL;
	_iocp = NULL;

	_sendCnt = 0;
	_recvCnt = 0;
	
	_sendTPS = 0;
	_recvTPS = 0;

	_sendBytes = 0;
	
	_log.setDirectory(L"log");
	_log.setPrintGroup(LOG_GROUP::LOG_ERROR | LOG_GROUP::LOG_SYSTEM);

	_stopEvent = CreateEvent(nullptr, true, false, L"stopEvent");
}

void CNetServer::disconnect(unsigned __int64 sessionID){

	unsigned short sessionIndex = sessionID & netServer::SESSION_INDEX_MASK;
	stSession* session = &_sessionArr[sessionIndex];

	InterlockedIncrement16((SHORT*)&session->_ioCnt);

	{
#ifdef SESSION_LOG
		unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
		stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

		log->logCnt = logCnt;
		log->sessionID = sessionID >> 16;
		log->msg = L"disconnect enter";
#endif
	}

	if (session->_callDisconnect == true || session->_released == true) {
		{

#ifdef SESSION_LOG
			unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
			stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

			log->logCnt = logCnt;
			log->sessionID = sessionID >> 16;
			log->msg = L"disconnect return - already";
#endif

		}
		if (InterlockedDecrement16((SHORT*)&session->_ioCnt) == 0) {
			release(sessionID);
		}
		return;
	}

	if (session->_sessionID != sessionID) {

		{

#ifdef SESSION_LOG
			unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
			stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

			log->logCnt = logCnt;
			log->sessionID = sessionID >> 16;
			log->msg = L"disconnect return - sessionID";
#endif

		}
		if (InterlockedDecrement16((SHORT*)&session->_ioCnt) == 0) {
			release(sessionID);
		}
		return;
	}


	session->_callDisconnect = true;

	CancelIoEx((HANDLE)session->_sock, &session->_recvOverlapped);
	CancelIoEx((HANDLE)session->_sock, &session->_sendOverlapped);

	InterlockedAdd64((LONG64*)&_sendBytes, netServer::PACKET_HEADER_SIZE);

	{

#ifdef SESSION_LOG
		unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
		stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

		log->logCnt = logCnt;
		log->sessionID = sessionID >> 16;
		log->msg = L"disconnect end";
#endif

	}

	if (InterlockedDecrement16((SHORT*)&session->_ioCnt) == 0) {
		release(sessionID);
	}
}

void CNetServer::release(unsigned __int64 sessionID) {

	unsigned short sessionIndex = sessionID & netServer::SESSION_INDEX_MASK;
	stSession* session = &_sessionArr[sessionIndex];
	{

#ifdef SESSION_LOG
		unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
		stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

		log->logCnt = logCnt;
		log->sessionID = sessionID >> 16;
		log->msg = L"release enter";
#endif

	}


	if (InterlockedCompareExchange((UINT*)&session->_released, 1, 0) != 0) {
		{

#ifdef SESSION_LOG
			unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
			stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

			log->logCnt = logCnt;
			log->sessionID = sessionID >> 16;
			log->msg = L"release return - already";
#endif

		}
		return;
	}

	if (session->_sessionID != sessionID) {
		{

#ifdef SESSION_LOG
			unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
			stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

			log->logCnt = logCnt;
			log->sessionID = sessionID >> 16;
			log->msg = L"release return - session ID";
#endif

		}
		return;
	}
		
	// send buffer 안에 있는 패킷들 제거
	CLockFreeQueue<CPacketPtr_Net>* sendBuffer = &session->_sendQueue;
	int sendQueueSize = sendBuffer->getSize();

	CPacketPtr_Net packet;
	packet.decRef();

	for (int packetCnt = 0; packetCnt < sendQueueSize; ++packetCnt) {
		
		sendBuffer->pop(&packet);
		packet.decRef();

	}

	CRingBuffer* recvBuffer = &session->_recvBuffer;
	recvBuffer->moveFront(recvBuffer->getUsedSize());

	closesocket(session->_sock);
	session->_sock = 0;


	/*if (session->_willDisconnect == false) {
		CDump::crash();
	}*/

	InterlockedDecrement64((LONG64*)&_sessionCnt);

	this->onClientLeave(session->_sessionID);


	_sessionIndexStack->push(sessionIndex);

#ifdef SESSION_LOG
	unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
	stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

	log->logCnt = logCnt;
	log->sessionID = sessionID >> 16;
	log->msg = L"release end";
#endif
}

bool CNetServer::sendPacket(unsigned __int64 sessionID, CPacketPtr_Net packet){
	
	unsigned short sessionIndex = sessionID & netServer::SESSION_INDEX_MASK;
	stSession* session = &_sessionArr[sessionIndex];
	{
#ifdef SESSION_LOG
		unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
		stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

		log->logCnt = logCnt;
		log->sessionID = sessionID >> 16;
		log->msg = L"send packet enter";
#endif
	}

	InterlockedIncrement16((SHORT*)&session->_ioCnt);

	if (session->_released == true || session->_callDisconnect == true) {
		if (InterlockedDecrement16((SHORT*)&session->_ioCnt) == 0) {
			release(sessionID);
		}
		{
#ifdef SESSION_LOG
			unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
			stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

			log->logCnt = logCnt;
			log->sessionID = sessionID >> 16;
			log->msg = L"send packet - already";
#endif
		}
		return false;
	}

	if (session->_sessionID != sessionID) {
		if (InterlockedDecrement16((SHORT*)&session->_ioCnt) == 0) {
			release(sessionID);
		}
		{
#ifdef SESSION_LOG
			unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
			stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

			log->logCnt = logCnt;
			log->sessionID = sessionID >> 16;
			log->msg = L"send packet - sessionID";
#endif
		}
		return false;
	}
	

	CLockFreeQueue<CPacketPtr_Net>* sendQueue = &session->_sendQueue;
	packet.incRef();
	packet.setHeader();
	packet.incoding();
	sendQueue->push(packet);
	
	sendPost(session);
	
	if (InterlockedDecrement16((SHORT*)&session->_ioCnt) == 0) {
		release(sessionID);
	}
	{
#ifdef SESSION_LOG
		unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
		stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

		log->logCnt = logCnt;
		log->sessionID = sessionID >> 16;
		log->msg = L"send packet end";
#endif
	}
	return true;

}

unsigned CNetServer::completionStatusFunc(void *args){
	
	CNetServer* server = (CNetServer*)args;

	HANDLE iocp = server->_iocp;

	for(;;){
		
		unsigned int transferred;
		OVERLAPPED* overlapped;
		unsigned __int64 sessionID;
		GetQueuedCompletionStatus(iocp, (LPDWORD)&transferred, (PULONG_PTR)&sessionID, &overlapped, INFINITE);
		
		if(overlapped == nullptr){
			break;			
		}
		
		unsigned short sessionIndex = (unsigned short)(sessionID & netServer::SESSION_INDEX_MASK);
		stSession* session = &server->_sessionArr[sessionIndex];

		do {

			SOCKET sock = session->_sock;

			// send 완료 처리
			if(&session->_sendOverlapped == overlapped){
				{
#ifdef SESSION_LOG
					unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
					stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

					log->logCnt = logCnt;
					log->sessionID = sessionID >> 16;
					log->msg = L"send copmlete";
#endif
				}
				int packetTotalSize = 0;

				int packetNum = session->_packetCnt;
				CPacketPtr_Net* packets = session->_packets;
				CPacketPtr_Net* packetIter = packets;
				CPacketPtr_Net* packetEnd = packets + packetNum;
				for(; packetIter != packetEnd; ++packetIter){
					packetTotalSize += packetIter->getPacketPoolUsage();
					packetIter->decRef();
					packetIter->~CPacketPtr_Net();
				}

				session->_packetCnt = 0;

				session->_isSent = false;
			
				InterlockedAdd((LONG*)&server->_sendCnt, packetNum);


				if (sessionID != session->_sessionID || session->_callDisconnect == true) {
					break;
				}

				server->onSend(sessionID, packetTotalSize);

				CLockFreeQueue<CPacketPtr_Net>* sendQueue = &session->_sendQueue;

				if (sendQueue->getSize() > 0) {
				
					server->sendPost(session);
					
				} 
				{
#ifdef SESSION_LOG
					unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
					stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

					log->logCnt = logCnt;
					log->sessionID = sessionID >> 16;
					log->msg = L"send complete end";
#endif
				}
			}
			
			// recv 완료 처리
			else if(&session->_recvOverlapped == overlapped){
				{
#ifdef SESSION_LOG
					unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
					stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

					log->logCnt = logCnt;
					log->sessionID = sessionID >> 16;
					log->msg = L"recv complete";
#endif
				}
				if (sessionID != session->_sessionID || session->_callDisconnect == true) {
					break;
				}

				CRingBuffer* recvBuffer = &session->_recvBuffer;

				recvBuffer->moveRear(transferred);

				session->_lastRecvTime = GetTickCount64();

				// packet proc
				server->checkCompletePacket(session, recvBuffer);

			
				server->recvPost(session);
				{
#ifdef SESSION_LOG
					unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
					stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

					log->logCnt = logCnt;
					log->sessionID = sessionID >> 16;
					log->msg = L"recv complete end";
#endif
				}
			}
			
		} while (false);

		if (InterlockedDecrement16((SHORT*)&session->_ioCnt) == 0) {
			server->release(sessionID);
		}
	}

	return 0;
}

unsigned CNetServer::acceptFunc(void* args){
	
	CNetServer* server = (CNetServer*)args;
	SOCKET listenSocket = server->_listenSocket;

	HANDLE iocp = server->_iocp;

	SOCKADDR_IN addr;
	int addrLen = sizeof(SOCKADDR_IN);

	for(;;){

		SOCKET sock = accept(listenSocket, (SOCKADDR*)&addr, &addrLen);
		
		DWORD stopEventResult = WaitForSingleObject(server->_stopEvent, 0);
		if(WAIT_OBJECT_0 == stopEventResult){
			// thread stop
			break;
		}

		unsigned int ip = addr.sin_addr.S_un.S_addr;
		unsigned short port = addr.sin_port;
		
		/////////////////////////////////////////////////////////
		// 서버 접속 가능 체크
		/////////////////////////////////////////////////////////
		if(server->onConnectRequest(ip, port) == false){
			// 접속이 거부되었습니다.
			closesocket(sock);
			continue;
		}
		/////////////////////////////////////////////////////////
		
		/////////////////////////////////////////////////////////
		// 접속 허용
		/////////////////////////////////////////////////////////
		{
			unsigned __int64 sessionNum = server->_sessionNum;
			unsigned __int64* sessionCnt = &server->_sessionCnt;
			bool isSessionFull = sessionNum == *sessionCnt;
			
			/////////////////////////////////////////////////////////
			// 동접 최대치 체크
			/////////////////////////////////////////////////////////
			if(isSessionFull == true){
				server->onError(20000, L"세팅된 동시접속자 최대치 도달하여 신규 접속 불가");
				closesocket(sock);
				continue;
			}
			/////////////////////////////////////////////////////////
			
			/////////////////////////////////////////////////////////
			// 세션 초기화
			/////////////////////////////////////////////////////////
			
			// session array의 index 확보
			unsigned short sessionIndex = 0;
			CLockFreeStack<int>* sessionIndexStack = server->_sessionIndexStack;
			sessionIndexStack->pop((int*)&sessionIndex);

			// session 확보
			stSession* sessionArr = server->_sessionArr;
			stSession* session = &sessionArr[sessionIndex];


			InterlockedIncrement((LONG*)&server->_acceptCnt);
			server->_acceptTotal += 1;

			unsigned __int64 sessionID = session->_sessionID;
			unsigned __int64 sessionUseCnt = 0;

			// ID의 상위 6바이트는 세션 메모리에 대한 재사용 횟수
			// 하위 2바이트는 세션 인덱스
				
			// session id 세팅
			if(sessionID == 0){
				// 세션 최초 사용
				int sendBufferSize = server->_sendBufferSize;
				int recvBufferSize = server->_recvBufferSize;

				new (session) stSession(sendBufferSize, recvBufferSize);

				session->_sessionID = (unsigned __int64)session;

			} else {
				// 세션 재사용
				sessionUseCnt = sessionID & netServer::SESSION_ALLOC_COUNT_MASK;

			}

			InterlockedIncrement16((SHORT*)&session->_ioCnt);
			

			sessionID = (sessionUseCnt + 0x10000) | (unsigned __int64)sessionIndex;


			{
#ifdef SESSION_LOG
				unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
				stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

				log->logCnt = logCnt;
				log->sessionID = sessionID >> 16;
				log->msg = L"accept enter";
#endif
			}

			session->_sessionID = sessionID;
			session->_ip = ip;
			session->_port = port;
			session->_isSent = false;		
			session->_timeoutTime = server->_defaultTimeOut;
			session->_lastRecvTime = GetTickCount64();
			session->_sock = sock;
			//session->_willDisconnect = false;
			CRingBuffer* recvBuffer = &session->_recvBuffer;
			CLockFreeQueue<CPacketPtr_Net>* sendQueue = &session->_sendQueue;

			InterlockedIncrement64((LONG64*)&server->_sessionCnt);

			CreateIoCompletionPort((HANDLE)sock, iocp, (ULONG_PTR)sessionID, 0);

			session->_released = false;
			session->_callDisconnect = false;

			if (session->_recvBuffer.getUsedSize() > 0 || session->_sendQueue.getSize() > 0) {
				CDump::crash();
			}

			server->onClientJoin(ip, port, sessionID);
			server->recvPost(session);

			InterlockedAdd64((LONG64*)&server->_sendBytes, netServer::PACKET_HEADER_SIZE);

			if (InterlockedDecrement16((SHORT*)&session->_ioCnt) == 0) {

				{

#ifdef SESSION_LOG
					unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
					stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

					log->logCnt = logCnt;
					log->sessionID = sessionID >> 16;
					log->msg = L"accept release";
#endif
				}


				server->release(sessionID);
			}
			/*
			{

				unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
				stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

				log->logCnt = logCnt;
				log->sessionID = session->_sessionID;
				log->msg = L"accept end";

			}
			*/
			/////////////////////////////////////////////////////////

		}
		/////////////////////////////////////////////////////////
	}

	return 0;
}

void CNetServer::recvPost(stSession* session, bool ignoreReleased){

	if (_stop == true) {
		return;
	}

	unsigned __int64 sessionID = session->_sessionID;

	{
#ifdef SESSION_LOG
		unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
		stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

		log->logCnt = logCnt;
		log->sessionID = sessionID >> 16;
		log->msg = L"recv post enter";
#endif
	}

	InterlockedIncrement16((SHORT*)&session->_ioCnt);

	if (ignoreReleased == false) {
		if (session->_released == true || session->_callDisconnect == true) {
			
			
			if (InterlockedDecrement16((SHORT*)&session->_ioCnt) == 0) {
				release(sessionID);
			}

			{
#ifdef SESSION_LOG
				unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
				stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

				log->logCnt = logCnt;
				log->sessionID = sessionID >> 16;
				log->msg = L"recv post - release";
#endif
			}
			return;
		}
	}


	/////////////////////////////////////////////////////////
	// recv buffer 정보를 wsa buf로 복사
	/////////////////////////////////////////////////////////
	WSABUF wsaBuf[2];
	int wsaCnt = 1;

	CRingBuffer* recvBuffer = &session->_recvBuffer;

	int rear = recvBuffer->rear();
	int front = recvBuffer->front();
	char* directPushPtr = recvBuffer->getDirectPush();
	int directFreeSize = recvBuffer->getDirectFreeSize();
	char* bufStartPtr = recvBuffer->getBufferStart();

	wsaBuf[0].buf = directPushPtr;
	wsaBuf[0].len = directFreeSize;

	if(front <= rear){
		wsaBuf[1].buf = bufStartPtr;
		wsaBuf[1].len = front;
		wsaCnt = 2;
	}
	/////////////////////////////////////////////////////////
	
	/////////////////////////////////////////////////////////
	// wsa recv
	/////////////////////////////////////////////////////////
	OVERLAPPED* overlapped = &session->_recvOverlapped;
	SOCKET sock = session->_sock;
	
	int recvResult;
	int recvError;

	unsigned int flag = 0;
	recvResult = WSARecv(sock, wsaBuf, wsaCnt, nullptr, (LPDWORD)&flag, overlapped, nullptr);
	if(recvResult == SOCKET_ERROR){
		recvError = WSAGetLastError();
		if(recvError != WSA_IO_PENDING){
			{
#ifdef SESSION_LOG
				unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
				stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

				log->logCnt = logCnt;
				log->sessionID = sessionID >> 16;
				log->msg = L"recv post error";
#endif
			}
		//	session->_willDisconnect = true;
			if (InterlockedDecrement16((SHORT*)&session->_ioCnt) == 0) {
				release(sessionID);
			}

			if(recvError != 10054){
				_log(L"recv.txt", LOG_GROUP::LOG_SYSTEM, L"session: 0x%I64x, sock: %I64d, wsaCnt: %d, wsaBuf[0]: 0x%I64x, wsaBuf[1]: 0x%I64x\n", sessionID, sock, wsaCnt, wsaBuf[0], wsaBuf[1]);
			}
			
			return ;
		}
	}
	{
#ifdef SESSION_LOG
		unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
		stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

		log->logCnt = logCnt;
		log->sessionID = sessionID >> 16;
		log->msg = L"recv post end";
#endif
	}

	/////////////////////////////////////////////////////////
}

void CNetServer::sendPost(stSession* session){
	
	if (_stop == true) {
		return;
	}

	unsigned __int64 sessionID = session->_sessionID;
	{
#ifdef SESSION_LOG
		unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
		stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

		log->logCnt = logCnt;
		log->sessionID = sessionID >> 16;
		log->msg = L"send post enter";
#endif
	}
	/////////////////////////////////////////////////////////
	// ioCnt 증가
	/////////////////////////////////////////////////////////
	InterlockedIncrement16((SHORT*)&session->_ioCnt);
	/////////////////////////////////////////////////////////

	
	/////////////////////////////////////////////////////////
	// 보낼 데이터가 있는지 체크
	/////////////////////////////////////////////////////////
	CLockFreeQueue<CPacketPtr_Net>* sendQueue = &session->_sendQueue;
	int wsaNum;

	unsigned int usedSize = sendQueue->getSize();
	wsaNum = usedSize;
	wsaNum = min(wsaNum, netServer::MAX_PACKET);
	/////////////////////////////////////////////////////////

	if (wsaNum == 0) {

		if (InterlockedDecrement16((SHORT*)&session->_ioCnt) == 0) {
			release(sessionID);
		} {
#ifdef SESSION_LOG
			unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
			stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

			log->logCnt = logCnt;
			log->sessionID = sessionID >> 16;
			log->msg = L"send post - no data";
#endif
		}
		return;
	}
	if (session->_released == true || session->_callDisconnect == true) {
		
		if (InterlockedDecrement16((SHORT*)&session->_ioCnt) == 0) {
			release(sessionID);
		} {
#ifdef SESSION_LOG
			unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
			stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

			log->logCnt = logCnt;
			log->sessionID = sessionID >> 16;
			log->msg = L"send post - released";
#endif
		}
		return;
	}

	/////////////////////////////////////////////////////////
	// send 1회 제한 처리
	/////////////////////////////////////////////////////////
	if (InterlockedExchange8((CHAR*)&session->_isSent, 1) == 1) {
	
		if (InterlockedDecrement16((SHORT*)&session->_ioCnt) == 0) {
			release(sessionID);
		} {
#ifdef SESSION_LOG
			unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
			stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

			log->logCnt = logCnt;
			log->sessionID = sessionID >> 16;
			log->msg = L"send post - sending....";
#endif
		}
		return;
	}
	/////////////////////////////////////////////////////////


	/////////////////////////////////////////////////////////
	// packet을 wsaBuf로 복사
	/////////////////////////////////////////////////////////
	WSABUF wsaBuf[netServer::MAX_PACKET];
	session->_packetCnt = wsaNum;
	int packetNum = wsaNum;

	unsigned __int64 sendSize = 0;

	for(int packetCnt = 0; packetCnt < packetNum; ++packetCnt){
		
		CPacketPtr_Net* packet = &session->_packets[packetCnt];

		sendQueue->pop(packet);
		wsaBuf[packetCnt].buf = packet->getBufStart();

		int packetSize = packet->getPacketSize();
		wsaBuf[packetCnt].len = packetSize;
		sendSize += packetSize;

	}
	/////////////////////////////////////////////////////////
	
	/////////////////////////////////////////////////////////
	// wsa send
	/////////////////////////////////////////////////////////
	int sendResult;
	int sendError;

	SOCKET sock = session->_sock;
	OVERLAPPED* overlapped = &session->_sendOverlapped;

	sendResult = WSASend(sock, wsaBuf, wsaNum, nullptr, 0, overlapped, nullptr);

	if(sendResult == SOCKET_ERROR){
		sendError = WSAGetLastError();
		if(sendError != WSA_IO_PENDING){

		//	session->_willDisconnect = true;
			{
#ifdef SESSION_LOG
				unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
				stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

				log->logCnt = logCnt;
				log->sessionID = sessionID >> 16;
				log->msg = L"send post - error";
#endif
			}
			for (int packetCnt = 0; packetCnt < packetNum; ++packetCnt) {
				session->_packets[packetCnt].decRef();
			}

			session->_isSent = false;
			if (InterlockedDecrement16((SHORT*)&session->_ioCnt) == 0) {
				release(sessionID);
			}

			if (sendError != 10054) {
				_log(L"send.txt", LOG_GROUP::LOG_SYSTEM, L"session: 0x%I64x, sock: %I64d, wsaNum: %d, wsaBuf[0]: 0x%I64x, wsaBuf[1]: 0x%I64x, error: %d\n", sessionID, sock, wsaNum, wsaBuf[0], wsaBuf[1], sendError);
			}
			return ;
		}
	}

	int mssPacketNum = sendSize / netServer::MSS_SIZE;
	sendSize -= mssPacketNum * netServer::MSS_SIZE;
	_sendBytes += mssPacketNum * netServer::MTU_SIZE + sendSize + netServer::PACKET_HEADER_SIZE;

	/////////////////////////////////////////////////////////
	{
#ifdef SESSION_LOG
		unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&session->logCnt);
		stSession::stLog* log = &session->log[(stSession::LOG_INDEX_TYPE)logCnt];

		log->logCnt = logCnt;
		log->sessionID = sessionID >> 16;
		log->msg = L"send post end";
#endif
	}
}

void CNetServer::start(const wchar_t* serverIP, unsigned short serverPort,
			int createWorkerThreadNum, int runningWorkerThreadNum,
			unsigned short maxSessionNum, bool onNagle,
			unsigned int sendBufferSize, unsigned int recvBufferSize,
			ULONGLONG defaultTimeOut){
		
	/* var init */ {
		_sendBufferSize = sendBufferSize;
		_recvBufferSize = recvBufferSize;
		_sessionNum = maxSessionNum;
		_defaultTimeOut = defaultTimeOut;
	}

	// wsa startup
	int startupResult;
	int startupError;
	{
		WSAData wsaData;
		startupResult = WSAStartup(MAKEWORD(2,2), &wsaData);
		if(startupResult != NULL){
			startupError = WSAGetLastError();
			onError(startupError, L"wsa startup error");
			return ;
		}
	}

	// 리슨 소켓 생성
	int socketError;
	{
		_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
		if(_listenSocket == INVALID_SOCKET){
			socketError = WSAGetLastError();
			onError(socketError, L"listen socket create error");
			return ;
		}
	}

	// 리슨 소켓에 linger 옵션 적용
	int lingerResult;
	int lingerError;
	{
		LINGER lingerSet = {(unsigned short)1, (unsigned short)0};
		lingerResult = setsockopt(_listenSocket, SOL_SOCKET, SO_LINGER, (const char*)&lingerSet, sizeof(LINGER));
		if(lingerResult == SOCKET_ERROR){
			lingerError = WSAGetLastError();
			onError(lingerError, L"listen socket ligner option error");
			return ;
		}
	}

	// 리슨 소켓 send buffer 0
	int setSendBufferResult;
	int setSendBufferError;
	{
		int sendBufferSize = 0;
		setSendBufferResult = setsockopt(_listenSocket, SOL_SOCKET, SO_SNDBUF, (char*)&sendBufferSize, sizeof(int));
		if(setSendBufferResult == SOCKET_ERROR){

			setSendBufferError = WSAGetLastError();
			onError(setSendBufferError, L"listen socket send buffer size 변경 error");
			return ;
		}
	}
	
	// 리슨 소켓 바인딩
	int bindResult;
	int bindError;
	{
		SOCKADDR_IN addr;
		addr.sin_family = AF_INET;
		InetPtonW(AF_INET, serverIP, &addr.sin_addr.S_un.S_addr);
		addr.sin_port = htons(serverPort);

		bindResult = bind(_listenSocket, (SOCKADDR*)&addr, sizeof(SOCKADDR_IN));
		if(bindResult == SOCKET_ERROR){
			bindError = WSAGetLastError();
			onError(bindError, L"listen socket bind error");
			return ;
		}

	}

	// 리스닝 시작
	int listenResult;
	int listenError;
	{
		listenResult = listen(_listenSocket, SOMAXCONN_HINT(50000));
		if(listenResult == SOCKET_ERROR){
			listenError = WSAGetLastError();
			onError(listenError, L"listen error");
			return ;
		}
	}

	// session 배열 초기화
	{
		_sessionArr = (stSession*)malloc(sizeof(stSession) * _sessionNum);
		
		new (_sessionIndexStack) CLockFreeStack<int>();
		for(int sessionCnt = _sessionNum - 1; sessionCnt >= 0 ; --sessionCnt){
			new (&_sessionArr[sessionCnt]) stSession(sendBufferSize, recvBufferSize);
			_sessionIndexStack->push(sessionCnt);
		}
	}


	// iocp 초기화
	int iocpError;
	{
		_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, runningWorkerThreadNum);
		if(_iocp == NULL){
			iocpError = GetLastError();
			onError(iocpError, L"create iocp error");
			return ;
		}
	}

	// tps thread 초기화
	{
		_tpsCalcThread = (HANDLE)_beginthreadex(nullptr, 0, CNetServer::tpsCalcFunc, this, 0, nullptr);
	}

	// worker thread 초기화
	{
		_workerThreadNum = createWorkerThreadNum;
		_workerThread = (HANDLE*)malloc(sizeof(HANDLE) * createWorkerThreadNum);
			//(HANDLE*)HeapAlloc(_heap, HEAP_ZERO_MEMORY, sizeof(HANDLE) * );

		for(int threadCnt = 0; threadCnt < createWorkerThreadNum ; ++threadCnt){
			_workerThread[threadCnt] = (HANDLE)_beginthreadex(nullptr, 0, CNetServer::completionStatusFunc, (void*)this, 0, nullptr);
		}

	}

	// accept thread 초기화
	{
		_acceptThread = (HANDLE)_beginthreadex(nullptr, 0, CNetServer::acceptFunc, (void*)this, 0, nullptr);
	}

	/* time out thread */{

		//_timeOutThread = (HANDLE)_beginthreadex(nullptr, 0, CNetServer::timeoutFunc, (void*)this, 0, nullptr);

	}

}

void CNetServer::checkCompletePacket(stSession* session, CRingBuffer* recvBuffer){
	
	unsigned __int64 sessionID = session->_sessionID;
	unsigned int usedSize = recvBuffer->getUsedSize();

	while(usedSize > sizeof(stHeader)){
		
		// header 체크
		stHeader header;
		recvBuffer->frontBuffer(sizeof(stHeader), (char*)&header);

		if (header.code != netServer::PACKET_CODE) {
			disconnect(sessionID);
		}

		int payloadSize = header.size;
		int packetSize = payloadSize + sizeof(stHeader);

		if (packetSize > netServer::PACKET_MAX_SIZE) {
			disconnect(sessionID);
		}

		// 패킷이 recvBuffer에 완성되었다면
		if(usedSize >= packetSize){
			
			recvBuffer->popBuffer(sizeof(stHeader));

			CPacketPtr_Net packet;

			if (packet.checkBufferSize(payloadSize) == false) {
				packet.setBufferSize(payloadSize);
			}

			recvBuffer->frontBuffer(payloadSize, packet.getRearPtr());
			packet.moveRear(payloadSize);

			recvBuffer->popBuffer(payloadSize);
			
			unsigned short sessionIndex = sessionID & netServer::SESSION_INDEX_MASK;
			stSession* session = &_sessionArr[sessionIndex];

			packet.moveFront(sizeof(stHeader));

			memcpy(packet.getBufStart(), &header, sizeof(stHeader));

			packet.decoding();

			InterlockedIncrement((LONG*)&_recvCnt);
			onRecv(sessionID, packet);

			packet.decRef();

			usedSize -= packetSize;

		} else {
			break;
		}

	}
}

void CNetServer::stop(){

	closesocket(_listenSocket);

	_stop = true;

	SetEvent(_stopEvent);
	
	if (WaitForSingleObject(_acceptThread, INFINITE) != WAIT_OBJECT_0) {
		CDump::crash();
	}

	for(int sessionCnt = 0; sessionCnt < _sessionNum ; ++sessionCnt){
		disconnect(_sessionArr[sessionCnt]._sessionID);
	}
	while(_sessionCnt > 0);

	for(int threadCnt = 0 ; threadCnt < _workerThreadNum; ++threadCnt){
		PostQueuedCompletionStatus((HANDLE)_iocp, (DWORD)0, (ULONG_PTR)0, (LPOVERLAPPED)0);
	}
	WaitForMultipleObjects(_workerThreadNum, _workerThread, true, INFINITE);

	CloseHandle(_iocp);
	
	_sessionIndexStack->~CLockFreeStack();
	for(int sessionCnt = 0; sessionCnt < _sessionNum; ++sessionCnt){
		_sessionArr[sessionCnt].~stSession();
	}
}

unsigned __stdcall CNetServer::tpsCalcFunc(void* args){

	CNetServer* server = (CNetServer*)args;

	int* sendCnt = &server->_sendCnt;
	int* recvCnt = &server->_recvCnt;
	int* acceptCnt = &server->_acceptCnt;

	int* sendTPS = &server->_sendTPS;
	int* recvTPS = &server->_recvTPS;
	int* acceptTPS = &server->_acceptTPS;

	for(;;){

		*sendTPS = *sendCnt;
		*recvTPS = *recvCnt;
		*acceptTPS = *acceptCnt;

		InterlockedExchange((LONG*)sendCnt, 0);
		InterlockedExchange((LONG*)recvCnt, 0);
		InterlockedExchange((LONG*)acceptCnt, 0);

		Sleep(999);

	}

	return 0;
}



CNetServer::stSession::stSession(unsigned int sendQueueSize, unsigned int recvBufferSize):
	_recvBuffer(recvBufferSize)
{
	_sessionID = 0;
	_sock = NULL;
	_ip = 0;
	_port = 0;
	_isSent = false;
	_released = true;
	_ioCnt = 0;
	_callDisconnect = false;

	ZeroMemory(&_sendOverlapped, sizeof(OVERLAPPED));
	ZeroMemory(&_recvOverlapped, sizeof(OVERLAPPED));
	_packets = (CPacketPtr_Net*)malloc(sizeof(CPacketPtr_Net) * netServer::MAX_PACKET);
		
		//(CPacketPtr_Net*)HeapAlloc(_heap, HEAP_ZERO_MEMORY, sizeof(CPacketPtr_Net) * netServer::MAX_PACKET);
	_packetCnt = 0;

//	_willDisconnect = false;

}

CNetServer::stSession::~stSession(){
	free(_packets);
	//HeapFree(_heap, 0, _packets);
}

unsigned __stdcall CNetServer::timeoutFunc(void* args) {

	CNetServer* server = (CNetServer*)args;

	ULONGLONG sleepTime = server->_defaultTimeOut/2;
	
	stSession* sessionBegin = server->_sessionArr;
	stSession* sessionEnd = server->_sessionArr + server->_sessionNum;

	HANDLE stopEvent = server->_stopEvent;

	for (;;) {

		if (WaitForSingleObject(stopEvent, 0) == WAIT_OBJECT_0) {
			break;
		}

		stSession* sessionIter = sessionBegin;
		for (; sessionIter != sessionEnd; ++sessionIter) {

			if (sessionIter->_released == true) {
				continue;
			}

			LONGLONG nowTime = GetTickCount64();
			if (nowTime - sessionIter->_lastRecvTime > sessionIter->_timeoutTime) {

				server->onError(30000, (const wchar_t*)sessionIter->_sessionID);
			}
		}

		Sleep(sleepTime);

	}

	return 0;

}




template <>
CLockFreeQueue<CPacketPtr_Net>::stNode* CObjectFreeList<CLockFreeQueue<CPacketPtr_Net>::stNode>::_allocObject(const wchar_t* fileName, int line) {

	InterlockedIncrement((UINT*)&_usedCnt);

	stAllocNode<CLockFreeQueue<CPacketPtr_Net>::stNode>* allocNode;

	void* freePtr;
	void* nextPtr;

	unsigned __int64 nodeChangeCnt;

	_nodeChangeCnt += 1;

	do {
		// 원본 데이터 복사
		freePtr = (void*)_freePtr;
		nodeChangeCnt = _nodeChangeCnt;

		allocNode = ((stAllocNode<CLockFreeQueue<CPacketPtr_Net>::stNode>*)((unsigned __int64)freePtr & 0x000007FFFFFFFFFF));

		if (allocNode == nullptr) {

			// 추가 할당
			allocNode = (stAllocNode<CLockFreeQueue<CPacketPtr_Net>::stNode>*)
				malloc(sizeof(stAllocNode<CLockFreeQueue<CPacketPtr_Net>::stNode>));
			//ZeroMemory(allocNode, sizeof(stAllocNode<CLockFreeQueue<CPacketPtr_Net>::stNode>));
			//HeapAlloc(_heap, HEAP_ZERO_MEMORY, sizeof(stAllocNode<CLockFreeQueue<CPacketPointer>::stNode>));

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

	CLockFreeQueue<CPacketPtr_Net>::stNode* data = &allocNode->_data;


	return data;
}



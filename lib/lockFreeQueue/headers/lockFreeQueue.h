#pragma once

#include <Windows.h>

#include "objectFreeListTLS/headers/objectFreeListTLS.h"

static constexpr unsigned __int64 _useCntMask	= 0xFFFFF80000000000;
static constexpr unsigned __int64 _pointerMask	= 0x000007FFFFFFFFFF;

template <typename T>
class CLockFreeQueue{

public:

	CLockFreeQueue();

	void push(T data);
	bool pop(T* data);

	__int64 getSize();

	struct stNode {
		stNode() {
			_next = nullptr;
		}
		void* _next;
		T _data;
	};

private:

	void* _head;
	void* _tail;

	__int64 _size;

	unsigned __int64 _nodeChangeCnt;

	CObjectFreeList<stNode> _nodeFreeList;
	/*
	struct stLog {

		unsigned char _code;
		unsigned __int64 _logCnt;
		DWORD _threadID;

		void* _head;
		stNode* _headNode;
		stNode* _popNode;
		stNode* _freeNode;

		void* _tail;
		stNode* _tailNode;
		stNode* _newNode;

	};

	using LOG_INDEX_TYPE = unsigned short;
	stLog log[65536];
	unsigned __int64 _logCnt;
	*/
};

template <typename T>
CLockFreeQueue<T>::CLockFreeQueue():
	_nodeFreeList(false, false){

	stNode* node = _nodeFreeList.allocObject();
	_head = node;
	_tail = node;
	
	node->_next = nullptr;

	_nodeChangeCnt = 0;
	_size = 0;
}

template <typename T>
void CLockFreeQueue<T>::push(T data){
		
	stNode* newNode = _nodeFreeList.allocObject();
	newNode->_data = data;
	newNode->_next = nullptr;

	void* newPtr;
	void* tail;
	void* tailNextPtr;

	stNode* tailNode;
	/*
	{
		unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&_logCnt);
		LOG_INDEX_TYPE logIndex = (LOG_INDEX_TYPE)logCnt;
		stLog* logLine = &log[logIndex];

		logLine->_code = 0x11;
		logLine->_logCnt = logCnt;
		logLine->_threadID = GetCurrentThreadId();

		logLine->_head = _head;
		logLine->_headNode = nullptr;
		logLine->_popNode = nullptr;
		logLine->_freeNode = nullptr;
		
		logLine->_tail = _tail;
		logLine->_tailNode = nullptr;
		logLine->_newNode = newNode;
	}
	*/
	_nodeChangeCnt += 0x0000080000000000;
	newPtr = (void*)(_nodeChangeCnt | (unsigned __int64)newNode);

	int loopCnt = 0;

	do{
		
		// tail의 next가 null일 때까지 변경
		do{

			tail = _tail;
			tailNode = (stNode*)((unsigned __int64)tail & _pointerMask);
			tailNextPtr = tailNode->_next;

			if(tailNextPtr == nullptr){
				break;
			}
						

		}while(InterlockedCompareExchange64((LONG64*)&_tail, (LONG64)tailNextPtr, (LONG64)tail) != (LONG64)tail);


		tail = _tail;
		tailNode = (stNode*)((unsigned __int64)tail & _pointerMask);

		/*
		{
			unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&_logCnt);
			LOG_INDEX_TYPE logIndex = (LOG_INDEX_TYPE)logCnt;
			stLog* logLine = &log[logIndex];

			logLine->_code = 0x12;
			logLine->_logCnt = logCnt;
			logLine->_threadID = GetCurrentThreadId();

			logLine->_head = _head;
			logLine->_headNode = nullptr;
			logLine->_popNode = nullptr;
			logLine->_freeNode = nullptr;

			logLine->_tail = tail;
			logLine->_tailNode = tailNode;
			logLine->_newNode = newNode;
		}
		*/

	} while( InterlockedCompareExchange64((LONG64*)&tailNode->_next, (LONG64)newPtr, (LONG64)nullptr ) != (LONG64)nullptr );
	
	// tail로 저장했던 노드가 다른 스레드에 의해 해제, 재할당, 기존 tail의 next로 붙고 tail을 먼저 변경해버리면
	// 아래 interlock 실패하게됨 (_tail이 변경되었기에)
	InterlockedCompareExchange64((LONG64*)&_tail, (LONG64)newPtr, (LONG64)tail);
	
	InterlockedIncrement64((LONG64*)&_size);

	/*
	{
		unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&_logCnt);
		LOG_INDEX_TYPE logIndex = (LOG_INDEX_TYPE)logCnt;
		stLog* logLine = &log[logIndex];

		logLine->_code = 0x13;
		logLine->_logCnt = logCnt;
		logLine->_threadID = GetCurrentThreadId();

		logLine->_head = _head;
		logLine->_headNode = nullptr;
		logLine->_popNode = nullptr;
		logLine->_freeNode = nullptr;

		logLine->_tail = tail;
		logLine->_tailNode = tailNode;
		logLine->_newNode = newNode;
	}*/
}

template <typename T>
bool CLockFreeQueue<T>::pop(T* data){
	
	
	__int64 size = InterlockedDecrement64((LONG64*)&_size);
	if(size < 0){
		InterlockedIncrement64((LONG64*)&_size);
		return false;
	}

	void* popPtr;
	void* head;

	stNode* popNode;
	stNode* headNode;

	T* popNodeData;

	void* tail;
	void* tailNextPtr;

	stNode* tailNode;
	/*
	{
		unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&_logCnt);
		LOG_INDEX_TYPE logIndex = (LOG_INDEX_TYPE)logCnt;
		stLog* logLine = &log[logIndex];

		logLine->_code = 0x21;
		logLine->_logCnt = logCnt;
		logLine->_threadID = GetCurrentThreadId();

		logLine->_head = _head;
		logLine->_headNode = nullptr;
		logLine->_popNode = nullptr;
		logLine->_freeNode = nullptr;

		logLine->_tail = _tail;
		logLine->_tailNode = nullptr;
		logLine->_newNode = nullptr;
	}*/

	for(;;){
		
		head = _head;
		headNode = (stNode*)((unsigned __int64)head & _pointerMask);
			
		popPtr = headNode->_next;
		// tail의 next가 null일 때까지 변경
		do {

			tail = _tail;
			tailNode = (stNode*)((unsigned __int64)tail & _pointerMask);
			tailNextPtr = tailNode->_next;

			if (tailNextPtr == nullptr) {
				break;
			}

		} while (InterlockedCompareExchange64((LONG64*)&_tail, (LONG64)tailNextPtr, (LONG64)tail) != (LONG64)tail);

		
		popNode = (stNode*)((unsigned __int64)popPtr & _pointerMask);
		popNodeData = &popNode->_data;
		/*
		{
			unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&_logCnt);
			LOG_INDEX_TYPE logIndex = (LOG_INDEX_TYPE)logCnt;
			stLog* logLine = &log[logIndex];

			logLine->_code = 0x22;
			logLine->_logCnt = logCnt;
			logLine->_threadID = GetCurrentThreadId();

			logLine->_head = head;
			logLine->_headNode = headNode;
			logLine->_popNode = popNode;
			logLine->_freeNode = headNode;

			logLine->_tail = _tail;
			logLine->_tailNode = nullptr;
			logLine->_newNode = nullptr;
		}*/
		if(InterlockedCompareExchange64((LONG64*)&_head, (LONG64)popPtr, (LONG64)head) == (LONG64)head){
			break;
		}
	}

	*data = *popNodeData;
	
	_nodeFreeList.freeObject(headNode);
	/*
	{
		unsigned __int64 logCnt = InterlockedIncrement64((LONG64*)&_logCnt);
		LOG_INDEX_TYPE logIndex = (LOG_INDEX_TYPE)logCnt;
		stLog* logLine = &log[logIndex];

		logLine->_code = 0x23;
		logLine->_logCnt = logCnt;
		logLine->_threadID = GetCurrentThreadId();

		logLine->_head = head;
		logLine->_headNode = headNode;
		logLine->_popNode = popNode;
		logLine->_freeNode = headNode;

		logLine->_tail = _tail;
		logLine->_tailNode = nullptr;
		logLine->_newNode = nullptr;
	}*/
	return true;
}

template <typename T>
__int64 CLockFreeQueue<T>::getSize(){
	return _size;
}
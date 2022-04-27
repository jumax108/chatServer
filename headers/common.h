#pragma once

enum class JOB {

	USER_ENTER = -10,
	USER_LEAVE,

	REQ_LOGIN = 1,
	RES_LOGIN,
	REQ_MOVE_SECTOR,
	RES_MOVE_SECTOR,
	REQ_SEND_CHAT,
	RES_SEND_CHAT,
	REQ_HEARTBEAT
};

struct stJob {

	JOB _code;
	unsigned __int64 _sessionID;
	CPacketPointer _packet;

};

constexpr int SECTOR_SIZE = 50;
constexpr int ID_LEN = 20;
constexpr int NICKNAME_LEN = 20;
constexpr int SESSION_KEY_LEN = 64;

struct stSector;

struct stUser {

	unsigned __int64 _sessionID;
	stSector* _sector;
	__int64 _accountNo;
	WCHAR _id[ID_LEN];
	WCHAR _nickName[NICKNAME_LEN];
};

struct stSector {

	std::list<stUser*> _userList;
	std::vector<stSector*> _sendRange;
};

struct REQ_LOGIN {
	WORD _type;
	__int64 _accountNo;
	WCHAR _id[ID_LEN];
	WCHAR _nickName[NICKNAME_LEN];
	char _sessionKey[SESSION_KEY_LEN];
};

struct RES_LOGIN {
	WORD _type;
	BYTE _status;
	__int64 _accountNo;
};

struct REQ_MOVE_SECTOR {
	WORD _type;
	__int64 _accountNo;
	WORD _sectorX;
	WORD _sectorY;
};

struct RES_MOVE_SECTOR {
	WORD _type;
	__int64 _accountNo;
	WORD _sectorX;
	WORD _sectorY;
};

struct REQ_CHAT{
	WORD _type;
	__int64 _accountNo;
	WORD _msgLen;
};

struct RES_CHAT {
	WORD _type;
	__int64 _accountNo;
	WCHAR _id[ID_LEN];
	WCHAR nickName[NICKNAME_LEN];
	WORD _msgLen;
	//WCHAR msg[msgLen / 2]
};
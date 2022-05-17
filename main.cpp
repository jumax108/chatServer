
#include "headers/chatServer.h"

CDump dump;

CChatServer* chatServer;

int main() {

	chatServer = new CChatServer();

	//chatServer->init();

	chatServer->start(L"0.0.0.0", 6001, 4, 4, 17000, false, 5000, 5000, 10000);

	for (;;) {

		printf("\n\n\n");
		printf("sessionCnt: %d\n", chatServer->getSessionCount());
		printf("accept Total: %d\n", chatServer->getAcceptTotal());
		printf("accept TPS: %d\n", chatServer->getAcceptTPS());
		printf("RECV TPS: %d\n", chatServer->getRecvTPS());
		printf("SEND TPS: %d\n", chatServer->getSendTPS());
		printf("Packet Pool Usage: %d\n", CPacketPointer::getPacketPoolUsage());
		printf("Job Pool Usage: %d\n", chatServer->getJobCnt());
		
		Sleep(1000);

	}
	return 0;
}
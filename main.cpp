#include "headers/chatServer.h"

CDump dump;

int main() {

	CChatServer chatServer;

	chatServer.start(L"0.0.0.0", 6001, 3, 3, 5000, false, 5000, 5000);

	for (;;) {

		printf("\n\n\n");
		printf("sessionCnt: %d\n", chatServer.getSessionCount());
		printf("accept TPS: %d\n", chatServer.getAcceptTPS());
		printf("RECV TPS: %d\n", chatServer.getRecvTPS());
		printf("SEND TPS: %d\n", chatServer.getSendTPS());
		printf("Packet Pool Usage: %d\n", CPacketPointer::getPacketPoolUsage());
		Sleep(1000);

	}

	return 0;
}
#include "sectorCntServer.h"

unsigned __stdcall CSectorCntServer::updateFunc(void* args) {

	CSectorCntServer* server = (CSectorCntServer*)args;

	int delay = server->_delay;

	for (;;) {

		Sleep(delay);
		if (server->_clientID == 0) {
			continue;
		}

		for (int y = 0; y < SECTOR_SIZE; ++y) {
			for (int x = 0; x < SECTOR_SIZE; ++x) {

				CPacketPtr_Lan packet;
				packet << (short)y;
				packet << (short)x;
				packet << (int)server->_sectorUserList[y][x]->size();

				server->sendPacket(server->_clientID, packet);

			}
		}


	}

	return 0;

}
#include "TCPSocket.h"
#include "TNSController.h"

int main() {
	// ���� �ν���Ʈ
	TNSController server;

	// ��������
	server.startTNSServer();

	//TCPSocket server;
	//server.StartServer();
	return 0;
}

#include "FrontEndApp.h"

void main(void) {
	// ���� �ν���Ʈ
	TCPSocket server;

	// ��������
	server.StartServer();
	return;
	/*
	




	/*
	SOCKET s; //���� ��ũ����
	WSADATA wsaData;
	SOCKADDR_IN sin;


	TP = new TopicParser();

	if (WSAStartup(WINSOCK_VERSION, &wsaData) != 0) {
		printf("WSAStartup Failed. Error Code : %d\n", WSAGetLastError());
		//������ �߻��ϸ� WSAGetLastError() �Լ��� ���� ���� �ڵ带 Ȯ���� �� �ֽ��ϴ�.
		return;
	}

	puts(wsaData.szDescription); //����ϰ� �ִ� ���� ������ �����ݴϴ�.
	puts(wsaData.szSystemStatus); //����ϰ� �ִ� ������ ���¸� �����ݴϴ�. 

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //TCP/IP�� ���� ����

	if (s == INVALID_SOCKET) {
		printf("��Ĺ ���� ����, �����ڵ�:%d\n", WSAGetLastError());
		WSACleanup(); //WS2_32DLL�� ����� ����.
		return;
	}

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr("127.0.0.1"); //���� �ּ� ����
	sin.sin_port = htons(10000);      //��Ʈ ��ȣ ����

	if (connect(s, (struct sockaddr*)&sin, sizeof(sin)) != 0) {
		printf("���� ����, ���� �ڵ� = %u \n", WSAGetLastError());
		closesocket(s);
		WSACleanup();
		return;
	}


	
	puts("������ ����� �� �ֽ��ϴ�.");

	puts("222.122.84.200�� 80�� ��Ʈ�� ������ �����Ͽ����ϴ�.");


	vector<string> x = TP->split("A/BB/CCC/DDDD/EEEEEE");
	puts("��ū �м� ���");
	for (int i = 0; i < x.size(); i++) {
		cout << x.at(i) << endl;
		cout << x.at(i).c_str() << endl;

		if (send(s, x.at(i).c_str(), x.at(i).length(), 0) < 3) {
			printf("������ ���� ����, ���� �ڵ� = %u \n", WSAGetLastError());
			closesocket(s);
			WSACleanup();
			return;
		}
	}
	puts("��ū���� ���� ���α׷��� �����Ͽ����ϴ�.");

	puts("���� ����");
	
	if (closesocket(s) != 0) {
		printf("���� ���� ����, ���� �ڵ� = %u", WSAGetLastError());
		WSACleanup(); //WS2_32DLL�� ����� ����.
		return;
	}

	if (WSACleanup() != 0) {
		printf("WSACleanup����, ���� �ڵ� = %u\n", WSAGetLastError());
		return;
	}
	*/

}

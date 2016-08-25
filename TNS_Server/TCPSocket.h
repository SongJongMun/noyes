#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <winsock2.h> 
#include "stdafx.h"
#include "TopicNameTable.h"
#include "RequestTable.h"


using namespace std;
#pragma comment(lib,"wsock32.lib")
#pragma comment(lib, "ws2_32.lib")


class TCPSocket
{
public:
	TCPSocket();
	TNSN_ENTRY		TNSNDatagram;
	TopicNameTable*	TNTable;
	RequestTable *	RTable;

private:
	// ���� ����	
	WSADATA wsaData;
	SOCKET hServSock;
	SOCKADDR_IN servAddr;

	SOCKET hSockArray[WSA_MAXIMUM_WAIT_EVENTS]; //���� �ڵ�迭 - ���� ��û�� ���� ������ �����Ǵ� ������ �ڵ��� �� �迭�� ����. (�ִ�64)
	SOCKET hClntSock;
	SOCKADDR_IN clntAddr;

	WSAEVENT hEventArray[WSA_MAXIMUM_WAIT_EVENTS];	// �̺�Ʈ �迭 
	WSAEVENT newEvent;
	WSANETWORKEVENTS netEvents;

	int clntLen;
	int sockTotal;
	int index, i;
	char message[BUFSIZE];
	int strLen;

	//void CompressSockets(SOCKET* hSockArray, int omitIndex, int total);
	//void CompressEvents(WSAEVENT* hEventArray, int omitIndex, int total);

	//void ErrorHandling(char *message);
	//���� �Լ�
	void inputDummyEntryToTNTable();


public:
	int StartServer();
	void ResetTCPSocket();
	void Response();
	void SaveRequests(IN_ADDR ip, TNSN_ENTRY tnsData);
	void initialize();
};


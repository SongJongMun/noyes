
// �ش����� ����
#include <winsock2.h>
#include "TerminalTable.h"
#include "RequestTable.h"
#include "stdafx.h"

using namespace std;

// ws2_32.lib ��ũ
#pragma comment(lib, "ws2_32.lib")

#pragma once
class TCPSocket
{
public:
	TNSN_ENTRY TNSNDatagram;
	RequestTable *	RTable;
	TerminalTable *pubList;
	TerminalTable *subList;

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

	void CompressSockets(SOCKET* hSockArray, int omitIndex, int total);
	void CompressEvents(WSAEVENT* hEventArray, int omitIndex, int total);


public:
	TCPSocket();
	int StartServer();

	void ResetTCPSocket();
	void Response();
	void SaveRequests(IN_ADDR ip, TNSN_ENTRY tnsData);
	
};

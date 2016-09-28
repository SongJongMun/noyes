// �ش����� ����
#include <winsock2.h>
#include "stdafx.h"
#include "DBManager.h"
#include <deque>
#include <vector>

using namespace std;

// ws2_32.lib ��ũ
#pragma comment(lib, "ws2_32.lib")

#pragma once
class TCPSocket
{
public:
	DBManager *						DB;
	deque<pair<IN_ADDR, PDD_NODE>>	recvData;
	CRITICAL_SECTION				cs;

private:
	
	// ���� ����	
	
	WSADATA wsaData;
	int sockTotal;
	/*
	SOCKET hServSock;
	SOCKADDR_IN servAddr;

	SOCKET hSockArray[WSA_MAXIMUM_WAIT_EVENTS]; //���� �ڵ�迭 - ���� ��û�� ���� ������ �����Ǵ� ������ �ڵ��� �� �迭�� ����. (�ִ�64)
	SOCKET hClntSock;
	SOCKADDR_IN clntAddr;

	WSAEVENT hEventArray[WSA_MAXIMUM_WAIT_EVENTS];	// �̺�Ʈ �迭 
	WSAEVENT newEvent;
	WSANETWORKEVENTS netEvents;

	int clntLen;
	int index, i;
	char message[BUFSIZE];
	int strLen;
	*/


	void ResetTCPSocket();
	void inputDummyDataToDB();
public:
	TCPSocket();
	int StartServer();
};

// �ش����� ����
#include <winsock2.h>
#include "stdafx.h"
#include "ParticipantDataDistributor.h"
#include "TerminalTable.h"
#include "RequestTable.h"
#include "DBManager.h"

using namespace std;

// ws2_32.lib ��ũ
#pragma comment(lib, "ws2_32.lib")

#pragma once
class TCPSocket
{
public:
	TNSN_ENTRY					TNSNDatagram;
	RequestTable *				RTable;
	TerminalTable *				participantList;
	ParticipantDataDistributor*	distributor;
	DBManager * DB;

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


public:
	TCPSocket();
	int StartServer();

	void ResetTCPSocket();
	void Response();
	void SaveRequests(IN_ADDR ip, TNSN_ENTRY tnsData);
	void participantDataDistribute();
	void inputDummyData();
	void inputDummyDataToDB();
};

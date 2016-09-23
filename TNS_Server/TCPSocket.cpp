#include <process.h>
#include "TCPSocket.h"
#include "TopicNameTable.h"

static UINT WINAPI Sending(LPVOID p);
static UINT WINAPI Receiving(LPVOID p);
void ErrorHandling(char *message);

TCPSocket::TCPSocket()
{
	ResetTCPSocket();
}

int TCPSocket::StartServer()
{
	//Test Dummy Insert

	// ���� �ʱ�ȭ (������ 0, ���н� ���� �ڵ帮��)
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		ErrorHandling(MESSAGE_ERROR_WSA_STARTUP);
	}

	HANDLE recvThread = (HANDLE)_beginthreadex(NULL, 0, Receiving, (LPVOID)this, 0, NULL);
	HANDLE sendThread = (HANDLE)_beginthreadex(NULL, 0, Sending, (LPVOID)this, 0, NULL);

	this->initialize();
	TNTable->testShowAll();

	//Thread ���� ��, ���� Thead�� ���Ḧ �������� Loop
	while (true) {}

	CloseHandle(recvThread);
	CloseHandle(sendThread);

	return 0;
}

void TCPSocket::initialize() {
	int	test_type;
	TN_ENTRY TE;

	printf("[ INITIALIZE TNS SERVER ]\n\n");

	printf("***** Select inital type *****\n");
	printf("[1] Create Transmission Topic\n");
	printf("[2] Input  Dummy        Topic\n");
	printf("[others] Exit\n");
	printf("******************************\n");

	printf("input>");
	scanf("%d", &test_type);
	
	if(test_type == 2) {
		inputDummyEntryToTNTable();
	}

	while (test_type == 1) {
		printf("[ INPUT TOPIC NAME SPACE ENTRY ]\n\n");
		printf("Input Topic >");
		scanf("%s", &TE.TN_TOPIC);

		printf("Input Token >");
		scanf("%s", &TE.TN_TOKEN);

		printf("Input Level of Token >");
		scanf("%d", &TE.TN_LEVEL);

		printf("Input Next Zone >");
		scanf("%s", &TE.TN_NEXTZONE);

		TNTable->addEntry(TE);
		TNTable->testShowAll();

		fflush(stdin);

		printf("***** Select inital type *****\n");
		printf("[1] Create Transmission Topic\n");
		printf("[others] Exit\n");
		printf("******************************\n");

		printf("input>");
		scanf("%d", &test_type);
	}

	printf("[ Complete Inital TNS SERVER ]\n\n");
}

void TCPSocket::ResetTCPSocket() {
	this->sockTotal = 0;
	this->RTable = new RequestTable();
	this->TNTable = new TopicNameTable();
	this->recvQueue.clear();
}

/************************************
/*
/*	ErrorHandling
/*������� �� ����
*/
//void TCPSocket::ErrorHandling(char *message)
void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

///////////Test Function
void TCPSocket::inputDummyEntryToTNTable() {
	TN_ENTRY TE;

	TE.TN_LEVEL = 1;
	memcpy(TE.TN_NEXTZONE, IP, ADDRESS_SIZE);

	memcpy(TE.TN_TOKEN, TOPIC_NAME_1, sizeof(TOPIC_NAME_1));
	memcpy(TE.TN_TOPIC, TOPIC_NAME_FULL, sizeof(TOPIC_NAME_FULL));
	TNTable->addEntry(TE);

	TE.TN_LEVEL = 2;
	memcpy(TE.TN_TOKEN, TOPIC_NAME_2, sizeof(TOPIC_NAME_2));
	TNTable->addEntry(TE);

	TE.TN_LEVEL = 3;
	memcpy(TE.TN_TOKEN, TOPIC_NAME_3, sizeof(TOPIC_NAME_3));
	TNTable->addEntry(TE);

	TE.TN_LEVEL = 4;
	memcpy(TE.TN_TOKEN, TOPIC_NAME_4, sizeof(TOPIC_NAME_4));
	TNTable->addEntry(TE);

	TE.TN_LEVEL = 5;
	memcpy(TE.TN_TOKEN, TOPIC_NAME_5, sizeof(TOPIC_NAME_5));
	//memcpy(TE.TN_NEXTZONE, "192.168.0.22", ADDRESS_SIZE);

	memcpy(TE.TN_NEXTZONE, IP, ADDRESS_SIZE);
	TNTable->addEntry(TE);
	TNTable->testShowAll();
}

//�Ⱦ���
void TCPSocket::Response() {
	TN_ENTRY TE;
	//����� RequestTable���� �����ͼ� ������ �۽�
	if (RTable->isRequestExist()) {
		PR_NODE PN = RTable->getLastEntry();
		PDD_NODE entry = PN->key.REQUEST_DATA;

		if (entry.PDD_HEADER.MESSAGE_TYPE == MESSAGE_TYPE_REQUEST) {
			memcpy(&TE.TN_LEVEL, entry.PDD_DATA[0].PARTICIPANT_DATA, sizeof(int));
			memcpy(TE.TN_TOPIC, entry.PDD_DATA[0].PARTICIPANT_TOPIC, MAX_CHAR);
			
			if (TNTable->isEntryExist(TE)) {
				TNTable->getEntry(&TE);
				memcpy(entry.PDD_DATA[0].PARTICIPANT_DATA, TE.TN_NEXTZONE, sizeof(TE.TN_NEXTZONE));
				entry.PDD_HEADER.MESSAGE_TYPE = MESSAGE_TYPE_RESPONSE;
			} else {
				entry.PDD_HEADER.MESSAGE_TYPE = MESSAGE_TYPE_NOTEXIST;
			}

			cout << "Response TOKEN :" << TE.TN_TOKEN << "/" << TE.TN_NEXTZONE << endl;
			cout << "Response ADDRESS :" << entry.PDD_DATA[0].PARTICIPANT_DATA << endl;
		}

		cout << "=============================================" << endl;
	}
}

void TCPSocket::SaveRequests(IN_ADDR ip, PDD_NODE receiveData) {
	RTable->addEntry(ip, receiveData);
}

static UINT WINAPI Sending(LPVOID p) {
	TCPSocket *			tcpSocket = (TCPSocket*)p;
	SOCKET					clientSocket;
	SOCKADDR_IN		tempAddr;
	vector<string>			tokenArray;

	while (true) {
		Sleep(SLEEP_TIME_SHORT);
		if (tcpSocket->RTable->isRequestExist()) {
			TN_ENTRY TE;
			//����� RequestTable���� �����ͼ� ������ �۽�
			PR_NODE PN = tcpSocket->RTable->getLastEntry();
			PDD_NODE entry = PN->key.REQUEST_DATA;

			//�޼��� Ÿ�� REQUEST�� ���
			if (entry.PDD_HEADER.MESSAGE_TYPE == MESSAGE_TYPE_REQUEST) {
				tokenArray = tcpSocket->TNTable->splitTopic(entry.PDD_DATA[FirstIndex].PARTICIPANT_TOPIC);
				
				//���� �޽��� ���
				cout << "Request MSG" << endl;
				cout << "Request Topic :" << entry.PDD_DATA[FirstIndex].PARTICIPANT_TOPIC << endl;
				cout << "Request TokenLevel :" << entry.PDD_DATA[FirstIndex].PARTICIPANT_DATA << endl;
				cout << "Request TYPE :" << entry.PDD_HEADER.MESSAGE_TYPE << endl;

				TE.TN_LEVEL = atoi(entry.PDD_DATA[FirstIndex].PARTICIPANT_DATA);
				
				memcpy(TE.TN_TOPIC, entry.PDD_DATA[FirstIndex].PARTICIPANT_TOPIC, MAX_CHAR);
				memcpy(TE.TN_TOKEN, tokenArray.at(TE.TN_LEVEL - 1).c_str(), MAX_CHAR);

				//Entry���翩�ο� ���� Message Type ��������
				CheckEntry(tcpSocket, entry);
			}

			clientSocket = socket(PF_INET, SOCK_STREAM, 0);

			if (clientSocket == INVALID_SOCKET) {
				ErrorHandling("clientSocket() error");
			}

			memset(&tempAddr, FirstIndex, sizeof(tempAddr));
			tempAddr.sin_family = AF_INET;
			tempAddr.sin_addr.S_un.S_addr = inet_addr(inet_ntoa(PN->key.REQUEST_IP));
			tempAddr.sin_port = htons(FES_PORT);

			cout << inet_ntoa(PN->key.REQUEST_IP) << endl;

			if (connect(clientSocket, (SOCKADDR*)&tempAddr, sizeof(tempAddr)) == SOCKET_ERROR) {
				ErrorHandling("connect() error!");
			}

			send(clientSocket, (char*)&entry, sizeof(entry), 0);

			closesocket(clientSocket);

			cout << "Send" << endl;
			cout << "========================" << endl;
		}
	}
}

static void CheckEntry(TCPSocket * tcpSocket, PDD_NODE entry) {
	if (tcpSocket->TNTable->isEntryExist(TE)) {
		tcpSocket->TNTable->getEntry(&TE);
		memcpy(entry.PDD_DATA[FirstIndex].PARTICIPANT_DATA, TE.TN_NEXTZONE, sizeof(TE.TN_NEXTZONE));
		entry.PDD_HEADER.MESSAGE_TYPE = MESSAGE_TYPE_RESPONSE;
		printf("%s", entry.PDD_DATA[FirstIndex].PARTICIPANT_DATA);

		cout << "RESPONSE" << endl;
	}
	else {
		entry.PDD_HEADER.MESSAGE_TYPE = MESSAGE_TYPE_NOTEXIST;
		cout << "NOT EXIST" << endl;
	}
}

static SOCKET CreateSocket() {
	// ���� ���� (������ �ڵ���, ���н� "INVALID_SOCKET" ��ȯ)
	SOCKET servSock = socket(PF_INET, SOCK_STREAM, 0);

	// ���� ���� ���� ó��
	if (servSock == INVALID_SOCKET) {
		ErrorHandling(MESSAGE_ERROR_SOCKET);
	}

	return servSock;
}

static void BindingSocket(SOCKET servSocket){
	// ���� ����� ���� �⺻ ����
	SOCKADDR_IN servAddr;

	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(TNS_PORT);

	// �ּҿ� Port �Ҵ� (���ε�!!)
	if (bind(servSocket, (struct sockaddr *) &servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
		ErrorHandling(MESSAGE_ERROR_BIND);
	}

}

static void LinkingEvents(SOCKET servSock, int* sockNum, vector<SOCKET> * sockArray, vector<WSAEVENT> * eventArray) {
	// �̺�Ʈ �߻��� Ȯ�� (������ 0, ���н� "SOCKET_ERROR" ��ȯ)
	vector<SOCKET>::iterator sVec_it;
	vector<WSAEVENT>::iterator eVec_it;

	WSAEVENT newEvent = WSACreateEvent();
	if (WSAEventSelect(servSock, newEvent, FD_ACCEPT) == SOCKET_ERROR) {
		ErrorHandling(MESSAGE_ERROR_WSA_SELECT);
	}

	// ���� ��� ��û ���·��� ���� (��ȣ�� ���ö����� ���)
	if (listen(servSock, 5) == SOCKET_ERROR) {
		ErrorHandling(MESSAGE_ERROR_LISTEN);
	}
	
	sVec_it = sockArray->begin() + *sockNum;
	// ���� ���� �ڵ� ����
	sockArray->insert(sVec_it, servSock);

	eVec_it = eventArray->begin() + *sockNum;
	// �̺�Ʈ ������Ʈ �ڵ� ����
	eventArray->insert(eVec_it, newEvent);

	(*sockNum)++;
}

static UINT WINAPI Receiving(LPVOID p) {
	WSADATA wsaData;
	SOCKET hServSock;
	WSANETWORKEVENTS netEvents;

	vector<SOCKET> hSockArray; //���� �ڵ�迭 - ���� ��û�� ���� ������ �����Ǵ� ������ �ڵ��� �� �迭�� ����. (�ִ�64)
	vector<WSAEVENT> hEventArray; //�̺�Ʈ ����
	
	SOCKET * sockArray;
	WSAEVENT * eventArray;
	vector<SOCKET>::iterator sVec_it;
	vector<WSAEVENT>::iterator eVec_it;

	int sockTotal = 0;
	int index, i;
	char message[BUFSIZE];
	int strLen;

	//���� ����
	hServSock = CreateSocket();
	BindingSocket(hServSock);
	LinkingEvents(hServSock, &sockTotal, &hSockArray, &hEventArray);

	// ����
	while (true)	{
		sockArray = &hSockArray[FirstIndex];
		eventArray = &hEventArray[FirstIndex];

		// �̺�Ʈ ���� �����ϱ�(WSAWaitForMultipleEvents)
		index = WSAWaitForMultipleEvents(sockTotal, eventArray, FALSE, WSA_INFINITE, FALSE);
		index = index - WSA_WAIT_EVENT_0;

		for (i = index; i < sockTotal; i++) {
			index = WSAWaitForMultipleEvents(1, &eventArray[i], TRUE, FirstIndex, FALSE);

			if ((index == WSA_WAIT_FAILED || index == WSA_WAIT_TIMEOUT)) {
				continue;
			} else {
				index = i;
				WSAEnumNetworkEvents(sockArray[index], eventArray[index], &netEvents);
								
				if (netEvents.lNetworkEvents & FD_ACCEPT) { // �ʱ� ���� ��û�� ���.
					if (netEvents.iErrorCode[FD_ACCEPT_BIT] != 0) {
						puts("Accept Error");
						break;
					}
					OpenConnection(hSockArray, hEventArray, sVec_it, eVec_it, index);
					sockTotal++;
					
				} else if (netEvents.lNetworkEvents & FD_READ) { // ������ �����ؿ� ���.
					if (netEvents.iErrorCode[FD_READ_BIT] != 0) {
						puts("Read Error");
						break;
					}
					TransferData(p, &sockArray, index);
				} else if (netEvents.lNetworkEvents & FD_CLOSE) { 	// ���� ���� ��û�� ���.
					if (!CloseConnection(netEvents, hSockArray, hEventArray, sVec_it, eVec_it, &eventArray, index)) {
						break;
					}
					sockTotal--;
				}
			}//else
		}//for
	}//while

	// �Ҵ� ���� ���ҽ� ��ȯ.
	WSACleanup();

	return 0;
}

static bool OpenConnection(vector<SOCKET> &hSockArray, vector<SOCKET> &hEventArray, 
	vector<SOCKET>::iterator &sVec_it, vector<WSAEVENT>::iterator &eVec_it, int index) {
	WSAEVENT newEvent;

	SOCKET hClntSock;
	SOCKADDR_IN clntAddr;

	int clntLen;

	clntLen = sizeof(clntAddr);

	// ������ ���� (accept | ������ �����ڵ�, ���н� "INVALID_SOCKET" ��ȯ)
	hClntSock = accept(hSockArray.at(index), (SOCKADDR*)&clntAddr, &clntLen);

	// �̺�Ʈ Ŀ�� ������Ʈ ����(WSACreateEvent)
	newEvent = WSACreateEvent();

	// �̺�Ʈ �߻� ���� Ȯ��(WSAEventSelect)
	WSAEventSelect(hClntSock, newEvent, FD_READ | FD_CLOSE);

	sVec_it = hSockArray.begin() + sockTotal;
	eVec_it = hEventArray.begin() + sockTotal;

	hSockArray.insert(sVec_it, hClntSock);
	hEventArray.insert(eVec_it, newEvent);

	printf("���� ����� ������ �ڵ� %d \n", hClntSock);
	printf("vector size = %d\n", hSockArray.size());
	printf("array  size : %d\n", sockTotal);
}

static bool TransferData(LPVOID p, SOCKET * sockArray, int index) {
	TCPSocket * tcpSocket = (TCPSocket*)p;
	// �����͸� ���� (message�� ���� �����͸� ����)
	PDD_NODE	receiveData;
	struct sockaddr_in name;
	int len = sizeof(name);
	int strLen;

	strLen = recv(sockArray[index - WSA_WAIT_EVENT_0], (char*)&receiveData, sizeof(PDD_NODE), 0);

	if (getpeername(sockArray[index - WSA_WAIT_EVENT_0], (struct sockaddr *)&name, &len) != 0) {
		perror("getpeername Error");
	}

	if (strLen != -1) {
		//RequestTable�� �ϴ� ����
		tcpSocket->SaveRequests(name.sin_addr, receiveData);
	}
}

static bool CloseConnection(WSANETWORKEVENTS netEvents, vector<SOCKET> &hSockArray, vector<SOCKET> &hEventArray, 
	vector<SOCKET>::iterator &sVec_it, vector<WSAEVENT>::iterator &eVec_it, SOCKET * sockArray, WSAEVENT * eventArray, int index) {
	if (netEvents.iErrorCode[FD_CLOSE_BIT] != 0) {
		puts("Close Error");
		return false;
	}

	WSACloseEvent(eventArray[index]);

	// ���� ����
	closesocket(sockArray[index]);
	printf("���� �� ������ �ڵ� %d \n", sockArray[index]);

	// �迭 ����.
	printf("���� : %d\n", index);
	sVec_it = hSockArray.begin() + index;
	hSockArray.erase(sVec_it);

	eVec_it = hEventArray.begin() + index;
	hEventArray.erase(eVec_it);
}
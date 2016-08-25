#include <process.h>
#include "TCPSocket.h"
#include "TopicNameTable.h"

static UINT WINAPI sending(LPVOID p);
static UINT WINAPI receiving(LPVOID p);
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
		ErrorHandling("WSAStartup() error!");
	}

	HANDLE recvThread = (HANDLE)_beginthreadex(NULL, 0, receiving, (LPVOID)this, 0, NULL);
	HANDLE sendThread = (HANDLE)_beginthreadex(NULL, 0, sending, (LPVOID)this, 0, NULL);

	this->initialize();
	TNTable->testShowAll();

	while (1) {}

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
	
	if(test_type == 2)
		inputDummyEntryToTNTable();

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
}


/************************************
/*
/*	CompressSockets
/*
*/
//void TCPSocket::CompressSockets(SOCKET* hSockArray, int omitIndex, int total)
void CompressSockets(SOCKET* hSockArray, int omitIndex, int total)
{
	int i;

	for (i = omitIndex; i<total; i++)
	{
		hSockArray[i] = hSockArray[i + 1];
	}
}

/************************************
/*
/*	CompressEvents
/*
*/
//void TCPSocket::CompressEvents(WSAEVENT* hEventArray, int omitIndex, int total)
void CompressEvents(WSAEVENT* hEventArray, int omitIndex, int total)
{
	int i;

	for (i = omitIndex; i<total; i++)
	{
		hEventArray[i] = hEventArray[i + 1];
	}
}

/************************************
/*
/*	ErrorHandling
/*
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
	memcpy(TE.TN_NEXTZONE, "127.0.0.1", ADDRESS_SIZE);
	memcpy(TE.TN_TOKEN, "A", sizeof("A"));
	memcpy(TE.TN_TOPIC, "A/BB/CCC/DDDD/EEEEEE", sizeof("A/BB/CCC/DDDD/EEEEEE"));
	TNTable->addEntry(TE);
	TE.TN_LEVEL = 2;
	memcpy(TE.TN_TOKEN, "BB", sizeof("BB"));
	TNTable->addEntry(TE);
	TE.TN_LEVEL = 3;
	memcpy(TE.TN_TOKEN, "CCC", sizeof("CCC"));
	TNTable->addEntry(TE);
	TE.TN_LEVEL = 4;
	memcpy(TE.TN_TOKEN, "DDDD", sizeof("DDDD"));
	TNTable->addEntry(TE);
	TE.TN_LEVEL = 5;
	memcpy(TE.TN_TOKEN, "EEEEEE", sizeof("EEEEEE"));
	memcpy(TE.TN_NEXTZONE, "127.0.0.1", ADDRESS_SIZE);
	//memcpy(TE.TN_NEXTZONE, "192.168.0.22", ADDRESS_SIZE);
	TNTable->addEntry(TE);
	TNTable->testShowAll();
}

void TCPSocket::Response() {
	TN_ENTRY TE;
	//����� RequestTable���� �����ͼ� ������ �۽�
	if (RTable->isRequestExist()) {
		PR_NODE PN = RTable->getLastEntry();
		TNSN_ENTRY entry = PN->key.REQUEST_DATA;

		if (entry.TNSN_MESSAGETYPE == MESSAGE_TYPE_REQUEST) {
			//���� �޽��� ���
			//cout << "Request MSG" << endl;
			//cout << "Request Topic :" << entry.TNSN_TOPIC << endl;
			//cout << "Request Token :" << entry.TNSN_TOKEN << endl;
			//cout << "Request TokenLevel :" << entry.TNSN_TOKENLEVEL << endl;
			//cout << "Request TYPE :" << entry.TNSN_DATATYPE << endl;
			TE.TN_LEVEL = entry.TNSN_TOKENLEVEL;
			memcpy(TE.TN_TOPIC, entry.TNSN_TOPIC, sizeof(entry.TNSN_TOPIC));
			memcpy(TE.TN_TOKEN, entry.TNSN_TOKEN, sizeof(entry.TNSN_TOKEN));
			if (TNTable->isEntryExist(TE)) {
				TNTable->getEntry(&TE);
				memcpy(entry.TNSN_DATA, TE.TN_NEXTZONE, sizeof(TE.TN_NEXTZONE));
				entry.TNSN_MESSAGETYPE = MESSAGE_TYPE_RESPONSE;
			}
			else {
				entry.TNSN_MESSAGETYPE = MESSAGE_TYPE_NOTEXIST;
			}

			cout << "Response TOKEN :" << TE.TN_TOKEN << "/" << TE.TN_NEXTZONE << endl;
			cout << "Response ADDRESS :" << entry.TNSN_DATA << endl;
		}

		// ����(�����͸��� Ŭ���̾�Ʈ�� �ٽ� �����ͽ��)
		//send(hSockArray[index - WSA_WAIT_EVENT_0], (char*)&entry, sizeof(entry), 0);

		cout << "=============================================" << endl;
	}
}

void TCPSocket::SaveRequests(IN_ADDR ip, TNSN_ENTRY tnsData) {
	RTable->addEntry(ip, tnsData);
}


static UINT WINAPI sending(LPVOID p) {
	TCPSocket *		tcpSocket = (TCPSocket*)p;
	SOCKET			clientSocket;
	SOCKADDR_IN		tempAddr;

	while (1) {
		Sleep(10);
		if (tcpSocket->RTable->isRequestExist()) {
			TN_ENTRY TE;
			//����� RequestTable���� �����ͼ� ������ �۽�
			PR_NODE PN = tcpSocket->RTable->getLastEntry();
			TNSN_ENTRY entry = PN->key.REQUEST_DATA;

			if (entry.TNSN_MESSAGETYPE == MESSAGE_TYPE_REQUEST) {
				//���� �޽��� ���
				cout << "Request MSG" << endl;
				cout << "Request Topic :" << entry.TNSN_TOPIC << endl;
				cout << "Request Token :" << entry.TNSN_TOKEN << endl;
				cout << "Request TokenLevel :" << entry.TNSN_TOKENLEVEL << endl;
				cout << "Request TYPE :" << entry.TNSN_MESSAGETYPE << endl;

				TE.TN_LEVEL = entry.TNSN_TOKENLEVEL;
				memcpy(TE.TN_TOPIC, entry.TNSN_TOPIC, sizeof(entry.TNSN_TOPIC));
				memcpy(TE.TN_TOKEN, entry.TNSN_TOKEN, sizeof(entry.TNSN_TOKEN));
				
				if (tcpSocket->TNTable->isEntryExist(TE)) {
					tcpSocket->TNTable->getEntry(&TE);
					memcpy(entry.TNSN_DATA, TE.TN_NEXTZONE, sizeof(TE.TN_NEXTZONE));
					entry.TNSN_MESSAGETYPE = MESSAGE_TYPE_RESPONSE;
					printf("%s", entry.TNSN_DATA);

					cout << "RESPONSE" << endl;
				}
				else {
					entry.TNSN_MESSAGETYPE = MESSAGE_TYPE_NOTEXIST;
					cout << "NOT EXIST" << endl;
				}

				//cout << "Response TOKEN :" << TE.TN_TOKEN << "/" << TE.TN_NEXTZONE << endl;
				//cout << "Response ADDRESS :" << entry.TNSN_DATA << endl;
			}

			// ����(�����͸��� Ŭ���̾�Ʈ�� �ٽ� �����ͽ��)
			//send(PN->key.REQUEST_SOCEKT, (char*)&entry, sizeof(entry), 0);

			clientSocket = socket(PF_INET, SOCK_STREAM, 0);

			if (clientSocket == INVALID_SOCKET)
				ErrorHandling("clientSocket() error");

			memset(&tempAddr, 0, sizeof(tempAddr));
			tempAddr.sin_family = AF_INET;
			tempAddr.sin_addr.S_un.S_addr = inet_addr(inet_ntoa(PN->key.REQUEST_IP));
			tempAddr.sin_port = htons(FES_PORT);

			cout << inet_ntoa(PN->key.REQUEST_IP) << endl;

			if (connect(clientSocket, (SOCKADDR*)&tempAddr, sizeof(tempAddr)) == SOCKET_ERROR)
				ErrorHandling("connect() error!");

			send(clientSocket, (char*)&entry, sizeof(entry), 0);

			closesocket(clientSocket);

			cout << "Send" << endl;
			cout << "========================" << endl;
		}
	}
}

static SOCKET CreateSocket() {
	// ���� ���� (������ �ڵ���, ���н� "INVALID_SOCKET" ��ȯ)
	SOCKET servSock = socket(PF_INET, SOCK_STREAM, 0);

	// ���� ���� ���� ó��
	if (servSock == INVALID_SOCKET) {
		ErrorHandling("socket() error");
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
		ErrorHandling("bind() error");
	}

}
static void LinkingEvents(SOCKET servSock, int* sockNum, vector<SOCKET> * sockArray, vector<WSAEVENT> * eventArray) {
//static void LinkingEvents(SOCKET servSock, int* sockNum, SOCKET sockArray[WSA_MAXIMUM_WAIT_EVENTS], WSAEVENT eventArray[WSA_MAXIMUM_WAIT_EVENTS]) {
	// �̺�Ʈ �߻��� Ȯ�� (������ 0, ���н� "SOCKET_ERROR" ��ȯ)
	vector<SOCKET>::iterator sVec_it;
	vector<WSAEVENT>::iterator eVec_it;

	WSAEVENT newEvent = WSACreateEvent();
	if (WSAEventSelect(servSock, newEvent, FD_ACCEPT) == SOCKET_ERROR) {
		ErrorHandling("WSAEventSelect() error");
	}


	// ���� ��� ��û ���·��� ���� (��ȣ�� ���ö����� ���)
	if (listen(servSock, 5) == SOCKET_ERROR) {
		ErrorHandling("listen() error");
	}
	
	sVec_it = sockArray->begin() + *sockNum;
	// ���� ���� �ڵ� ����
	sockArray->insert(sVec_it, servSock);

	eVec_it = eventArray->begin() + *sockNum;
	// �̺�Ʈ ������Ʈ �ڵ� ����
	eventArray->insert(eVec_it, newEvent);

	(*sockNum)++;
}



static UINT WINAPI receiving(LPVOID p) {
	TCPSocket * tcpSocket = (TCPSocket*)p;

	TNSN_ENTRY	TNSNDatagram;

	WSADATA wsaData;
	SOCKET hServSock;

	//SOCKET hSockArray[WSA_MAXIMUM_WAIT_EVENTS];
	vector<SOCKET> hSockArray; //���� �ڵ�迭 - ���� ��û�� ���� ������ �����Ǵ� ������ �ڵ��� �� �迭�� ����. (�ִ�64)
	SOCKET hClntSock;
	SOCKADDR_IN clntAddr;

	//WSAEVENT hEventArray[WSA_MAXIMUM_WAIT_EVENTS];	// �̺�Ʈ �迭
	vector<WSAEVENT> hEventArray;
	WSAEVENT newEvent;
	WSANETWORKEVENTS netEvents;


	int clntLen;
	int sockTotal = 0;
	int index, i;
	char message[BUFSIZE];
	int strLen;

	struct sockaddr_in name;
	int len = sizeof(name);

	hServSock = CreateSocket();

	BindingSocket(hServSock);
	
	LinkingEvents(hServSock, &sockTotal, &hSockArray, &hEventArray);


	SOCKET * sockArray;
	WSAEVENT * eventArray;
	vector<SOCKET>::iterator sVec_it;
	vector<WSAEVENT>::iterator eVec_it;

	// ����
	while (1)
	{
		sockArray = &hSockArray[0];
		eventArray = &hEventArray[0];

		// �̺�Ʈ ���� �����ϱ�(WSAWaitForMultipleEvents)
		index = WSAWaitForMultipleEvents(sockTotal, eventArray, FALSE, WSA_INFINITE, FALSE);
		index = index - WSA_WAIT_EVENT_0;

		for (i = index; i < sockTotal; i++)
		{
			index = WSAWaitForMultipleEvents(1, &eventArray[i], TRUE, 0, FALSE);

			if ((index == WSA_WAIT_FAILED || index == WSA_WAIT_TIMEOUT)) continue;
			else
			{
				index = i;
				WSAEnumNetworkEvents(sockArray[index], eventArray[index], &netEvents);


				// �ʱ� ���� ��û�� ���.
				if (netEvents.lNetworkEvents & FD_ACCEPT)
				{
					if (netEvents.iErrorCode[FD_ACCEPT_BIT] != 0)
					{
						puts("Accept Error");
						break;
					}

					clntLen = sizeof(clntAddr);

					// ������ ���� (accept | ������ �����ڵ� ���н� "INVALID_SOCKET" ��ȯ)
					hClntSock = accept(hSockArray.at(index), (SOCKADDR*)&clntAddr, &clntLen);

					// �̺�Ʈ Ŀ�� ������Ʈ ����(WSACreateEvent)
					newEvent = WSACreateEvent();

					// �̺�Ʈ �߻� ���� Ȯ��(WSAEventSelect)
					WSAEventSelect(hClntSock, newEvent, FD_READ | FD_CLOSE);


					sVec_it = hSockArray.begin() + sockTotal;
					eVec_it = hEventArray.begin() + sockTotal;

					hSockArray.insert(sVec_it, hClntSock);
					hEventArray.insert(eVec_it, newEvent);
					//eventArray[sockTotal] = newEvent;
					//sockArray[sockTotal] = hClntSock;
					sockTotal++;

					printf("���� ����� ������ �ڵ� %d \n", hClntSock);
					printf("vector size = %d\n", hSockArray.size());
					printf("array  size : %d\n", sockTotal);
				}


				// ������ �����ؿ� ���.
				if (netEvents.lNetworkEvents & FD_READ)
				{
					if (netEvents.iErrorCode[FD_READ_BIT] != 0)
					{
						puts("Read Error");
						break;
					}

					//////////////////////////////////////////////////////////////////////////////////////////////////
					//////////////////////////////////////////////////////////////////////////////////////////////////
					//////////////////////////////////////////////////////////////////////////////////////////////////
					//
					//		���� �۾��� ���⼭ ���ϰ���..
					//
					// �����͸� ���� (message�� ���� �����͸� ����)
					strLen = recv(sockArray[index - WSA_WAIT_EVENT_0], (char*)&TNSNDatagram, sizeof(TNSNDatagram), 0);
					
					if (getpeername(sockArray[index - WSA_WAIT_EVENT_0], (struct sockaddr *)&name, &len) != 0) {
						perror("getpeername Error");
					}
					
					//cout << inet_ntoa(name.sin_addr) << endl;
					//cout << sizeof(inet_ntoa(name.sin_addr)) << endl;					
					
					if (strLen != -1) {
						//RequestTable�� �ϴ� ����
						tcpSocket->SaveRequests(name.sin_addr, TNSNDatagram);
						//Response();
					}

					//////////////////////////////////////////////////////////////////////////////////////////////////
					//////////////////////////////////////////////////////////////////////////////////////////////////
					//////////////////////////////////////////////////////////////////////////////////////////////////
				}


				// ���� ���� ��û�� ���.
				if (netEvents.lNetworkEvents & FD_CLOSE)
				{
					if (netEvents.iErrorCode[FD_CLOSE_BIT] != 0)
					{
						puts("Close Error");
						break;
					}

					WSACloseEvent(eventArray[index]);

					// ���� ����
					closesocket(sockArray[index]);
					printf("���� �� ������ �ڵ� %d \n", sockArray[index]);

					sockTotal--;

					// �迭 ����.
					printf("���� : %d\n", index);
					sVec_it = hSockArray.begin() + index;
					hSockArray.erase(sVec_it);

					eVec_it = hEventArray.begin() + index;
					hEventArray.erase(eVec_it);

					//CompressSockets(hSockArray, index, sockTotal);
					//CompressEvents(hEventArray, index, sockTotal);
				}

			}//else
		}//for
	}//while

	// �Ҵ� ���� ���ҽ� ��ȯ.
	WSACleanup();

	return 0;
}
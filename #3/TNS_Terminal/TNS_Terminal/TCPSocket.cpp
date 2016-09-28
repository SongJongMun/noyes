#include <process.h>
#include "TCPSocket.h"

static UINT WINAPI receiving(LPVOID p);
static UINT WINAPI storing(LPVOID p);

void ErrorHandling(char *message);
static SOCKET CreateSocket();
static void BindingSocket(SOCKET servSocket, int PORT);
static void sendPacket(char * TargetAddress, const char * Datagram, int SizeOfDatagram, int port);

TCPSocket::TCPSocket()
{
	ResetTCPSocket();
}

int TCPSocket::StartServer()
{
	// ���� �ʱ�ȭ (������ 0, ���н� ���� �ڵ帮��)
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		ErrorHandling("WSAStartup() error!");
	}

	HANDLE recvThread = (HANDLE)_beginthreadex(NULL, 0, receiving, (LPVOID)this, 0, NULL);
	HANDLE saveThread = (HANDLE)_beginthreadex(NULL, 0, storing, (LPVOID)this, 0, NULL);

	inputDummyDataToDB();
		
	//DB->showAllEntry();

	//TestDataInput - Complete

	//participantDataDistribute();

	while (true) {}

	CloseHandle(recvThread);
	CloseHandle(saveThread);
	DeleteCriticalSection(&cs);

	return 0;
}


void TCPSocket::ResetTCPSocket() {
	this->sockTotal				= 0;
	this->DB					= new DBManager();
	
	this->recvData.clear();
	InitializeCriticalSection(&cs);
}


void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

static UINT WINAPI storing(LPVOID p) {
	// ������ ��Ŷ�� �ϳ��� ó���ϴ� ������ �Լ�
	// �������� ��Ŷ�� ��� ������ 3���� ������ ��Ŷ ����
	// 1. Front-End�� �����ϴ� ó�� �Ϸ� / ���� �޽���
	// 2. ������ �ִ� �����ڵ鿡�� ���ο� �����ڰ� �߻������� �˸��� �޽���
	// 3. ���ο� �����ڿ��� ������ �����ϴ� �����ڵ��� ��� ������ �˷��ִ� �Ž���

	TCPSocket *					tcpSocket = (TCPSocket*)p;
	list<PDD_DATA>				distributeList;
	list<PDD_DATA>::iterator	it;
	int							NumOfParticipant;


	PPDD_NODE PDatagram			= (PPDD_NODE)malloc(sizeof(_PDD_NODE));		//������ �ִ� ���鿡�� ���ο� �������� ������ �����ϱ� ���� ���� �����ͱ׷�, ���� �Ѱ� ��Ʈ���� ����, Front-End���� ���޹��� ��Ŷ �״�� �ᵵ �ɵ�
	PPDD_NODE ReturnDatagram	= (PPDD_NODE)malloc(sizeof(_PDD_NODE));		//���� �߰��� ��忡�� ���� �����ڵ��� ������ �����ϱ� ���� ���� �����ͱ׷�, �������� ��Ʈ���� ����

	while (1) {
		memset(PDatagram, 0, sizeof(PDD_NODE));
		memset(ReturnDatagram, 0, sizeof(PDD_NODE));

		Sleep(10);

		if (tcpSocket->recvData.size() > 0) {
			//Queue���� ���� ������

			PDD_NODE entry = tcpSocket->recvData.back().second;

			if (entry.PDD_HEADER.MESSAGE_TYPE == MESSAGE_TYPE_SAVE || entry.PDD_HEADER.MESSAGE_TYPE == MESSAGE_TYPE_MODIFY || entry.PDD_HEADER.MESSAGE_TYPE == MESSAGE_TYPE_REMOVE) {
				if (!tcpSocket->DB->isTopicExist(entry.PDD_DATA[0].PARTICIPANT_TOPIC)) {
					printf("This Topic isn't Exist\n");
					entry.PDD_HEADER.MESSAGE_TYPE = MESSAGE_TYPE_NOTEXIST;
				} 
				else {
					ReturnDatagram->PDD_HEADER.MESSAGE_TYPE = entry.PDD_HEADER.MESSAGE_TYPE;
					memcpy(PDatagram, &entry, sizeof(PDD_NODE));
					PDatagram->PDD_HEADER.NUMBER_OF_PARTICIPANT = 1;

					distributeList = tcpSocket->DB->InsertEntry(entry.PDD_DATA[0]);
					// ���ο� �������� ������ �ݴ�Ǵ� ������ �����ڵ� ������ DB���� ������

					entry.PDD_HEADER.MESSAGE_TYPE += MESSAGE_OPTION_PLUS_DONE;
				}
			}
			else {
				puts("ERROR MSG TYPE");
				entry.PDD_HEADER.MESSAGE_TYPE = MESSAGE_TYPE_NOTEXIST;
			}

			// Return Result Packet To Front-End Server
			sendPacket(inet_ntoa(tcpSocket->recvData.back().first), (char *)&entry, sizeof(_PDD_NODE), FES_PORT);

			// NumOfParticipat
			NumOfParticipant = 0;

			for (it = distributeList.begin(); it != distributeList.end(); ++it) {
				printf("To : %s || Data : %s / %s / %s / %s / %d / %s\n", (*it).PARTICIPANT_IP, (*it).PARTICIPANT_NODE_TYPE == NODE_TYPE_PUB ? "PUB" : "SUB",
					(*it).PARTICIPANT_TOPIC, (*it).PARTICIPANT_DOMAIN_ID, (*it).PARTICIPANT_IP, (*it).PARTICIPANT_PORT, (*it).PARTICIPANT_DATA);
				
				ReturnDatagram->PDD_DATA[NumOfParticipant++] = (*it);

				// Send New Participant Data to Existing Participant
				sendPacket((*it).PARTICIPANT_IP, (const char *)PDatagram, sizeof(_PDD_NODE), DDS_PORT);	
			}

			//Send Existing Particiant Data To New Participant
			ReturnDatagram->PDD_HEADER.NUMBER_OF_PARTICIPANT = NumOfParticipant;
			
			sendPacket(entry.PDD_DATA[0].PARTICIPANT_IP, (const char *)ReturnDatagram, sizeof(_PDD_NODE), DDS_PORT);

			//Remove Receive Data
			EnterCriticalSection(&(tcpSocket->cs));
			tcpSocket->recvData.pop_back();
			LeaveCriticalSection(&(tcpSocket->cs));
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

static void BindingSocket(SOCKET servSocket, int PORT) {
	// ���� ����� ���� �⺻ ����
	SOCKADDR_IN servAddr;

	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(PORT);

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

	PDD_NODE receiveData;

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

	BindingSocket(hServSock, TERMINAL_PORT);

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
					strLen = recv(sockArray[index - WSA_WAIT_EVENT_0], (char*)&receiveData, sizeof(receiveData), 0);

					if (getpeername(sockArray[index - WSA_WAIT_EVENT_0], (struct sockaddr *)&name, &len) != 0) {
						perror("getpeername Error");
					}

					//cout << inet_ntoa(name.sin_addr) << endl;
					//cout << sizeof(inet_ntoa(name.sin_addr)) << endl;					

					if (strLen != -1) {
						//RequestTable�� �ϴ� ����
						EnterCriticalSection(&(tcpSocket->cs));
						tcpSocket->recvData.push_front(make_pair(name.sin_addr, receiveData));
						LeaveCriticalSection(&(tcpSocket->cs));
						//tcpSocket->SaveRequests(name.sin_addr, receiveData);
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

static void sendPacket(char * TargetAddress, const char * Datagram, int SizeOfDatagram, int port) {
	SOCKET				Socket;
	SOCKADDR_IN			tempAddr;

	Socket = socket(PF_INET, SOCK_STREAM, 0);

	if (Socket == INVALID_SOCKET)
		ErrorHandling("clientSocket() error");

	memset(&tempAddr, 0, sizeof(tempAddr));
	tempAddr.sin_family = AF_INET;
	tempAddr.sin_addr.S_un.S_addr = inet_addr(TargetAddress);
	tempAddr.sin_port = htons(port);

	if (connect(Socket, (SOCKADDR*)&tempAddr, sizeof(tempAddr)) == SOCKET_ERROR)
		ErrorHandling("connect() error!");

	send(Socket, Datagram, SizeOfDatagram, 0);

	closesocket(Socket);
}

void TCPSocket::inputDummyDataToDB() {
	PDD_DATA dummy, dummy2, dummy3;
	memcpy(dummy.PARTICIPANT_DOMAIN_ID, "DDS_1", sizeof("DDS_1"));
	memcpy(dummy.PARTICIPANT_DATA, "TEST_DDS_DATA", sizeof("TEST_DDS_DATA"));
	dummy.PARTICIPANT_NODE_TYPE = NODE_TYPE_PUB;
	//memcpy(dummy.TD_TOKEN, "BBBBBB", sizeof("BBBBBB"));
	memcpy(dummy.PARTICIPANT_TOPIC, "Z/XX/CCC/VVVV/BBBBBB", sizeof("Z/XX/CCC/VVVV/BBBBBB"));
	strcpy(dummy.PARTICIPANT_IP, "127.0.0.1");
	dummy.PARTICIPANT_PORT = 1000;

	memcpy(dummy2.PARTICIPANT_DOMAIN_ID, "DDS_1", sizeof("DDS_1"));
	memcpy(dummy2.PARTICIPANT_DATA, "TEST_DDS_DATA", sizeof("TEST_DDS_DATA"));
	dummy2.PARTICIPANT_NODE_TYPE = NODE_TYPE_PUB;
	//memcpy(dummy2.TD_TOKEN, "EEEEEE", sizeof("EEEEEE"));
	memcpy(dummy2.PARTICIPANT_TOPIC, "A/BB/CCC/DDDD/EEEEEE", sizeof("A/BB/CCC/DDDD/EEEEEE"));
	strcpy(dummy2.PARTICIPANT_IP, "127.0.0.1");
	dummy2.PARTICIPANT_PORT = 2000;

	memcpy(dummy3.PARTICIPANT_DOMAIN_ID, "DDS_1", sizeof("DDS_1"));
	memcpy(dummy3.PARTICIPANT_DATA, "TEST_DDS_DATA", sizeof("TEST_DDS_DATA"));
	dummy3.PARTICIPANT_NODE_TYPE = NODE_TYPE_PUB;
	//memcpy(dummy3.TD_TOKEN, "TTTTTT", sizeof("TTTTTT"));
	memcpy(dummy3.PARTICIPANT_TOPIC, "Q/WW/EEE/RRRR/TTTTTT", sizeof("Q/WW/EEE/RRRR/TTTTTT"));
	strcpy(dummy3.PARTICIPANT_IP, "127.0.0.1");
	dummy3.PARTICIPANT_PORT = 3000;

	this->DB->InsertEntry(dummy);
	this->DB->InsertEntry(dummy2);
	this->DB->InsertEntry(dummy3);

	dummy.PARTICIPANT_NODE_TYPE = NODE_TYPE_SUB;
	dummy2.PARTICIPANT_NODE_TYPE = NODE_TYPE_SUB;
	dummy3.PARTICIPANT_NODE_TYPE = NODE_TYPE_SUB;


	this->DB->InsertEntry(dummy);
	this->DB->InsertEntry(dummy2);
	this->DB->InsertEntry(dummy3);

	memcpy(dummy.PARTICIPANT_DOMAIN_ID, "DDS_2", sizeof("DDS_1"));
	memcpy(dummy.PARTICIPANT_DATA, "TEST_DDS_DATA", sizeof("TEST_DDS_DATA"));
	dummy.PARTICIPANT_NODE_TYPE = NODE_TYPE_PUB;
	//memcpy(dummy.TD_TOKEN, "BBBBBB", sizeof("BBBBBB"));
	memcpy(dummy.PARTICIPANT_TOPIC, "Z/XX/CCC/VVVV/BBBBBB", sizeof("Z/XX/CCC/VVVV/BBBBBB"));
	//dummy.PARTICIPANT_IP.S_un.S_addr = inet_addr("127.0.0.5");
	strcpy(dummy.PARTICIPANT_IP, "127.0.0.1");

	memcpy(dummy2.PARTICIPANT_DOMAIN_ID, "DDS_2", sizeof("DDS_1"));
	memcpy(dummy2.PARTICIPANT_DATA, "TEST_DDS_DATA", sizeof("TEST_DDS_DATA"));
	dummy2.PARTICIPANT_NODE_TYPE = NODE_TYPE_PUB;
	//memcpy(dummy2.TD_TOKEN, "EEEEEE", sizeof("EEEEEE"));
	memcpy(dummy2.PARTICIPANT_TOPIC, "A/BB/CCC/DDDD/EEEEEE", sizeof("A/BB/CCC/DDDD/EEEEEE"));
	//dummy2.PARTICIPANT_IP.S_un.S_addr = inet_addr("127.0.0.6");
	strcpy(dummy2.PARTICIPANT_IP, "127.0.0.1");

	memcpy(dummy3.PARTICIPANT_DOMAIN_ID, "DDS_2", sizeof("DDS_1"));
	memcpy(dummy3.PARTICIPANT_DATA, "TEST_DDS_DATA", sizeof("TEST_DDS_DATA"));
	dummy3.PARTICIPANT_NODE_TYPE = NODE_TYPE_PUB;
	//memcpy(dummy3.TD_TOKEN, "TTTTTT", sizeof("TTTTTT"));
	memcpy(dummy3.PARTICIPANT_TOPIC, "Q/WW/EEE/RRRR/TTTTTT", sizeof("Q/WW/EEE/RRRR/TTTTTT"));
	//dummy3.PARTICIPANT_IP.S_un.S_addr = inet_addr("127.0.0.7");
	strcpy(dummy3.PARTICIPANT_IP, "127.0.0.1");

	this->DB->InsertEntry(dummy);
	this->DB->InsertEntry(dummy2);
	this->DB->InsertEntry(dummy3);

	dummy.PARTICIPANT_NODE_TYPE = NODE_TYPE_SUB;
	dummy2.PARTICIPANT_NODE_TYPE = NODE_TYPE_SUB;
	dummy3.PARTICIPANT_NODE_TYPE = NODE_TYPE_SUB;

	this->DB->InsertEntry(dummy);
	this->DB->InsertEntry(dummy2);
	this->DB->InsertEntry(dummy3);

	memcpy(dummy.PARTICIPANT_DOMAIN_ID, "DDS_3", sizeof("DDS_1"));
	memcpy(dummy.PARTICIPANT_DATA, "TEST_DDS_DATA", sizeof("TEST_DDS_DATA"));
	dummy.PARTICIPANT_NODE_TYPE = NODE_TYPE_PUB;
	//memcpy(dummy.TD_TOKEN, "BBBBBB", sizeof("BBBBBB"));
	memcpy(dummy.PARTICIPANT_TOPIC, "Z/XX/CCC/VVVV/BBBBBB", sizeof("Z/XX/CCC/VVVV/BBBBBB"));
	//dummy.PARTICIPANT_IP.S_un.S_addr = inet_addr("127.0.0.7");
	strcpy(dummy.PARTICIPANT_IP, "127.0.0.1");

	memcpy(dummy2.PARTICIPANT_DOMAIN_ID, "DDS_3", sizeof("DDS_1"));
	memcpy(dummy2.PARTICIPANT_DATA, "TEST_DDS_DATA", sizeof("TEST_DDS_DATA"));
	dummy2.PARTICIPANT_NODE_TYPE = NODE_TYPE_PUB;
	//memcpy(dummy2.TD_TOKEN, "EEEEEE", sizeof("EEEEEE"));
	memcpy(dummy2.PARTICIPANT_TOPIC, "A/BB/CCC/DDDD/EEEEEE", sizeof("A/BB/CCC/DDDD/EEEEEE"));
	//dummy2.PARTICIPANT_IP.S_un.S_addr = inet_addr("127.0.0.6");
	strcpy(dummy2.PARTICIPANT_IP, "127.0.0.1");

	memcpy(dummy3.PARTICIPANT_DOMAIN_ID, "DDS_3", sizeof("DDS_1"));
	memcpy(dummy3.PARTICIPANT_DATA, "TEST_DDS_DATA", sizeof("TEST_DDS_DATA"));
	dummy3.PARTICIPANT_NODE_TYPE = NODE_TYPE_PUB;
	//memcpy(dummy3.TD_TOKEN, "TTTTTT", sizeof("TTTTTT"));
	memcpy(dummy3.PARTICIPANT_TOPIC, "Q/WW/EEE/RRRR/TTTTTT", sizeof("Q/WW/EEE/RRRR/TTTTTT"));
	//dummy3.PARTICIPANT_IP.S_un.S_addr = inet_addr("127.0.0.5");
	strcpy(dummy3.PARTICIPANT_IP, "127.0.0.1");

	this->DB->InsertEntry(dummy);
	this->DB->InsertEntry(dummy2);
	this->DB->InsertEntry(dummy3);

	dummy.PARTICIPANT_NODE_TYPE = NODE_TYPE_SUB;
	dummy2.PARTICIPANT_NODE_TYPE = NODE_TYPE_SUB;
	dummy3.PARTICIPANT_NODE_TYPE = NODE_TYPE_SUB;

	this->DB->InsertEntry(dummy);
	this->DB->InsertEntry(dummy2);
	this->DB->InsertEntry(dummy3);


	memcpy(dummy.PARTICIPANT_DOMAIN_ID, "DDS_1", sizeof("DDS_1"));
	memcpy(dummy.PARTICIPANT_DATA, "TEST_DDS_DATA", sizeof("TEST_DDS_DATA"));
	dummy.PARTICIPANT_NODE_TYPE = NODE_TYPE_SUB;
	memcpy(dummy.PARTICIPANT_TOPIC, "A/BB/CCC/DDDD/EEEEEE", sizeof("A/BB/CCC/DDDD/EEEEEE"));
	strcpy(dummy.PARTICIPANT_IP, "127.0.0.35");
	dummy.PARTICIPANT_PORT = 1000;
	this->DB->InsertEntry(dummy);

	printf("Input Complete\n");
}


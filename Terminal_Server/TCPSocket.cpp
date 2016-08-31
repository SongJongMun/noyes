#include <process.h>
#include "TCPSocket.h"
#include "TerminalTable.h"
#include "ParticipantDataDistributor.h"

static UINT WINAPI receiving(LPVOID p);
static UINT WINAPI storing(LPVOID p);
void ErrorHandling(char *message);
static SOCKET CreateSocket();
static void BindingSocket(SOCKET servSocket);

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

	//inputDummyData();
	inputDummyDataToDB();
	
	T_ENTRY dummy;
	memcpy(dummy.TD_DOMAIN, "DDS_1", sizeof("DDS_1"));
	memcpy(dummy.TD_DATA, "TEST_DDS_DATA", sizeof("TEST_DDS_DATA"));
	dummy.TD_PUBSUBTYPE = NODE_TYPE_SUB;
	memcpy(dummy.TD_TOPIC, "A/BB/CCC/DDDD/EEEEEE", sizeof("A/BB/CCC/DDDD/EEEEEE"));
	dummy.TD_PARTICIPANT_IP.S_un.S_addr = inet_addr("127.0.0.35");
	dummy.TD_PARTICIPANT_PORT = 1000;
	DB->InsertEntry(dummy);
	
	//DB->showAllEntry();

	printf("==============  LIST  ==============\n");
	this->participantList->test_showAllEntry();
	printf("====================================\n");

	//participantDataDistribute();
	while (true) {}

	CloseHandle(recvThread);
	CloseHandle(saveThread);

	return 0;
}


void TCPSocket::ResetTCPSocket() {
	this->sockTotal		= 0;
	this->participantList = new TerminalTable();
	this->RTable		= new RequestTable();
	this->distributor	= new ParticipantDataDistributor();
	this->distributor->setPubSubList(participantList);
	this->DB			= new DBManager();
}


void TCPSocket::initalize() {


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

void TCPSocket::SaveRequests(IN_ADDR ip, PDD_NODE receiveData) {
	RTable->addEntry(ip, receiveData);
}

static UINT WINAPI storing(LPVOID p) {
	TCPSocket *						tcpSocket = (TCPSocket*)p;
	SOCKET							clientSocket;
	SOCKADDR_IN						tempAddr;
	T_ENTRY							receiveData;
	list<PDD_DATA>   distributeList;
	list<PDD_DATA>::iterator   it;
	int counter;

	PPDD_NODE PDatagram = (PPDD_NODE)malloc(sizeof(_PDD_NODE));
	PPDD_NODE ReturnDatagram = (PPDD_NODE)malloc(sizeof(_PDD_NODE));
	SOCKET ClientSocket = CreateSocket();

	memset(&tempAddr, 0, sizeof(tempAddr));
	tempAddr.sin_family = AF_INET;

	while (1) {
		memset(PDatagram, 0, sizeof(PDD_NODE));
		memset(ReturnDatagram, 0, sizeof(PDD_NODE));

		Sleep(10);
		if (tcpSocket->RTable->isRequestExist()) {
			T_ENTRY TD;
			//����� RequestTable���� �����ͼ� ������ �۽�
			PR_NODE PN = tcpSocket->RTable->getLastEntry();

			//echo Data
			PDD_NODE entry = PN->key.REQUEST_DATA;

			if (!tcpSocket->DB->isTopicExist(entry.PDD_DATA[0].PARTICIPANT_TOPIC)) {
				printf("This Topic isn't Exist\n");
				continue;
			}

			if (entry.PDD_HEADER.MESSAGE_TYPE == MESSAGE_TYPE_SAVE || entry.PDD_HEADER.MESSAGE_TYPE == MESSAGE_TYPE_MODIFY || entry.PDD_HEADER.MESSAGE_TYPE == MESSAGE_TYPE_REMOVE) {
				receiveData.TD_PARTICIPANT_IP.S_un.S_addr	= inet_addr(entry.PDD_DATA[0].PARTICIPANT_IP);
				receiveData.TD_PUBSUBTYPE					= entry.PDD_DATA[0].PARTICIPANT_NODE_TYPE;
				receiveData.TD_PARTICIPANT_PORT				= entry.PDD_DATA[0].PARTICIPANT_PORT;
				
				memcpy(receiveData.TD_TOPIC,	entry.PDD_DATA[0].PARTICIPANT_TOPIC,	MAX_CHAR);
				memcpy(receiveData.TD_DOMAIN,	entry.PDD_DATA[0].PARTICIPANT_DOMAIN_ID,	MAX_CHAR);
				memcpy(receiveData.TD_DATA,		entry.PDD_DATA[0].PARTICIPANT_DATA,	MAX_DATA_SIZE);

				PDatagram->PDD_HEADER.MESSAGE_TYPE = entry.PDD_HEADER.MESSAGE_TYPE;
				
				if(entry.PDD_HEADER.MESSAGE_TYPE == MESSAGE_TYPE_SAVE)
					ReturnDatagram->PDD_HEADER.MESSAGE_TYPE = entry.PDD_HEADER.MESSAGE_TYPE;

				PDatagram->PDD_HEADER.NUMBER_OF_PARTICIPANT = 1;

				PDatagram->PDD_DATA[0].PARTICIPANT_NODE_TYPE = entry.PDD_DATA[0].PARTICIPANT_NODE_TYPE;
				strcpy(PDatagram->PDD_DATA[0].PARTICIPANT_TOPIC, entry.PDD_DATA[0].PARTICIPANT_TOPIC);
				strcpy(PDatagram->PDD_DATA[0].PARTICIPANT_DOMAIN_ID, entry.PDD_DATA[0].PARTICIPANT_DOMAIN_ID);
				strcpy(PDatagram->PDD_DATA[0].PARTICIPANT_IP ,entry.PDD_DATA[0].PARTICIPANT_IP);
				PDatagram->PDD_DATA[0].PARTICIPANT_PORT = entry.PDD_DATA[0].PARTICIPANT_PORT;
				memcpy(PDatagram->PDD_DATA[0].PARTICIPANT_DATA, entry.PDD_DATA[0].PARTICIPANT_DATA, MAX_DATA_SIZE);

				distributeList = tcpSocket->DB->InsertEntry(receiveData);
			}

			switch (entry.PDD_HEADER.MESSAGE_TYPE)
			{
			case MESSAGE_TYPE_SAVE:
				entry.PDD_HEADER.MESSAGE_TYPE = MESSAGE_TYPE_SAVEDONE;
				break;
			case MESSAGE_TYPE_REMOVE:
				entry.PDD_HEADER.MESSAGE_TYPE = MESSAGE_TYPE_REMOVEDONE;
				break;
			case MESSAGE_TYPE_MODIFY:
				entry.PDD_HEADER.MESSAGE_TYPE = MESSAGE_TYPE_MODIFYDONE;
				break;
			default:
				//while�� ������ ���� ��� �߰�
				puts("ERROR MSG TYPE");
				break;
			}

			//print list
			/*
			list<IN_ADDR>::iterator it;
			for (it = distributeList.begin(); it != distributeList.end(); ++it) {
				printf("To : %s\n", inet_ntoa(*it));
			}
			*/

			
			// ����(�����͸��� Ŭ���̾�Ʈ�� �ٽ� �����ͽ��)
			clientSocket = socket(PF_INET, SOCK_STREAM, 0);

			if (clientSocket == INVALID_SOCKET)
				ErrorHandling("clientSocket() error");

			memset(&tempAddr, 0, sizeof(tempAddr));
			tempAddr.sin_family = AF_INET;
			tempAddr.sin_addr.S_un.S_addr = inet_addr(inet_ntoa(PN->key.REQUEST_IP));
			tempAddr.sin_port = htons(FES_PORT);

			//cout << inet_ntoa(PN->key.REQUEST_IP) << endl;

			if (connect(clientSocket, (SOCKADDR*)&tempAddr, sizeof(tempAddr)) == SOCKET_ERROR)
				ErrorHandling("connect() error!");

			send(clientSocket, (char*)&entry, sizeof(entry), 0);

			closesocket(clientSocket);

			cout << "Send" << endl;
			cout << "========================" << endl;


			tempAddr.sin_port = htons(DDS_PORT);
			//������ ���� ����
			counter = 0;

			for (it = distributeList.begin(); it != distributeList.end(); ++it) {
				printf("To : %s || Data : %s / %s / %s / %s / %d / %s\n", (*it).PARTICIPANT_IP, (*it).PARTICIPANT_NODE_TYPE == NODE_TYPE_PUB ? "PUB" : "SUB",
					(*it).PARTICIPANT_TOPIC, (*it).PARTICIPANT_DOMAIN_ID, (*it).PARTICIPANT_IP, (*it).PARTICIPANT_PORT, (*it).PARTICIPANT_DATA);
				
				ReturnDatagram->PDD_DATA[counter++] = (*it);

				ClientSocket = CreateSocket();

				tempAddr.sin_addr.S_un.S_addr = inet_addr((*it).PARTICIPANT_IP);

				printf("Sending to %s..... \n", (*it).PARTICIPANT_IP);

				if (connect(ClientSocket, (SOCKADDR*)&tempAddr, sizeof(tempAddr)) == SOCKET_ERROR)
					ErrorHandling("connect() error!");

				send(ClientSocket, (const char *)PDatagram, sizeof(_PDD_NODE), 0);

				closesocket(ClientSocket);

				printf("Complete ..... \n");


				/*
				if (entry.TNSN_NODETYPE != (*it).second.PARTICIPANT_NODE_TYPE) {
					PDatagram->PDD_DATA[counter] = (*it).second;
					counter++;

					PDatagram->PDD_HEADER.NUMBER_OF_PARTICIPANT = counter + 1;

					tempAddr.sin_addr.S_un.S_addr = inet_addr(inet_ntoa((*it).first));
				}
				else {
					ClientSocket = CreateSocket();

					if (connect(ClientSocket, (SOCKADDR*)&tempAddr, sizeof(tempAddr)) == SOCKET_ERROR)
						ErrorHandling("connect() error!");

					send(ClientSocket, (const char *)PDatagram, sizeof(_PDD_NODE), 0);

					closesocket(ClientSocket);
				}

				ClientSocket = CreateSocket();

				tempAddr.sin_addr.S_un.S_addr = inet_addr(inet_ntoa((*it).first));

				printf("Sending to %s..... \n", inet_ntoa((*it).first));

				if (connect(ClientSocket, (SOCKADDR*)&tempAddr, sizeof(tempAddr)) == SOCKET_ERROR)
					ErrorHandling("connect() error!");

				send(ClientSocket, (const char *)PDatagram, sizeof(_PDD_NODE), 0);

				closesocket(ClientSocket);

				printf("Complete ..... \n");
				*/
			}

			ReturnDatagram->PDD_HEADER.NUMBER_OF_PARTICIPANT = counter;

			ClientSocket = CreateSocket();

			tempAddr.sin_addr.S_un.S_addr = inet_addr(entry.PDD_DATA[0].PARTICIPANT_IP);

			printf("Sending to %s..... \n", entry.PDD_DATA[0].PARTICIPANT_IP);

			if (connect(ClientSocket, (SOCKADDR*)&tempAddr, sizeof(tempAddr)) == SOCKET_ERROR)
				ErrorHandling("connect() error!");

			send(ClientSocket, (const char *)ReturnDatagram, sizeof(_PDD_NODE), 0);

			closesocket(ClientSocket);

			printf("Complete ..... \n");
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
						tcpSocket->SaveRequests(name.sin_addr, receiveData);
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


void TCPSocket::inputDummyDataToDB() {
	T_ENTRY dummy, dummy2, dummy3;
	memcpy(dummy.TD_DOMAIN, "DDS_1", sizeof("DDS_1"));
	memcpy(dummy.TD_DATA, "TEST_DDS_DATA", sizeof("TEST_DDS_DATA"));
	dummy.TD_PUBSUBTYPE = NODE_TYPE_PUB;
	//memcpy(dummy.TD_TOKEN, "BBBBBB", sizeof("BBBBBB"));
	memcpy(dummy.TD_TOPIC, "Z/XX/CCC/VVVV/BBBBBB", sizeof("Z/XX/CCC/VVVV/BBBBBB"));
	dummy.TD_PARTICIPANT_IP.S_un.S_addr = inet_addr("127.0.0.1");
	dummy.TD_PARTICIPANT_PORT = 1000;

	memcpy(dummy2.TD_DOMAIN, "DDS_1", sizeof("DDS_1"));
	memcpy(dummy2.TD_DATA, "TEST_DDS_DATA", sizeof("TEST_DDS_DATA"));
	dummy2.TD_PUBSUBTYPE = NODE_TYPE_PUB;
	//memcpy(dummy2.TD_TOKEN, "EEEEEE", sizeof("EEEEEE"));
	memcpy(dummy2.TD_TOPIC, "A/BB/CCC/DDDD/EEEEEE", sizeof("A/BB/CCC/DDDD/EEEEEE"));
	dummy2.TD_PARTICIPANT_IP.S_un.S_addr = inet_addr("127.0.0.1");
	dummy2.TD_PARTICIPANT_PORT = 2000;

	memcpy(dummy3.TD_DOMAIN, "DDS_1", sizeof("DDS_1"));
	memcpy(dummy3.TD_DATA, "TEST_DDS_DATA", sizeof("TEST_DDS_DATA"));
	dummy3.TD_PUBSUBTYPE = NODE_TYPE_PUB;
	//memcpy(dummy3.TD_TOKEN, "TTTTTT", sizeof("TTTTTT"));
	memcpy(dummy3.TD_TOPIC, "Q/WW/EEE/RRRR/TTTTTT", sizeof("Q/WW/EEE/RRRR/TTTTTT"));
	dummy3.TD_PARTICIPANT_IP.S_un.S_addr = inet_addr("127.0.0.1");
	dummy3.TD_PARTICIPANT_PORT = 3000;

	this->DB->InsertEntry(dummy);
	this->DB->InsertEntry(dummy2);
	this->DB->InsertEntry(dummy3);

	dummy.TD_PUBSUBTYPE = NODE_TYPE_SUB;
	dummy2.TD_PUBSUBTYPE = NODE_TYPE_SUB;
	dummy3.TD_PUBSUBTYPE = NODE_TYPE_SUB;


	this->DB->InsertEntry(dummy);
	this->DB->InsertEntry(dummy2);
	this->DB->InsertEntry(dummy3);

	memcpy(dummy.TD_DOMAIN, "DDS_2", sizeof("DDS_1"));
	memcpy(dummy.TD_DATA, "TEST_DDS_DATA", sizeof("TEST_DDS_DATA"));
	dummy.TD_PUBSUBTYPE = NODE_TYPE_PUB;
	//memcpy(dummy.TD_TOKEN, "BBBBBB", sizeof("BBBBBB"));
	memcpy(dummy.TD_TOPIC, "Z/XX/CCC/VVVV/BBBBBB", sizeof("Z/XX/CCC/VVVV/BBBBBB"));
	//dummy.TD_PARTICIPANT_IP.S_un.S_addr = inet_addr("127.0.0.5");
	dummy.TD_PARTICIPANT_IP.S_un.S_addr = inet_addr("127.0.0.1");

	memcpy(dummy2.TD_DOMAIN, "DDS_2", sizeof("DDS_1"));
	memcpy(dummy2.TD_DATA, "TEST_DDS_DATA", sizeof("TEST_DDS_DATA"));
	dummy2.TD_PUBSUBTYPE = NODE_TYPE_PUB;
	//memcpy(dummy2.TD_TOKEN, "EEEEEE", sizeof("EEEEEE"));
	memcpy(dummy2.TD_TOPIC, "A/BB/CCC/DDDD/EEEEEE", sizeof("A/BB/CCC/DDDD/EEEEEE"));
	//dummy2.TD_PARTICIPANT_IP.S_un.S_addr = inet_addr("127.0.0.6");
	dummy2.TD_PARTICIPANT_IP.S_un.S_addr = inet_addr("127.0.0.1");

	memcpy(dummy3.TD_DOMAIN, "DDS_2", sizeof("DDS_1"));
	memcpy(dummy3.TD_DATA, "TEST_DDS_DATA", sizeof("TEST_DDS_DATA"));
	dummy3.TD_PUBSUBTYPE = NODE_TYPE_PUB;
	//memcpy(dummy3.TD_TOKEN, "TTTTTT", sizeof("TTTTTT"));
	memcpy(dummy3.TD_TOPIC, "Q/WW/EEE/RRRR/TTTTTT", sizeof("Q/WW/EEE/RRRR/TTTTTT"));
	//dummy3.TD_PARTICIPANT_IP.S_un.S_addr = inet_addr("127.0.0.7");
	dummy3.TD_PARTICIPANT_IP.S_un.S_addr = inet_addr("127.0.0.1");

	this->DB->InsertEntry(dummy);
	this->DB->InsertEntry(dummy2);
	this->DB->InsertEntry(dummy3);

	dummy.TD_PUBSUBTYPE = NODE_TYPE_SUB;
	dummy2.TD_PUBSUBTYPE = NODE_TYPE_SUB;
	dummy3.TD_PUBSUBTYPE = NODE_TYPE_SUB;

	this->DB->InsertEntry(dummy);
	this->DB->InsertEntry(dummy2);
	this->DB->InsertEntry(dummy3);

	memcpy(dummy.TD_DOMAIN, "DDS_3", sizeof("DDS_1"));
	memcpy(dummy.TD_DATA, "TEST_DDS_DATA", sizeof("TEST_DDS_DATA"));
	dummy.TD_PUBSUBTYPE = NODE_TYPE_PUB;
	//memcpy(dummy.TD_TOKEN, "BBBBBB", sizeof("BBBBBB"));
	memcpy(dummy.TD_TOPIC, "Z/XX/CCC/VVVV/BBBBBB", sizeof("Z/XX/CCC/VVVV/BBBBBB"));
	//dummy.TD_PARTICIPANT_IP.S_un.S_addr = inet_addr("127.0.0.7");
	dummy.TD_PARTICIPANT_IP.S_un.S_addr = inet_addr("127.0.0.1");

	memcpy(dummy2.TD_DOMAIN, "DDS_3", sizeof("DDS_1"));
	memcpy(dummy2.TD_DATA, "TEST_DDS_DATA", sizeof("TEST_DDS_DATA"));
	dummy2.TD_PUBSUBTYPE = NODE_TYPE_PUB;
	//memcpy(dummy2.TD_TOKEN, "EEEEEE", sizeof("EEEEEE"));
	memcpy(dummy2.TD_TOPIC, "A/BB/CCC/DDDD/EEEEEE", sizeof("A/BB/CCC/DDDD/EEEEEE"));
	//dummy2.TD_PARTICIPANT_IP.S_un.S_addr = inet_addr("127.0.0.6");
	dummy2.TD_PARTICIPANT_IP.S_un.S_addr = inet_addr("127.0.0.1");

	memcpy(dummy3.TD_DOMAIN, "DDS_3", sizeof("DDS_1"));
	memcpy(dummy3.TD_DATA, "TEST_DDS_DATA", sizeof("TEST_DDS_DATA"));
	dummy3.TD_PUBSUBTYPE = NODE_TYPE_PUB;
	//memcpy(dummy3.TD_TOKEN, "TTTTTT", sizeof("TTTTTT"));
	memcpy(dummy3.TD_TOPIC, "Q/WW/EEE/RRRR/TTTTTT", sizeof("Q/WW/EEE/RRRR/TTTTTT"));
	//dummy3.TD_PARTICIPANT_IP.S_un.S_addr = inet_addr("127.0.0.5");
	dummy3.TD_PARTICIPANT_IP.S_un.S_addr = inet_addr("127.0.0.1");

	this->DB->InsertEntry(dummy);
	this->DB->InsertEntry(dummy2);
	this->DB->InsertEntry(dummy3);

	dummy.TD_PUBSUBTYPE = NODE_TYPE_SUB;
	dummy2.TD_PUBSUBTYPE = NODE_TYPE_SUB;
	dummy3.TD_PUBSUBTYPE = NODE_TYPE_SUB;

	this->DB->InsertEntry(dummy);
	this->DB->InsertEntry(dummy2);
	this->DB->InsertEntry(dummy3);

	printf("Input Complete\n");
}
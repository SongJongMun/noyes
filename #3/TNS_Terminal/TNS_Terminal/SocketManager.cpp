#include "stdafx.h"
#include "SocketManager.h"

static UINT WINAPI receiving(LPVOID p);

SocketManager::SocketManager()
{
}


SocketManager::~SocketManager()
{

}


void 							SocketManager::startRecevingThread() {
	this->recvThread = (HANDLE)_beginthreadex(NULL, 0, receiving, (LPVOID)this, 0, NULL);
}

void 							SocketManager::closeRecevingThread() {
	CloseHandle(recvThread);
}

void			 				SocketManager::setCriticalSection(CRITICAL_SECTION * criticlaSection) {
	this->cs = *criticlaSection;
}

void  							SocketManager::getRecevingDEQUE(deque<pair<IN_ADDR, PDD_NODE>> * dq) {
	*dq = this->recvData;
}

void							SocketManager::setUIView() {

}

void							SocketManager::sendPacket(char * TargetAddress, const char * Datagram, int SizeOfDatagram, int port) {
	SOCKET				Socket;
	//SOCKADDR_IN			tempAddr;

	Socket = socket(PF_INET, SOCK_STREAM, 0);

	if (Socket == INVALID_SOCKET)
		puts("clientSocket() error");

	setSocketTargetAddress(&Socket, TargetAddress, port);

	/*
	memset(&tempAddr, 0, sizeof(tempAddr));
	tempAddr.sin_family = AF_INET;
	tempAddr.sin_addr.S_un.S_addr = inet_addr(TargetAddress);
	tempAddr.sin_port = htons(port);

	if (connect(Socket, (SOCKADDR*)&tempAddr, sizeof(tempAddr)) == SOCKET_ERROR)
	puts("connect() error!");
	*/
	send(Socket, Datagram, SizeOfDatagram, 0);

	closesocket(Socket);
}

SOCKET 							SocketManager::createSocket() {
	// ���� ���� (������ �ڵ���, ���н� "INVALID_SOCKET" ��ȯ)
	SOCKET servSock = socket(PF_INET, SOCK_STREAM, 0);

	// ���� ���� ���� ó��
	if (servSock == INVALID_SOCKET) {
		puts("socket() error");
	}

	return servSock;
}
void							SocketManager::setSocketTargetAddress(SOCKET * s, char * TargetAddress, int port) {
	SOCKADDR_IN			tempAddr;

	memset(&tempAddr, 0, sizeof(tempAddr));
	tempAddr.sin_family = AF_INET;
	tempAddr.sin_addr.S_un.S_addr = inet_addr(TargetAddress);
	tempAddr.sin_port = htons(port);

	if (connect(*s, (SOCKADDR*)&tempAddr, sizeof(tempAddr)) == SOCKET_ERROR)
		puts("connect() error!");
}

void							SocketManager::bindingSocket(SOCKET servSocket, int PORT) {
	// ���� ����� ���� �⺻ ����
	SOCKADDR_IN servAddr;

	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(PORT);

	// �ּҿ� Port �Ҵ� (���ε�!!)
	if (bind(servSocket, (struct sockaddr *) &servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
		puts("bind() error");
	}
}

void 							SocketManager::linkingEvents(SOCKET servSock, int* sockNum, vector<SOCKET> * sockArray, vector<WSAEVENT> * eventArray) {
	//static void LinkingEvents(SOCKET servSock, int* sockNum, SOCKET sockArray[WSA_MAXIMUM_WAIT_EVENTS], WSAEVENT eventArray[WSA_MAXIMUM_WAIT_EVENTS]) {
	// �̺�Ʈ �߻��� Ȯ�� (������ 0, ���н� "SOCKET_ERROR" ��ȯ)
	vector<SOCKET>::iterator sVec_it;
	vector<WSAEVENT>::iterator eVec_it;

	WSAEVENT newEvent = WSACreateEvent();

	if (WSAEventSelect(servSock, newEvent, FD_ACCEPT) == SOCKET_ERROR) {
		puts("WSAEventSelect() error");
	}


	// ���� ��� ��û ���·��� ���� (��ȣ�� ���ö����� ���)
	if (listen(servSock, 5) == SOCKET_ERROR) {
		puts("listen() error");
	}

	sVec_it = sockArray->begin() + *sockNum;
	// ���� ���� �ڵ� ����
	sockArray->insert(sVec_it, servSock);

	eVec_it = eventArray->begin() + *sockNum;
	// �̺�Ʈ ������Ʈ �ڵ� ����
	eventArray->insert(eVec_it, newEvent);

	(*sockNum)++;
}

SOCKET							SocketManager::getRecevingSocket(int port, int* sockNum, vector<SOCKET> * sockArray, vector<WSAEVENT> * eventArray) {
	SOCKET s = this->createSocket();

	this->bindingSocket(s, port);
	this->linkingEvents(s, sockNum, sockArray, eventArray);

	return s;
}

//receving
bool							SocketManager::isWsaWaitERROR() {
	return false;
}

bool							SocketManager::acceptConnection() {
	return false;
}

bool							SocketManager::closeConnection() {
	return false;
}

void							SocketManager::insertSocketEvent(int * idx, vector<SOCKET> * SocketArray, vector<WSAEVENT> * EventArray, SOCKET s, WSAEVENT Event) {
	vector<SOCKET>::iterator sVec_it;
	vector<WSAEVENT>::iterator eVec_it;

	sVec_it = SocketArray->begin() + *idx;
	eVec_it = EventArray->begin() + *idx;

	SocketArray->insert(sVec_it, s);
	EventArray->insert(eVec_it, Event);

	*idx++;
}

void							SocketManager::deleteSocketEvent(int * idx, vector<SOCKET> * SocketArray, vector<WSAEVENT> * EventArray) {
	vector<SOCKET>::iterator sVec_it;
	vector<WSAEVENT>::iterator eVec_it;

	sVec_it = SocketArray->begin() + *idx;
	eVec_it = EventArray->begin() + *idx;

	SocketArray->erase(sVec_it);
	EventArray->erase(eVec_it);
}

void SocketManager::AcceptProc(int idx, int * totalSocket, vector<SOCKET> * SocketArray, vector<WSAEVENT> * EventArray) {
	SOCKADDR_IN clntAddr;
	int clntLen = sizeof(clntAddr);

	// ������ ���� (accept | ������ �����ڵ� ���н� "INVALID_SOCKET" ��ȯ)
	SOCKET hClntSock = accept(SocketArray->at(idx), (SOCKADDR*)&clntAddr, &clntLen);

	// �̺�Ʈ Ŀ�� ������Ʈ ����(WSACreateEvent)
	WSAEVENT newEvent = WSACreateEvent();

	// �̺�Ʈ �߻� ���� Ȯ��(WSAEventSelect)
	WSAEventSelect(hClntSock, newEvent, FD_READ | FD_CLOSE);

	insertSocketEvent(totalSocket, SocketArray, EventArray, hClntSock, newEvent);

	//Test Print Code
	printf("���� ����� ������ �ڵ� %d \n", hClntSock);
	printf("vector size = %d\n", SocketArray->size());
	printf("array  size : %d\n", *totalSocket);
}

PDD_NODE SocketManager::ReadProc(int idx, int * strLen, vector<SOCKET> * SocketArray, vector<WSAEVENT> * EventArray) {

	PDD_NODE			receiveData;
	struct sockaddr_in	name;
	int					len		= sizeof(name);

	*strLen = recv(SocketArray->at(idx - WSA_WAIT_EVENT_0), (char*)&receiveData, sizeof(receiveData), 0);

	if (getpeername(SocketArray->at(idx - WSA_WAIT_EVENT_0), (struct sockaddr *)&name, &len) != 0) {
		perror("getpeername Error");
	}				
	
	return receiveData;
}

void SocketManager::CloseProc(int idx, int * totalSocket, vector<SOCKET> * SocketArray, vector<WSAEVENT> * EventArray) {

	WSACloseEvent(EventArray->at(idx));

	// ���� ����
	closesocket(SocketArray->at(idx));
	printf("���� �� ������ �ڵ� %d \n", SocketArray->at(idx));

	*totalSocket--;

	// �迭 ����.
	printf("���� : %d\n", idx);

	deleteSocketEvent(&idx, SocketArray, EventArray);
}

void SocketManager::savePacketToDeque(CRITICAL_SECTION * cs, deque<pair<IN_ADDR, PDD_NODE>>	* dq, PDD_NODE pn, sockaddr_in addr) {
	EnterCriticalSection(cs);
	dq->push_front(make_pair(addr.sin_addr, pn));
	LeaveCriticalSection(cs);
}

static							 UINT WINAPI receiving(LPVOID p) {
	SocketManager		* manager = (SocketManager*)p;

	PDD_NODE			receiveData;
	SOCKET				hServSock;

	vector<SOCKET>		hSockArray;		//���� �ڵ�迭 - ���� ��û�� ���� ������ �����Ǵ� ������ �ڵ��� �� �迭�� ����. (�ִ�64)
	vector<WSAEVENT>	hEventArray;	//EventArray
	WSANETWORKEVENTS	netEvents;		//Event Flags

	struct sockaddr_in	name;

	int					sockTotal = 0;
	int					index, i, strLen;
	int					len = sizeof(name);

	hServSock = manager->getRecevingSocket(TERMINAL_PORT, &sockTotal, &hSockArray, &hEventArray);

	/*
	hServSock = manager->CreateSocket();

	BindingSocket(hServSock, TERMINAL_PORT);

	LinkingEvents(hServSock, &sockTotal, &hSockArray, &hEventArray);
	*/

	SOCKET * sockArray;
	WSAEVENT * eventArray;
	vector<SOCKET>::iterator sVec_it;
	vector<WSAEVENT>::iterator eVec_it;

	// ����
	while (true)
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
					if (netEvents.iErrorCode[FD_ACCEPT_BIT] != 0) {
						puts("Accept Error");
						break;
					}

					manager->AcceptProc(index, &sockTotal, &hSockArray, &hEventArray);
					
				} else if (netEvents.lNetworkEvents & FD_READ) {
					if (netEvents.iErrorCode[FD_READ_BIT] != 0) {
						puts("Read Error");
						break;
					}

					receiveData = manager->ReadProc(index, &strLen,&hSockArray, &hEventArray);

					if (strLen != -1) {
						manager->savePacketToDeque(&(manager->cs), &(manager->recvData), receiveData, name);
					}

				} else if (netEvents.lNetworkEvents & FD_CLOSE) {
					
					if (netEvents.iErrorCode[FD_CLOSE_BIT] != 0) {
						puts("Close Error");
						break;
					}

					manager->CloseProc(index, &sockTotal, &hSockArray, &hEventArray);
				}
			}
		}
	}
	
	WSACleanup();

	return 0;
}

static							 UINT WINAPI receiving2(LPVOID p) {
	SocketManager * manager = (SocketManager*)p;

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

	hServSock = manager->getRecevingSocket(TERMINAL_PORT, &sockTotal, &hSockArray, &hEventArray);

	/*
	hServSock = manager->CreateSocket();

	BindingSocket(hServSock, TERMINAL_PORT);

	LinkingEvents(hServSock, &sockTotal, &hSockArray, &hEventArray);
	*/

	SOCKET * sockArray;
	WSAEVENT * eventArray;
	vector<SOCKET>::iterator sVec_it;
	vector<WSAEVENT>::iterator eVec_it;

	// ����
	while (true)
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
						//EnterCriticalSection(&(manager->cs));
						//tcpSocket->recvData.push_front(make_pair(name.sin_addr, receiveData));
						//LeaveCriticalSection(&(tcpSocket->cs));
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
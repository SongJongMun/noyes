#include "SocketManager.h"
#include "TNSNheader.h"

static UINT WINAPI Receiving(LPVOID p);

SocketManager::SocketManager()
{

}


SocketManager::~SocketManager()
{

}

void 							SocketManager::StartRecevingThread() {
	this->recvThread = (HANDLE)_beginthreadex(NULL, 0, Receiving, (LPVOID)this, 0, NULL);
}

void 							SocketManager::CloseRecevingThread() {
	CloseHandle(recvThread);
}

void			 				SocketManager::SetCriticalSection(CRITICAL_SECTION * criticlaSection) {
	this->cs = *criticlaSection;
}

void  							SocketManager::GetRecevingDEQUE(deque<PDD_NODE> ** dq) {
	*dq = &(this->recvData);
}

void							SocketManager::SendPacket(char * TargetAddress, const char * Datagram, int SizeOfDatagram, int port) {
	SOCKET					Socket;
	//SOCKADDR_IN			tempAddr;

	Socket = socket(PF_INET, SOCK_STREAM, 0);

	if (Socket == INVALID_SOCKET) {
		puts("clientSocket() error");
		printf("ERROR CODE : %d\n", WSAGetLastError());
	}

	SetSocketTargetAddress(&Socket, TargetAddress, port);
	//puts("a");
	send(Socket, Datagram, SizeOfDatagram, 0);
	//puts("b");
	closesocket(Socket);
}

SOCKET 							SocketManager::CreateSocket() {
	// ���� ���� (������ �ڵ���, ���н� "INVALID_SOCKET" ��ȯ)
	SOCKET servSock = socket(PF_INET, SOCK_STREAM, 0);

	// ���� ���� ���� ó��
	if (servSock == INVALID_SOCKET) {
		puts("socket() error");
	}

	return servSock;
}
void							SocketManager::SetSocketTargetAddress(SOCKET * s, char * TargetAddress, int port) {
	SOCKADDR_IN			tempAddr;

	memset(&tempAddr, 0, sizeof(tempAddr));
	tempAddr.sin_family = AF_INET;
	tempAddr.sin_addr.S_un.S_addr = inet_addr(TargetAddress);
	tempAddr.sin_port = htons(port);

	if (connect(*s, (SOCKADDR*)&tempAddr, sizeof(tempAddr)) == SOCKET_ERROR) {
		puts("connect() error!");
		printf("%d", WSAGetLastError());
	}
}

void							SocketManager::BindingSocket(SOCKET servSocket, int port) {
	// ���� ����� ���� �⺻ ����
	SOCKADDR_IN servAddr;

	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(port);

	// �ּҿ� Port �Ҵ� (���ε�!!)
	
	if (bind(servSocket, (struct sockaddr *) &servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
		puts("bind() error");
	}
}

void 							SocketManager::LinkingEvents(SOCKET servSock, int* sockNum, vector<SOCKET> * sockArray, vector<WSAEVENT> * eventArray) {
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

SOCKET							SocketManager::GetRecevingSocket(int port, int* sockNum, vector<SOCKET> * sockArray, vector<WSAEVENT> * eventArray) {
	SOCKET s = this->CreateSocket();

	this->BindingSocket(s, port);
	this->LinkingEvents(s, sockNum, sockArray, eventArray);

	return s;
}

void							SocketManager::InsertSocketEvent(int * idx, vector<SOCKET> * SocketArray, vector<WSAEVENT> * EventArray, SOCKET s, WSAEVENT Event) {
	vector<SOCKET>::iterator	sVec_it = SocketArray->begin() + *idx;
	vector<WSAEVENT>::iterator	eVec_it = EventArray->begin() + *idx;

	SocketArray->insert(sVec_it, s);
	EventArray->insert(eVec_it, Event);

	(*idx)++;
}

void							SocketManager::DeleteSocketEvent(int * idx, vector<SOCKET> * SocketArray, vector<WSAEVENT> * EventArray) {
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

	InsertSocketEvent(totalSocket, SocketArray, EventArray, hClntSock, newEvent);

	//Test Print Code
	//printf("���� ����� ������ �ڵ� %d \n", hClntSock);
	//printf("vector size = %d\n", SocketArray->size());
	//printf("array  size : %d\n", *totalSocket);
}

PDD_NODE SocketManager::ReadProc(int idx, int * strLen, vector<SOCKET> * SocketArray, vector<WSAEVENT> * EventArray, sockaddr_in * addr) {

	PDD_NODE			receiveData;
	int					len = sizeof(*addr);

	*strLen = recv(SocketArray->at(idx - WSA_WAIT_EVENT_0), (char*)&receiveData, sizeof(receiveData), 0);

	if (getpeername(SocketArray->at(idx - WSA_WAIT_EVENT_0), (struct sockaddr *)addr, &len) != 0) {
		perror("getpeername Error");
	}

	return receiveData;
}

void	SocketManager::CloseProc(int idx, int * totalSocket, vector<SOCKET> * SocketArray, vector<WSAEVENT> * EventArray) {

	WSACloseEvent(EventArray->at(idx));

	// ���� ����
	closesocket(SocketArray->at(idx));
	//printf("���� �� ������ �ڵ� %d \n", SocketArray->at(idx));

	(*totalSocket)--;

	// �迭 ����.
	//printf("���� : %d\n", idx);

	DeleteSocketEvent(&idx, SocketArray, EventArray);
}

void SocketManager::SavePacketToDeque(CRITICAL_SECTION * cs, deque<PDD_NODE> * dq, PDD_NODE * pn, sockaddr_in addr) {
	EnterCriticalSection(cs);
	dq->push_front(*pn);
	LeaveCriticalSection(cs);
}


static				UINT WINAPI Receiving(LPVOID p) {
	SocketManager		* manager = (SocketManager*)p;
	
	WSADATA				wsaData;
	PDD_NODE			receiveData;
	SOCKET				hServSock;

	vector<SOCKET>		hSockArray;		//���� �ڵ�迭 - ���� ��û�� ���� ������ �����Ǵ� ������ �ڵ��� �� �迭�� ����. (�ִ�64)
	vector<WSAEVENT>	hEventArray;	//EventArray
	WSANETWORKEVENTS	netEvents;		//Event Flags

	struct sockaddr_in	name;

	int					sockTotal = 0;
	int					index, i, strLen;
	int					len = sizeof(name);


	// ���� �ʱ�ȭ (������ 0, ���н� ���� �ڵ帮��)
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		puts("WSAStartup() error!");
	}

	hServSock = manager->GetRecevingSocket(PORT, &sockTotal, &hSockArray, &hEventArray);

	SOCKET * sockArray;
	WSAEVENT * eventArray;

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

				}

				if (netEvents.lNetworkEvents & FD_READ) {

					if (netEvents.iErrorCode[FD_READ_BIT] != 0) {
						puts("Read Error");
						break;
					}

					receiveData = manager->ReadProc(index, &strLen, &hSockArray, &hEventArray, &name);

					//printf("%s", inet_ntoa(name.sin_addr));

					if (strLen != -1) {
						manager->SavePacketToDeque(&(manager->cs), &(manager->recvData), &receiveData, name);
					}

					//puts("Saved");

				}

				if (netEvents.lNetworkEvents & FD_CLOSE) {
					
					if (netEvents.iErrorCode[FD_CLOSE_BIT] != 0) {
						puts("Close Error");
						break;
					}

					manager->CloseProc(index, &sockTotal, &hSockArray, &hEventArray);
				}
			}
		}
	}

	puts("close");

	WSACleanup();

	return 0;
}

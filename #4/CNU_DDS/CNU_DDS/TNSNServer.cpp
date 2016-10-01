#include "TNSNheader.h"
#include "TNSNServer.h"

namespace CNU_DDS
{
	CRITICAL_SECTION	cs;
	HANDLE				recvThread;
	HANDLE				saveThread;
	deque<PDD_NODE>		recvQueue;
	int					TNS_TYPE;

	void InitalizeTNSServer() {
		printf("[ INITIALIZE < Front-End / DDS > SERVER ]\n\n");

		printf("***** Select Program type *****\n");
		printf("[1] TNS Front-End Server\n");
		printf("[2] DDS Extended  Node\n");
		printf("[others] Exit\n");
		printf("******************************\n");
		printf("input>");
		scanf("%d", &TNS_TYPE);

		if (TNS_TYPE == MESSAGE_TYPE_REQUEST || TNS_TYPE == MESSAGE_TYPE_RESPONSE)
			StartServer();
		else
			return;
	}

	void ErrorHandling(char *message)
	{
		printf("%s", message);
		//fputs(message, stderr);
		//fputc('\n', stderr);
		//exit(1);
	}

	void StartServer() {
		recvQueue.clear();

		InitializeCriticalSection(&cs);

		recvThread = (HANDLE)_beginthreadex(NULL, FIRST_INDEX, Receiving, (LPVOID)&recvQueue, 0, NULL);
		saveThread = (HANDLE)_beginthreadex(NULL, FIRST_INDEX, Storing, (LPVOID)&recvQueue, 0, NULL);

		while (true) {};
		EndServer();
	}

	void EndServer() {
		CloseHandle(saveThread);
		CloseHandle(recvThread);

		DeleteCriticalSection(&cs);

		recvQueue.clear();
	}

	SOCKET CreateSocket() {
		// ���� ���� (������ �ڵ���, ���н� "INVALID_SOCKET" ��ȯ)
		SOCKET servSock = socket(PF_INET, SOCK_STREAM, 0);

		// ���� ���� ���� ó��
		if (servSock == INVALID_SOCKET) {
			ErrorHandling(MESSAGE_ERROR_SOCKET);
		}

		return servSock;
	}

	void BindingSocket(SOCKET servSocket) {
		// ���� ����� ���� �⺻ ����
		SOCKADDR_IN servAddr;

		servAddr.sin_family = AF_INET;
		servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servAddr.sin_port = htons(PORT);

		// �ּҿ� Port �Ҵ� (���ε�!!)
		if (bind(servSocket, (struct sockaddr *) &servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
			ErrorHandling(MESSAGE_ERROR_BIND);
		}

	}

	void LinkingEvents(SOCKET servSock, int* sockNum, vector<SOCKET> * sockArray, vector<WSAEVENT> * eventArray) {
		//static void LinkingEvents(SOCKET servSock, int* sockNum, SOCKET sockArray[WSA_MAXIMUM_WAIT_EVENTS], WSAEVENT eventArray[WSA_MAXIMUM_WAIT_EVENTS]) {
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
		WSADATA				wsaData;
		SOCKET				hServSock;
		
		vector<SOCKET>		hSockArray; //���� �ڵ�迭 - ���� ��û�� ���� ������ �����Ǵ� ������ �ڵ��� �� �迭�� ����. (�ִ�64)
		vector<WSAEVENT>	hEventArray;
		
		WSANETWORKEVENTS	netEvents;

		int					sockTotal = 0;
		int					index, i;

		// ���� �ʱ�ȭ (������ 0, ���н� ���� �ڵ帮��)
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			ErrorHandling(MESSAGE_ERROR_WSA_STARTUP);
		}

		hServSock = CreateSocket();
		BindingSocket(hServSock);
		LinkingEvents(hServSock, &sockTotal, &hSockArray, &hEventArray);

		SOCKET * sockArray;
		WSAEVENT * eventArray;

		printf("START RECEIVING TEST\n");

		while (true) {
			sockArray = &hSockArray[0];
			eventArray = &hEventArray[0];

			// �̺�Ʈ ���� �����ϱ�(WSAWaitForMultipleEvents)
			index = WSAWaitForMultipleEvents(sockTotal, eventArray, FALSE, WSA_INFINITE, FALSE);
			index = index - WSA_WAIT_EVENT_0;

			for (i = index; i < sockTotal; i++) {
				index = WSAWaitForMultipleEvents(1, &eventArray[i], TRUE, 0, FALSE);

				if ((index == WSA_WAIT_FAILED || index == WSA_WAIT_TIMEOUT)) {
					continue;
				} else {
					index = i;
					WSAEnumNetworkEvents(sockArray[index], eventArray[index], &netEvents);

					// �ʱ� ���� ��û�� ���.
					if (netEvents.lNetworkEvents & FD_ACCEPT) {
						if (netEvents.iErrorCode[FD_ACCEPT_BIT] != 0) {
							puts(MESSAGE_ERROR_ACCEPT);
							break;
						} else {
							OpenConnection(hSockArray, hEventArray, index, sockTotal);
							sockTotal++;
						}
					}

					// ������ �����ؿ� ���.
					if (netEvents.lNetworkEvents & FD_READ)
					{
						if (netEvents.iErrorCode[FD_READ_BIT] != 0)
						{
							puts(MESSAGE_ERROR_READ);
							break;
						} else {
							TransferData(p, sockArray, index);
						}
					}

					// ���� ���� ��û�� ���.
					if (netEvents.lNetworkEvents & FD_CLOSE)
					{
						if (netEvents.iErrorCode[FD_CLOSE_BIT] != 0)
						{
							puts(MESSAGE_ERROR_CLOSE);
							break;
						} else {
							CloseConnection(hSockArray, hEventArray, sockArray, eventArray, index);
							sockTotal--;
						}
					}
				}//else
			}//for
		}//while

		 // �Ҵ� ���� ���ҽ� ��ȯ.
		WSACleanup();

		DeleteCriticalSection(&cs);
	}

	void OpenConnection(vector<SOCKET> hSockArray, vector<WSAEVENT>	hEventArray, int index, int sockTotal) {
		SOCKADDR_IN			clntAddr;
		int					clntLen;

		SOCKET				hClntSock;
		WSAEVENT			newEvent;

		vector<SOCKET>::iterator sVec_it;
		vector<WSAEVENT>::iterator eVec_it;

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

		printf("���� ����� ������ �ڵ� %d \n", hClntSock);
		printf("vector size = %d\n", hSockArray.size());
		printf("array  size : %d\n", sockTotal);
	}

	void TransferData(LPVOID p, SOCKET * sockArray, int index) {
		deque<PDD_NODE>		*recvQueue = (deque<PDD_NODE>*)p;
		PDD_NODE			receiveData;
		int					strLen;

		struct	sockaddr_in name;
		int len = sizeof(name);

		// �����͸� ���� (message�� ���� �����͸� ����)
		strLen = recv(sockArray[index - WSA_WAIT_EVENT_0], (char*)&receiveData, sizeof(_PDD_NODE), 0);

		if (getpeername(sockArray[index - WSA_WAIT_EVENT_0], (struct sockaddr *)&name, &len) != 0) {
			perror("getpeername Error");
		}

		EnterCriticalSection(&cs);
		recvQueue->push_back(receiveData);
		LeaveCriticalSection(&cs);
	}

	void CloseConnection(vector<SOCKET> hSockArray, vector<WSAEVENT>	hEventArray, SOCKET * sockArray, WSAEVENT * eventArray, int index) {
		vector<SOCKET>::iterator sVec_it;
		vector<WSAEVENT>::iterator eVec_it;

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

	static UINT WINAPI Storing(LPVOID p) {
		PTNSP_NODE					PTNSP;
		PDD_NODE					sendDatagram;
		SOCKADDR_IN					tempAddr;
		SOCKET						clientSocket;

		deque<PDD_NODE>				* recvQueue = (deque<PDD_NODE>*)p;
		TNSTable					* TNST = new TNSTable();
		InputDummy(TNST);

		while (true) {
			Sleep(SLEEP_TIME_SHORT);
			//receive Data Process

			//ť�� �޼��� �����ϸ� �޼��� Ÿ�Կ� ���� �з�
			if (recvQueue->size() > 0) {
				SeperateMessagePerType(recvQueue, TNST, sendDatagram);
			}

			if (TNS_TYPE == 1 && TNST->isEntryExist(STATE_SET)) {
				PTNSP = TNST->getEntry(STATE_SET);
				PrintPTNSP(PTNSP);

				memset(&tempAddr, 0, sizeof(tempAddr));

				SetDatagramFromPTNSP(&sendDatagram, PTNSP, &tempAddr);

				clientSocket = socket(PF_INET, SOCK_STREAM, 0);

				if (clientSocket == INVALID_SOCKET) {
					ErrorHandling(MESSAGE_ERROR_CLIENT);
				}

				if (connect(clientSocket, (SOCKADDR*)&tempAddr, sizeof(tempAddr)) == SOCKET_ERROR) {
					ErrorHandling(MESSAGE_ERROR_CONNECT);
				} else {
					send(clientSocket, (char*)&sendDatagram, sizeof(PDD_NODE), 0);
					PTNSP->key.TN_SPACE_STATE = STATE_RESPONSE;
					closesocket(clientSocket);
				}
			}
		}
	}

	void SeperateMessagePerType(deque<PDD_NODE> * recvQueue, TNSTable * TNST, PDD_NODE receiveDatagram) {
		PDD_NODE sendDatagram;
		deque<PDD_NODE>::iterator	it;

		int counter = 0;
		for (it = recvQueue->begin(); it != recvQueue->end(); it++) {
			sendDatagram = (*it);
			if (TNS_TYPE == 1) {
				if (sendDatagram.PDD_HEADER.MESSAGE_TYPE == MESSAGE_TYPE_RESPONSE) {
					TNST->setEntry(receiveDatagram.PDD_HEADER.ID, sendDatagram.PDD_DATA[0].PARTICIPANT_DATA, STATE_SET);
				}
				else if (sendDatagram.PDD_HEADER.MESSAGE_TYPE == MESSAGE_TYPE_SAVEDONE) {
					TNST->removeEntry(sendDatagram.PDD_HEADER.ID);
					puts("SAVE Complete");
				}
				else {
					TNST->removeEntry(sendDatagram.PDD_HEADER.ID);
					puts("ERROR MSG");
				}
			}

			if (sendDatagram.PDD_HEADER.MESSAGE_TYPE == MESSAGE_TYPE_SAVE) {
				// ���
				PrintMessage(sendDatagram);
			}
			counter++;
		}

		EnterCriticalSection(&cs);
		for (int k = 0; k < counter; k++) {
			recvQueue->pop_front();
		}
		LeaveCriticalSection(&cs);
	}
	
	void SetDatagramFromPTNSP(PDD_NODE * datagram, PTNSP_NODE PTNSP, SOCKADDR_IN * tempAddr) {
		tempAddr->sin_family = AF_INET;
		tempAddr->sin_addr.S_un.S_addr = inet_addr(PTNSP->key.TN_SPACE_NEXTZONE_ADDR);

		if (PTNSP->key.TN_SPACE_TOTAL_LEVEL < PTNSP->key.TN_SPACE_CURRENT_LEVEL) {
			tempAddr->sin_port = htons(TERMINAL_PORT);
			datagram->PDD_HEADER.MESSAGE_TYPE = PTNSP->key.TN_SPACE_MESSAGETYPE;
			memcpy(datagram->PDD_DATA[FIRST_INDEX].PARTICIPANT_DATA, PTNSP->key.TN_SPACE_DATA, MAX_DATA_SIZE);
		}
		else {
			tempAddr->sin_port = htons(TNS_PORT);
			datagram->PDD_HEADER.MESSAGE_TYPE = MESSAGE_TYPE_REQUEST;
		}

		datagram->PDD_HEADER.ID = PTNSP->key.TN_SPACE_ID;

		datagram->PDD_DATA[FIRST_INDEX].PARTICIPANT_NODE_TYPE = PTNSP->key.TN_SPACE_NODETYPE;

		memcpy(datagram->PDD_DATA[FIRST_INDEX].PARTICIPANT_DOMAIN_ID, PTNSP->key.TN_SPACE_DOMAIN, sizeof(PTNSP->key.TN_SPACE_DOMAIN));
		memcpy(datagram->PDD_DATA[FIRST_INDEX].PARTICIPANT_TOPIC, PTNSP->key.TN_SPACE_TOPIC, sizeof(PTNSP->key.TN_SPACE_TOPIC));
		memcpy(datagram->PDD_DATA[FIRST_INDEX].PARTICIPANT_IP, PTNSP->key.TN_SPACE_PARTICIPANT_ADDR, ADDRESS_SIZE);
		itoa(PTNSP->key.TN_SPACE_CURRENT_LEVEL, datagram->PDD_DATA[FIRST_INDEX].PARTICIPANT_DATA, 10);
		datagram->PDD_DATA[FIRST_INDEX].PARTICIPANT_PORT = PTNSP->key.TN_SPACE_PARTICIPANT_PORT;
		datagram->PDD_HEADER.PARTICIPANT_NUMBER_OF_DATA = 1;

		cout << "SEND TYPE :" << datagram->PDD_HEADER.MESSAGE_TYPE << endl;
		cout << "====================" << endl;
	}

	void InputDummy(TNSTable * TNSPTable) {
		//this->TNSPTable->addEntry("Q/WW/EEE/RRRR/TTTTTT", "DDS_3",
		//	"127.0.0.1", "127.0.0.10", 1000, "DDS_TEST_DATA3", NULL, NODE_TYPE_PUB, MESSAGE_TYPE_SAVE);
		TNSPTable->addEntry("A/BB/CCC/DDDD/EEEEEE", "DDS_1", IP, "127.0.0.20", 70, "CCCCCCC", NULL, NODE_TYPE_PUB, MESSAGE_TYPE_SAVE);
	}

	void InitialFrontEndServer() {
		int	test_type;
		char topic[MAX_CHAR];
		char domain[MAX_CHAR];
		char nextzone[ADDRESS_SIZE];
		char participantip[ADDRESS_SIZE];
		int participantport;
		char data[MAX_DATA_SIZE];
		int pubsub;
		int messageType;

		printf("[ INITIALIZE Front-End SERVER ]\n\n");

		printf("***** Select inital type *****\n");
		printf("[1] Create Transmission Topic\n");
		printf("[2] Input  Dummy        Topic\n");
		printf("[3] Run	   Front-End	Server\n");
		printf("[others] Exit\n");
		printf("******************************\n");

		printf("input>");
		scanf("%d", &test_type);

		if (test_type == 2) {
			//inputDummy();
		}
		else if (test_type == 3)
			return;

		while (test_type == 1) {

			printf("input Topic>");
			scanf("%s", topic);

			printf("input Domain>");
			scanf("%s", domain);

			printf("input Next Zone IP>");
			scanf("%s", nextzone);

			printf("input Participant IP>");
			scanf("%s", participantip);

			printf("input Participant Port>");
			scanf("%d", &participantport);

			printf("input Sample Data>");
			scanf("%s", data);

			printf("input Pub/Sub Type (100 : PUB / 200 : SUB)>");
			scanf("%d", &pubsub);

			printf("input Message Type (10 : SAVE / 11 : REMOVE / 12 : MODIFY)>");
			scanf("%d", &messageType);

			//TNSPTable->addEntry(topic, domain, nextzone, participantip, participantport, data, NULL, pubsub, messageType);

			printf("***** Select inital type *****\n");
			printf("[1] Create Transmission Topic\n");
			printf("[2] Input  Dummy        Topic\n");
			printf("[3] Run	   Front-End	Server\n");
			printf("[others] Exit\n");
			printf("******************************\n");

			printf("input>");
			scanf("%d", &test_type);
		}

		printf("[ Complete Inital Front-End SERVER ]\n\n");
	}
	
	void PrintMessage(PDD_NODE datagram) {
		cout << "======================================================================" << endl;
		cout << "NUM OF DATA		:	" << datagram.PDD_HEADER.PARTICIPANT_NUMBER_OF_DATA << endl;
		for (int k = FIRST_INDEX; k < datagram.PDD_HEADER.PARTICIPANT_NUMBER_OF_DATA; k++) {
			cout << "**********************************************************************" << endl;
			cout << "NODE TYPE			:	" << datagram.PDD_DATA[k].PARTICIPANT_NODE_TYPE << endl;
			cout << "Domain ID			:	" << datagram.PDD_DATA[k].PARTICIPANT_DOMAIN_ID << endl;
			cout << "TOPIC				:	" << datagram.PDD_DATA[k].PARTICIPANT_TOPIC << endl;
			cout << "PARTICIPANT IP			:	" << datagram.PDD_DATA[k].PARTICIPANT_IP << endl;
			cout << "PARTICIPANT PORT		:	" << datagram.PDD_DATA[k].PARTICIPANT_PORT << endl;
			cout << "DATA				:	" << datagram.PDD_DATA[k].PARTICIPANT_DATA << endl;
		}
		cout << "======================================================================" << endl;
	}

	void PrintPTNSP(PTNSP_NODE PTNSP) {
		cout << "====================" << endl;
		cout << "SEND MSG" << endl;
		cout << "SEND Topic :" << PTNSP->key.TN_SPACE_TOPIC << endl;
		cout << "SEND Token :" << PTNSP->key.TN_SPACE_TOKEN << endl;
		cout << "SEND TokenLevel :" << PTNSP->key.TN_SPACE_CURRENT_LEVEL << " / " << PTNSP->key.TN_SPACE_TOTAL_LEVEL << endl;
	}
}
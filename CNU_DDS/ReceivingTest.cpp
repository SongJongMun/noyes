#include <stdio.h>
#include <iostream>
#include <WinSock2.h>
#include <vector>

#define MAX_CHAR		100
#define MAX_DATA_SIZE	1000

using namespace std;

namespace CNU_DDS
{
	typedef struct _PDD_HEADER {
		char *				PARTICIPANT_DOMAIN_ID;
		int					PARTICIPANT_DOMAIN_SIZE;
		int					PARTICIPANT_NODE_TYPE;
		int					PARTICIPANT_NUMBER_OF_DATA;
	} PDD_HEADER, *PPDD_HEADER;

	typedef struct _PDD_DATA {
		char				PARTICIPANT_TOPIC[MAX_CHAR];
		char 				PARTICIPANT_DATA[MAX_DATA_SIZE];
	} PDD_DATA, *PPDD_DATA;

	typedef struct _PDD_NODE {
		PDD_HEADER			PDD_HEADER;
		PDD_DATA *			PDD_DATA;
	} PDD_NODE, *PPDD_NODE;

	void ErrorHandling(char *message)
	{
		fputs(message, stderr);
		fputc('\n', stderr);
		exit(1);
	}

	SOCKET CreateSocket() {
		// ���� ���� (������ �ڵ���, ���н� "INVALID_SOCKET" ��ȯ)
		SOCKET servSock = socket(PF_INET, SOCK_STREAM, 0);

		// ���� ���� ���� ó��
		if (servSock == INVALID_SOCKET) {
			ErrorHandling("socket() error");
		}

		return servSock;
	}

	void BindingSocket(SOCKET servSocket) {
		// ���� ����� ���� �⺻ ����
		SOCKADDR_IN servAddr;

		servAddr.sin_family = AF_INET;
		servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servAddr.sin_port = htons(4000);

		// �ּҿ� Port �Ҵ� (���ε�!!)
		if (bind(servSocket, (struct sockaddr *) &servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
			ErrorHandling("bind() error");
		}

	}
	
	void LinkingEvents(SOCKET servSock, int* sockNum, vector<SOCKET> * sockArray, vector<WSAEVENT> * eventArray) {
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

	void	StartReceving(){
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

		PPDD_NODE receiveData;

		int clntLen;
		int sockTotal = 0;
		int index, i;
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

		printf("START RECEIVING TEST\n");

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
						strLen = recv(sockArray[index - WSA_WAIT_EVENT_0], (char*)receiveData, sizeof(_PDD_NODE), 0);

						if (getpeername(sockArray[index - WSA_WAIT_EVENT_0], (struct sockaddr *)&name, &len) != 0) {
							perror("getpeername Error");
						}

						//cout << inet_ntoa(name.sin_addr) << endl;
						//cout << sizeof(inet_ntoa(name.sin_addr)) << endl;					

						if (strLen != -1) {
							// ���
							cout << "======================================================================" << endl;
							cout << "Domain ID		:	"<< receiveData->PDD_HEADER.PARTICIPANT_DOMAIN_ID << endl;
							cout << "NODE TYPE		:	"<< receiveData->PDD_HEADER.PARTICIPANT_NODE_TYPE << endl;
							cout << "NUM OF DATA	:	"<< receiveData->PDD_HEADER.PARTICIPANT_NUMBER_OF_DATA << endl;
							for (int k = 0 ; k < receiveData->PDD_HEADER.PARTICIPANT_NUMBER_OF_DATA ; k++){
								cout << "**********************************************************************" << endl;
								cout << "TOPIC			:	"<< receiveData->PDD_HEADER.PARTICIPANT_DOMAIN_ID << endl;
								cout << "DATA			:	"<< receiveData->PDD_HEADER.PARTICIPANT_DOMAIN_ID << endl;
							}
							cout << "======================================================================" << endl;
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
	}
}
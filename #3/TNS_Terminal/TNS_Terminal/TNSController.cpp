#include "TNSController.h"

TNSController::TNSController()
{
	initalizeSetting();
	startTNSServer();
}


TNSController::~TNSController()
{
	closeTNSServer();
	DeleteCriticalSection(&cs);
}

//controller
void 							TNSController::initalizeSetting() {
	this->socketManager = new SocketManager();
	this->databaseManager = new DBManager();
	this->messageHandler = new MessageHandler();

	this->databaseManager->initDBInfo();

	InitializeCriticalSection(&cs);
	this->socketManager->setCriticalSection(&cs);
}

void							TNSController::startTNSServer() {
	this->socketManager->getRecevingDEQUE(&(this->recvData));
	this->socketManager->startRecevingThread();
}

void							TNSController::closeTNSServer() {
	this->socketManager->closeRecevingThread();
}

void							TNSController::distibuteTNSData() {
	list<PDD_DATA>				distributeList;
	list<PDD_DATA>::iterator	it;
	int							NumOfParticipant;

	PPDD_NODE PDatagram = (PPDD_NODE)malloc(sizeof(_PDD_NODE));			//������ �ִ� ���鿡�� ���ο� �������� ������ �����ϱ� ���� ���� �����ͱ׷�, Front-End���� ���޹��� ��Ŷ �״�� �ᵵ �ɵ�
	PPDD_NODE ReturnDatagram = (PPDD_NODE)malloc(sizeof(_PDD_NODE));	//���� �߰��� ��忡�� ���� �����ڵ��� ������ �����ϱ� ���� ���� �����ͱ׷�, �������� ��Ʈ���� ����

	while (true) {
		messageHandler->setPDDNode(PDatagram);
		messageHandler->setPDDNode(ReturnDatagram);

		Sleep(10);

		while (isReceviedDataExist()) {
			PDD_NODE entry = recvData.back().second;

			if (messageHandler->isPacketAvailable(&entry)) {
				if (!this->databaseManager->isTopicExist(entry.PDD_DATA[0].PARTICIPANT_TOPIC)) {
					printf("This Topic isn't Exist\n");
					messageHandler->setMessageTypeTopicNotExist(&entry);
				}
				else {// DB�� ���� �� copy protocol
					messageHandler->setMessageType(ReturnDatagram, entry.PDD_HEADER.MESSAGE_TYPE);
					messageHandler->copyDatagram(&entry, PDatagram);
					
					distributeList = databaseManager->InsertEntry(entry.PDD_DATA[0]);

					messageHandler->setMessageTypeProcessDone(&entry);
				}
			}
			else {
				messageHandler->setMessageTypeTopicNotExist(&entry);
				puts("ERROR MSG TYPE");
			}

			socketManager->sendPacket(inet_ntoa(recvData.back().first), (char *)&entry, sizeof(_PDD_NODE), FES_PORT);

			NumOfParticipant = 0;
			for (it = distributeList.begin(); it != distributeList.end(); ++it) {
				//printf("To : %s || Data : %s / %s / %s / %s / %d / %s\n", (*it).PARTICIPANT_IP, (*it).PARTICIPANT_NODE_TYPE == NODE_TYPE_PUB ? "PUB" : "SUB",
				//	(*it).PARTICIPANT_TOPIC, (*it).PARTICIPANT_DOMAIN_ID, (*it).PARTICIPANT_IP, (*it).PARTICIPANT_PORT, (*it).PARTICIPANT_DATA);

				messageHandler->addDataToNode(ReturnDatagram, *it);

				//ReturnDatagram->PDD_DATA[NumOfParticipant++] = (*it);

				// Send New Participant Data to Existing Participant
				socketManager->sendPacket((*it).PARTICIPANT_IP, (const char *)PDatagram, sizeof(_PDD_NODE), DDS_PORT);
			}

			//Send Existing Particiant Data To New Participant
			messageHandler->setParticipantNumber(ReturnDatagram, NumOfParticipant);
			//ReturnDatagram->PDD_HEADER.NUMBER_OF_PARTICIPANT = NumOfParticipant;

			socketManager->sendPacket(entry.PDD_DATA[0].PARTICIPANT_IP, (const char *)ReturnDatagram, sizeof(_PDD_NODE), DDS_PORT);

			//Remove Receive Data
			EnterCriticalSection(&cs);
			recvData.pop_back();
			LeaveCriticalSection(&cs);
		}
	}
}
/*
void							TNSController::distibuteTNSData() {
	list<PDD_DATA>				distributeList;
	list<PDD_DATA>::iterator	it;
	int							NumOfParticipant;

	PPDD_NODE PDatagram			= (PPDD_NODE)malloc(sizeof(_PDD_NODE));		//������ �ִ� ���鿡�� ���ο� �������� ������ �����ϱ� ���� ���� �����ͱ׷�, ���� �Ѱ� ��Ʈ���� ����, Front-End���� ���޹��� ��Ŷ �״�� �ᵵ �ɵ�
	PPDD_NODE ReturnDatagram	= (PPDD_NODE)malloc(sizeof(_PDD_NODE));		//���� �߰��� ��忡�� ���� �����ڵ��� ������ �����ϱ� ���� ���� �����ͱ׷�, �������� ��Ʈ���� ����

	while (true) {
		memset(PDatagram, 0, sizeof(PDD_NODE));
		memset(ReturnDatagram, 0, sizeof(PDD_NODE));

		Sleep(10);

		while (isReceviedDataExist()) {
			PDD_NODE entry = recvData.back().second;
			
			if (entry.PDD_HEADER.MESSAGE_TYPE == MESSAGE_TYPE_SAVE || entry.PDD_HEADER.MESSAGE_TYPE == MESSAGE_TYPE_MODIFY || entry.PDD_HEADER.MESSAGE_TYPE == MESSAGE_TYPE_REMOVE) {
				if (!this->DB->isTopicExist(entry.PDD_DATA[0].PARTICIPANT_TOPIC)) {
					printf("This Topic isn't Exist\n");
					entry.PDD_HEADER.MESSAGE_TYPE = MESSAGE_TYPE_NOTEXIST;
				}
				else {
					ReturnDatagram->PDD_HEADER.MESSAGE_TYPE = entry.PDD_HEADER.MESSAGE_TYPE;
					memcpy(PDatagram, &entry, sizeof(PDD_NODE));
					PDatagram->PDD_HEADER.NUMBER_OF_PARTICIPANT = 1;

					distributeList = DB->InsertEntry(entry.PDD_DATA[0]);
					// ���ο� �������� ������ �ݴ�Ǵ� ������ �����ڵ� ������ DB���� ������

					entry.PDD_HEADER.MESSAGE_TYPE += MESSAGE_OPTION_PLUS_DONE;
				}
			} else {
				puts("ERROR MSG TYPE");
				entry.PDD_HEADER.MESSAGE_TYPE = MESSAGE_TYPE_NOTEXIST;
			}

			// Return Result Packet To Front-End Server
			socket->sendPacket(inet_ntoa(recvData.back().first), (char *)&entry, sizeof(_PDD_NODE), FES_PORT);

			// NumOfParticipat
			NumOfParticipant = 0;

			for (it = distributeList.begin(); it != distributeList.end(); ++it) {
				printf("To : %s || Data : %s / %s / %s / %s / %d / %s\n", (*it).PARTICIPANT_IP, (*it).PARTICIPANT_NODE_TYPE == NODE_TYPE_PUB ? "PUB" : "SUB",
					(*it).PARTICIPANT_TOPIC, (*it).PARTICIPANT_DOMAIN_ID, (*it).PARTICIPANT_IP, (*it).PARTICIPANT_PORT, (*it).PARTICIPANT_DATA);

				ReturnDatagram->PDD_DATA[NumOfParticipant++] = (*it);

				// Send New Participant Data to Existing Participant
				socket->sendPacket((*it).PARTICIPANT_IP, (const char *)PDatagram, sizeof(_PDD_NODE), DDS_PORT);
			}

			//Send Existing Particiant Data To New Participant
			ReturnDatagram->PDD_HEADER.NUMBER_OF_PARTICIPANT = NumOfParticipant;

			socket->sendPacket(entry.PDD_DATA[0].PARTICIPANT_IP, (const char *)ReturnDatagram, sizeof(_PDD_NODE), DDS_PORT);

			//Remove Receive Data
			EnterCriticalSection(&cs);
			recvData.pop_back();
			LeaveCriticalSection(&cs);

		}
	}
}
*/

bool							TNSController::isReceviedDataExist() {
	return this->recvData.size() == 0 ? false : true;
}

#pragma once

#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>

#define MAX_CHAR		100
#define MAX_DATA_SIZE	800
#define MAX_PDD_NUMBER  1
#define NULL			0

#define	ADDRESS_SIZE	16

#define STATE_NEW		0
#define STATE_SET		1
#define STATE_RESPONSE	2
#define	STATE_END		3

#define MESSAGE_TYPE_REQUEST	1
#define MESSAGE_TYPE_RESPONSE	2
#define MESSAGE_TYPE_NOTEXIST	3

#define MESSAGE_TYPE_SAVE		10
#define MESSAGE_TYPE_REMOVE		11
#define MESSAGE_TYPE_MODIFY		12

#define MESSAGE_TYPE_SAVEDONE	20
#define MESSAGE_TYPE_REMOVEDONE	21
#define MESSAGE_TYPE_MODIFYDONE	22

#define SERVER_TYPE_FRONT_END	1
#define SERVER_TYPE_DDS_EXTEND	2

#define NODE_TYPE_PUB			100
#define NODE_TYPE_SUB			200
#define BUFSIZE					100		// �޾ƿ� ������ �ִ� ũ��
#define PORT					3000		// ��Ʈ��ȣ �Ҵ�
#define TNS_PORT				3001
#define TERMINAL_PORT			3002

#define IP						"127.0.0.1"

#define MESSAGE_ERROR_SOCKET			"socket() error"
#define MESSAGE_ERROR_BIND				"bind() error"
#define MESSAGE_ERROR_LISTEN			"listen() error"
#define MESSAGE_ERROR_ACCEPT			"Accept Error"
#define MESSAGE_ERROR_READ				"Read Error"
#define MESSAGE_ERROR_CLOSE				"Close Error"
#define MESSAGE_ERROR_CLIENT			"clientSocket() error"
#define MESSAGE_ERROR_CONNECT			"connect() error!"
#define MESSAGE_ERROR_WSA_SELECT		"WSAEventSelect() error"
#define MESSAGE_ERROR_WSA_STARTUP		"WSAStartup() error!"

// TCPSocket
typedef struct _PDD_HEADER {
	int					ID;
	int					MESSAGE_TYPE;
} PDD_HEADER, *PPDD_HEADER;

typedef struct _PDD_DATA {
	int					PARTICIPANT_NODE_TYPE;
	char				PARTICIPANT_TOPIC[MAX_CHAR];
	char				PARTICIPANT_TOPICTYPE[MAX_CHAR];
	char				PARTICIPANT_DOMAIN_ID[MAX_CHAR];
	char				PARTICIPANT_IP[ADDRESS_SIZE];
	int					PARTICIPANT_PORT;
	unsigned char		PARTICIPANT_DATA_ORDERING_TYPE;
	unsigned char 		PARTICIPANT_DATA[MAX_DATA_SIZE];
	int					PARTICIPANT_LENGTH_OF_DATA;
} PDD_DATA, *PPDD_DATA;

typedef struct _PDD_NODE {
	PDD_HEADER			PDD_HEADER;
	PDD_DATA 			PDD_DATA;
} PDD_NODE, *PPDD_NODE;

//TopicNameSpaceTable
typedef struct _TNSP_ENTRY {
	int					TN_SPACE_ID;
	int					TN_SPACE_MESSAGETYPE;

	char				TN_SPACE_DOMAIN[MAX_CHAR];
	char				TN_SPACE_TOPIC[MAX_CHAR];
	char				TN_SPACE_TOPICTYPE[MAX_CHAR];
	char				TN_SPACE_TOKEN[MAX_CHAR];
	int					TN_SPACE_NODETYPE;
	char				TN_SPACE_PARTICIPANT_ADDR[ADDRESS_SIZE];
	int					TN_SPACE_PARTICIPANT_PORT;
	unsigned char		TN_SPACE_DATA[MAX_DATA_SIZE];
	int					TN_SPACE_LENGTH_OF_DATA;
	unsigned char		TN_SPCAE_DATA_ORDERING_TYPE;

	int					TN_SPACE_TOTAL_LEVEL;
	int					TN_SPACE_CURRENT_LEVEL;
	int					TN_SPACE_STATE;

	char				TN_SPACE_NEXTZONE_ADDR[ADDRESS_SIZE];
} TNSP_ENTRY, *PTNSP_ENTRY;
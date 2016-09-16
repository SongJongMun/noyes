#pragma once

#include <stdlib.h>
#include <vector>
#include "stdafx.h"

using namespace std;


class RequestTable
{
public:
	RequestTable();
	~RequestTable();

	int							numOfRequests;
	PR_NODE				r_head;
	vector<_R_ENTRY>	requestList;

	void							resetTable();
	bool							isRequestExist();														//Request ���翩�� Ȯ��
	PR_NODE				getLastEntry();														//������ Entry ����
	void							addEntry(IN_ADDR ip, PDD_NODE PDD_NODE);	//����Ʈ�� Entry �߰�

private:
	CRITICAL_SECTION cs;																			//Semaphore ����
};


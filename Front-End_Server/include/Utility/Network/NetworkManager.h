#ifndef INCLUDE_UTILITY_NETWORK_NETWORKMANAGER_H_
#define INCLUDE_UTILITY_NETWORK_NETWORKMANAGER_H_

#ifdef __CYGWIN__
#undef WIN
#undef _WIN
#undef _WIN32
#undef _WIN64
#undef __WIN__
#endif

#ifdef	WIN32
	#pragma comment(lib, "ws2_32.lib")
	#include <winsock2.h>
#else
	#include <sys/ioctl.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <sys/socket.h>
	#include <unistd.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <net/if.h>
	#include <arpa/inet.h>
#endif

#include "../DoublyLinkedList.h"

#ifdef CNU_DDS_DLL
#define CNU_DDS_DLL_API	__declspec(dllexport)	
#else
#define CNU_DDS_DLL_API	__declspec(dllimport)
#endif
#ifdef __cplusplus
extern "C" {
#endif
namespace CNU_DDS
{
	#define MAX_LOCAL_NUM 10

	class CNU_DDS_DLL_API IPAddress
	{
	public:
		char ipAddr[16];
	public:
		IPAddress(char addr[16])
		{
			memcpy(ipAddr, addr, 15);
			ipAddr[15]	= 0;
		}
	};

	class CNU_DDS_DLL_API NetworkInfoManager
	{
	private:
		DoublyLinkedList<IPAddress>* IPAddresses;

	private:
		NetworkInfoManager();
		~NetworkInfoManager();
	public:
		static	NetworkInfoManager*		getNetworkInfoManagerInstance();

		void							resetIPAddresses();

		DoublyLinkedList<IPAddress>*	getLocalIPAddressList();
	};
}

#ifdef __cplusplus
}
#endif
#endif /* INCLUDE_UTILITY_NETWORK_NETWORKMANAGER_H_ */
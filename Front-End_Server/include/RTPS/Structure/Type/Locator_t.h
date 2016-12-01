#ifndef LOCATOR_T_H_
#define LOCATOR_T_H_

#include "../../../Utility/DoublyLinkedList.h"

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
	class CNU_DDS_DLL_API Locator_t
	{
	public:
		long			kind;
		unsigned long	port;
		char			address[16];

	public:
		void			setAddress(char* a_addr, unsigned long a_port);

		bool			operator	== (const Locator_t& arg);
		void			operator	= (const Locator_t& arg);
	};

	typedef DoublyLinkedList<Locator_t>	LocatorSeq;

	const long			LOCATOR_KIND_INVALID		= -1;
	const long			LOCATOR_KIND_RESERVED		= 0;
	const long			LOCATOR_KIND_UDPv4			= 1;
	const long			LOCATOR_KIND_UDPv6			= 2;
	const unsigned long	LOCATOR_PORT_INVALID		= 0;
	const char			LOCATOR_ADDRESS_INVALID[16]	= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	const Locator_t		LOCATOR_INVALID				= {LOCATOR_KIND_INVALID, LOCATOR_PORT_INVALID, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
}

#ifdef __cplusplus
}
#endif
#endif
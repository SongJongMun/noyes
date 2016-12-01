#ifndef SAMPLELOSTSTATUS_H_
#define SAMPLELOSTSTATUS_H_

#include "Status.h"

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
	class CNU_DDS_DLL_API SampleLostStatus:public Status
	{
	public:
		long	total_count;
		long	total_count_change;
	};
}

#ifdef __cplusplus
}
#endif
#endif
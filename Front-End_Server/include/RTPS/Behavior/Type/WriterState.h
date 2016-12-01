#ifndef INCLUDE_RTPS_BEHAVIOR_TYPE_WRITERSTATE_H_
#define INCLUDE_RTPS_BEHAVIOR_TYPE_WRITERSTATE_H_

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
	enum WriterState
	{
		WRITER_STATE_IDLE,			// 0
		WRITER_STATE_PUSHING,		// 1
		WRITER_STATE_ANNOUNCING,	// 2
		WRITER_STATE_WAITING,		// 3
		WRITER_STATE_REPAIRING,		// 4
		WRITER_STATE_MUST_REPAIR,	// 5
		WRITER_STATE_FINAL			// 6
	};

	enum WriterEvent
	{
		UNSENT_CHANGES_TRUE,		// 0
		UNSENT_CHANGES_FALSE,		// 1
		UNACKED_CHANGES_TRUE,		// 2
		UNACKED_CHANGES_FALSE,		// 3
		REQUESTED_CHANGES_TRUE,		// 4
		REQUESTED_CHANGES_FALSE,	// 5
		AFTER_HEARTBEAT_PERIOD,		// 6
		AFTER_NACK_RESPONSE_DELAY,	// 7
		CAN_PUSHING,				// 8
		CAN_REPAIRING,				// 9
		ACKNACK_IS_RECEIVED,		// 10
		DELETE_READER_LOCATOR,		// 11
		DELETE_READER_PROXY,		// 12
		SEND_A_MESSAGE				// 13
	};
}

#ifdef __cplusplus
}
#endif
#endif /* INCLUDE_RTPS_BEHAVIOR_TYPE_WRITERSTATE_H_ */
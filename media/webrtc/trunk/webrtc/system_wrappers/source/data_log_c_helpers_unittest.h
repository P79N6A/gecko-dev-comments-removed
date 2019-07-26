









#ifndef SRC_SYSTEM_WRAPPERS_SOURCE_DATA_LOG_C_HELPERS_UNITTEST_H_
#define SRC_SYSTEM_WRAPPERS_SOURCE_DATA_LOG_C_HELPERS_UNITTEST_H_

#ifdef __cplusplus
extern "C" {
#endif

int WebRtcDataLogCHelper_TestCreateLog();

int WebRtcDataLogCHelper_TestReturnLog();

int WebRtcDataLogCHelper_TestCombine();

int WebRtcDataLogCHelper_TestAddTable();

int WebRtcDataLogCHelper_TestAddColumn();

int WebRtcDataLogCHelper_TestNextRow();

int WebRtcDataLogCHelper_TestInsertCell_int();

int WebRtcDataLogCHelper_TestInsertArray_int();

int WebRtcDataLogCHelper_TestInsertCell_float();

int WebRtcDataLogCHelper_TestInsertArray_float();

int WebRtcDataLogCHelper_TestInsertCell_double();

int WebRtcDataLogCHelper_TestInsertArray_double();

int WebRtcDataLogCHelper_TestInsertCell_int32();

int WebRtcDataLogCHelper_TestInsertArray_int32();

int WebRtcDataLogCHelper_TestInsertCell_uint32();

int WebRtcDataLogCHelper_TestInsertArray_uint32();

int WebRtcDataLogCHelper_TestInsertCell_int64();

int WebRtcDataLogCHelper_TestInsertArray_int64();

#ifdef __cplusplus
}  
#endif

#endif

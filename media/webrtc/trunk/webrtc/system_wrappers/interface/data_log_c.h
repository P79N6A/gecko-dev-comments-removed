













#ifndef SRC_SYSTEM_WRAPPERS_INTERFACE_DATA_LOG_C_H_
#define SRC_SYSTEM_WRAPPERS_INTERFACE_DATA_LOG_C_H_

#include <stddef.h>  

#include "webrtc/typedefs.h"  

#ifdef __cplusplus
extern "C" {
#endif



int WebRtcDataLog_CreateLog();
void WebRtcDataLog_ReturnLog();
char* WebRtcDataLog_Combine(char* combined_name, size_t combined_len,
                            const char* table_name, int table_id);
int WebRtcDataLog_AddTable(const char* table_name);
int WebRtcDataLog_AddColumn(const char* table_name, const char* column_name,
                            int multi_value_length);

int WebRtcDataLog_InsertCell_int(const char* table_name,
                                 const char* column_name,
                                 int value);
int WebRtcDataLog_InsertArray_int(const char* table_name,
                                  const char* column_name,
                                  const int* values,
                                  int length);
int WebRtcDataLog_InsertCell_float(const char* table_name,
                                   const char* column_name,
                                   float value);
int WebRtcDataLog_InsertArray_float(const char* table_name,
                                    const char* column_name,
                                    const float* values,
                                    int length);
int WebRtcDataLog_InsertCell_double(const char* table_name,
                                    const char* column_name,
                                    double value);
int WebRtcDataLog_InsertArray_double(const char* table_name,
                                     const char* column_name,
                                     const double* values,
                                     int length);
int WebRtcDataLog_InsertCell_int32(const char* table_name,
                                   const char* column_name,
                                   int32_t value);
int WebRtcDataLog_InsertArray_int32(const char* table_name,
                                    const char* column_name,
                                    const int32_t* values,
                                    int length);
int WebRtcDataLog_InsertCell_uint32(const char* table_name,
                                    const char* column_name,
                                    uint32_t value);
int WebRtcDataLog_InsertArray_uint32(const char* table_name,
                                     const char* column_name,
                                     const uint32_t* values,
                                     int length);
int WebRtcDataLog_InsertCell_int64(const char* table_name,
                                   const char* column_name,
                                   int64_t value);
int WebRtcDataLog_InsertArray_int64(const char* table_name,
                                    const char* column_name,
                                    const int64_t* values,
                                    int length);

int WebRtcDataLog_NextRow(const char* table_name);

#ifdef __cplusplus
}  
#endif

#endif

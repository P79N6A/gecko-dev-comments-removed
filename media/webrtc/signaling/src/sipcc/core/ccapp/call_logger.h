



#ifndef __CALL_LOGGER_H__
#define __CALL_LOGGER_H__

#include "cc_types.h"
#include "CCProvider.h" 

void calllogger_init_call_log(cc_call_log_t *log);
void calllogger_copy_call_log(cc_call_log_t *dest, cc_call_log_t * src);
void calllogger_free_call_log(cc_call_log_t *log);
void calllogger_print_call_log(cc_call_log_t *log);
void calllogger_setPlacedCallInfo(session_data_t *data);
void calllogger_updateLogDisp(session_data_t *data);
void calllogger_setMissedCallLoggingConfig(cc_string_t mask);
void  calllogger_update(session_data_t *data);

#endif

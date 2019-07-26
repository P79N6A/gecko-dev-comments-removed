






































#ifndef XML_UTIL_H_
#define XML_UTIL_H_
#include "cc_constants.h"
#include "xml_parser_defines.h"

int xmlInit();
void xmlDeInit();
char * xmlEncodeEventData(ccsip_event_data_t *event_datap);
int xmlDecodeEventData (cc_subscriptions_ext_t msg_type, const char *msg_body, int msg_length, ccsip_event_data_t ** event_datap);

#endif 


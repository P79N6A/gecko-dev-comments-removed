






































#ifndef __XML_PARSER_H__
#define __XML_PARSER_H__

#include "cc_constants.h"
#include "xml_parser_defines.h"






int ccxmlInitialize(void ** xml_parser_handle);






void ccxmlDeInitialize(void ** xml_parser_handle);








char * ccxmlEncodeEventData (void * xml_parser_handle, ccsip_event_data_t *event_datap);













int ccxmlDecodeEventData (void * xml_parser_handle, cc_subscriptions_ext_t msg_type, const char *msg_body, int msg_length, ccsip_event_data_t ** event_datap);
#endif

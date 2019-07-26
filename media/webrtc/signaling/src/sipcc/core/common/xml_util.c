






































#include "xml_parser.h"
#include "cpr_stdlib.h"
#include "phone_debug.h"
#include "xml_util.h"

static void *xml_parser_handle = NULL;

int xmlInit() {
    if (ccxmlInitialize(&xml_parser_handle) == CC_FAILURE) {
         return CC_FAILURE;
    }
    return CC_SUCCESS;
}

void xmlDeInit() {
    ccxmlDeInitialize(&xml_parser_handle);
}

char * xmlEncodeEventData(ccsip_event_data_t *event_datap) {
    const char     *fname = "xmlEncodeEventData";
    char           *buffer;
    uint32_t        nbytes = 0;

    CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"Encode event data: entered,\n",
                                    DEB_F_PREFIX_ARGS(SIP_SUB, fname));
    
    if (event_datap->type == EVENT_DATA_RAW) {
        nbytes = event_datap->u.raw_data.length;
        buffer = (char *) ccAllocXML(nbytes + 1);
        if (buffer) {
            memcpy(buffer, event_datap->u.raw_data.data, nbytes);
            CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"Framed raw buffer: length = %d,\n",
                                DEB_F_PREFIX_ARGS(SIP_SUB, fname), nbytes);
        }
        return (buffer);

    }
    buffer = ccxmlEncodeEventData(xml_parser_handle, event_datap);
    DEF_DEBUG(DEB_F_PREFIX"returned content after encoding:\n%s\n", DEB_F_PREFIX_ARGS(SIP_REG, fname), buffer);
    return (buffer);

    
}

int xmlDecodeEventData (cc_subscriptions_ext_t msg_type, const char *msg_body, int msg_length, ccsip_event_data_t ** event_datap) {
    const char     *fname = "xmlDecodeEventData";
    CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"Decode event data: entered,\n",
                                    DEB_F_PREFIX_ARGS(SIP_SUB, fname));

    return ccxmlDecodeEventData(xml_parser_handle, msg_type, msg_body, msg_length, event_datap );
}

void *ccAllocXML(cc_size_t size) {
    return cpr_calloc(1, size);
}

void ccFreeXML(void *mem) {
    cpr_free(mem);
}


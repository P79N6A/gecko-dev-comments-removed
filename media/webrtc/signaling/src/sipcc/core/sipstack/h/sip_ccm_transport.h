






































#ifndef __SIP_CCM_TRANSPORT_H__
#define __SIP_CCM_TRANSPORT_H__

#include "cpr_types.h"
#include "cpr_socket.h"
#include "phone_types.h"

#define CCM_ID_PRINT(arg) \
        (arg == PRIMARY_CCM ?  "PRIMARY_CCM" : \
        arg == SECONDARY_CCM ?  "SECONDARY_CCM" : \
        arg == TERTIARY_CCM ?  "TERTIARY_CCM" : \
        arg == MAX_CCM ?  "MAX_CCM" : \
        arg == UNUSED_PARAM ?  "UNUSED_PARAM" : "Unknown")\


#endif 

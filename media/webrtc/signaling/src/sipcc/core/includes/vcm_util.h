



#ifndef VCM_UTIL_H_
#define VCM_UTIL_H_
#include "vcm.h"
#include "phone_types.h"








vcm_media_payload_type_t vcm_rtp_to_media_payload (int32_t ptype,
                                            int32_t dynamic_ptype_value,
                                            uint16_t mode);


#endif 

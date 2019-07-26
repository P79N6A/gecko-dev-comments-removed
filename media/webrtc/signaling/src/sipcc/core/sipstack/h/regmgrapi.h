



#ifndef _REGMGRAPI_H_
#define _REGMGRAPI_H_

#include "sessionConstants.h"

typedef enum reg_mode_t_ {
    REG_MODE_CCM = CC_MODE_CCM,
    REG_MODE_NON_CCM = CC_MODE_NONCCM,
} reg_mode_t;

reg_mode_t sip_regmgr_get_cc_mode(line_t line);

#endif 

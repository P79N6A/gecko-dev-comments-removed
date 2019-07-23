




































#ifndef _tmUtils_H_
#define _tmUtils_H_

#include "nscore.h"
#include "nsError.h"
#include "nsID.h"
#include "prlog.h"
#include <stdio.h>




#define TRANSACTION_MODULE_ID                         \
{ /* c3dfbcd5-f51d-420b-abf4-3bae445b96a9 */          \
    0xc3dfbcd5,                                       \
    0xf51d,                                           \
    0x420b,                                           \
    {0xab, 0xf4, 0x3b, 0xae, 0x44, 0x5b, 0x96, 0xa9}  \
}








#define NS_ERROR_MODULE_TM       27   /* XXX goes in nserror.h -- integrating with ns error codes */

#define TM_ERROR                          NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_TM, 1)
#define TM_ERROR_WRONG_QUEUE              NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_TM, 2)
#define TM_ERROR_NOT_POSTED               NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_TM, 3)
#define TM_ERROR_QUEUE_EXISTS             NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_TM, 4)
#define TM_SUCCESS_DELETE_QUEUE           NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_TM, 6)



#define TM_INVALID_ID 0xFFFFFFFF
#define TM_INVALID 0xFFFFFFFF
#define TM_NO_ID 0xFFFFFFFE


enum {
  TM_ATTACH = 0, 
  TM_ATTACH_REPLY,
  TM_POST,
  TM_POST_REPLY,
  TM_NOTIFY,
  TM_FLUSH,
  TM_FLUSH_REPLY,
  TM_DETACH,
  TM_DETACH_REPLY 
};

#endif


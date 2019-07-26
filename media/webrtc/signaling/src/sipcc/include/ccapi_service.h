






































#ifndef _CCAPI_SERVICE_H_
#define _CCAPI_SERVICE_H_
#include "cc_constants.h"
















cc_return_t CCAPI_Service_create();






cc_return_t CCAPI_Service_destroy();






cc_return_t CCAPI_Service_start();






cc_return_t CCAPI_Service_stop();


















cc_return_t CCAPI_Service_reregister (int device_handle, const char *device_name,
                             const char *cfg, int from_memory);






void CCAPI_Service_reset_request();

#endif 





#ifndef _CCAPI_SERVICE_H_
#define _CCAPI_SERVICE_H_
#include "cc_constants.h"




extern int g_dev_hdl;
#define G_DEV_NAME_SIZE 64
extern char g_dev_name[G_DEV_NAME_SIZE];
#define G_CFG_P_SIZE 256
extern char g_cfg_p[G_CFG_P_SIZE];
extern int g_compl_cfg;

















cc_return_t CCAPI_Service_create();






cc_return_t CCAPI_Service_destroy();






cc_return_t CCAPI_Service_start();






cc_return_t CCAPI_Service_stop();


















cc_return_t CCAPI_Service_reregister (int device_handle, const char *device_name,
                             const char *cfg, int from_memory);






void CCAPI_Service_reset_request();

#endif 

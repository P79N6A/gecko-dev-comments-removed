



#ifndef _CC_SERVICE_H_
#define _CC_SERVICE_H_

#include "cc_constants.h"












cc_return_t CC_Service_init();









cc_return_t CC_Service_create();






cc_return_t CC_Service_destroy();






cc_return_t CC_Service_start();







cc_return_t CC_Service_shutdown(cc_shutdown_reason_t mgmt_reason, cc_string_t reason_string);







cc_return_t CC_Service_unregisterAllLines(cc_shutdown_reason_t mgmt_reason, cc_string_t reason_string);







cc_return_t CC_Service_registerAllLines(cc_shutdown_reason_t mgmt_reason, cc_string_t reason_string);





cc_return_t CC_Service_restart();

#endif 

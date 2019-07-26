



#ifndef _CONFIG_API_H_
#define _CONFIG_API_H_

#include "cc_types.h"
















void configFetchReq(int device_handle);













void configParserError(void);




void CCAPI_Start_response(int device_handle, const char *device_name, const char *sipUser, const char *sipPassword, const char *sipDomain);



























void configApplyConfigNotify(cc_string_t config_version,
		cc_string_t dial_plan_version,
		cc_string_t fcp_version,
		cc_string_t cucm_result,
		cc_string_t load_id,
		cc_string_t inactive_load_id,
		cc_string_t load_server,
		cc_string_t log_server,
		cc_boolean ppid);












cc_boolean dialPlanFetchReq(int device_handle, char* dialPlanFileName); 












cc_boolean fcpFetchReq(int device_handle, char* fcpFileName); 


cc_boolean CCAPI_Config_set_server_address(const char *ip_address);
cc_boolean CCAPI_Config_set_transport_udp(const cc_boolean is_udp);
cc_boolean CCAPI_Config_set_local_voip_port(const int port);
cc_boolean CCAPI_Config_set_remote_voip_port(const int port);
int CCAPI_Config_get_local_voip_port();
int CCAPI_Config_get_remote_voip_port();
const char* CCAPI_Config_get_version();
cc_boolean CCAPI_Config_set_p2p_mode(const cc_boolean is_p2p);
cc_boolean CCAPI_Config_set_sdp_mode(const cc_boolean is_sdp);
cc_boolean CCAPI_Config_set_avp_mode(const cc_boolean is_rtpsavpf);

#endif  

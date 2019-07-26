






































#ifndef CONFIG_PARSER_H_
#define CONFIG_PARSER_H_

#include "cc_constants.h"
#include "cc_types.h"
#include "cc_config.h"
#include "ccapi.h"
#include "phone_debug.h"
#include "debug.h"
#include "prot_configmgr.h"
#include "call_logger.h"
#include "sip_common_transport.h"
#include "sip_ccm_transport.h"

#define MAX_CFG_VERSION_STAMP_LEN 80









boolean is_cfgid_in_restart_list(int cfgid);





void compare_or_set_byte_value(int cfgid, unsigned char value, const unsigned char * config_name);





void compare_or_set_boolean_value(int cfgid, cc_boolean value, const unsigned char * config_name);





void compare_or_set_int_value(int cfgid, int value, const unsigned char * config_name);





void compare_or_set_string_value (int cfgid, const char* value, const unsigned char * config_name);





void config_fetch_dialplan(char *filename);





void config_fetch_fcp(char *filename);





void config_set_autoreg_properties ();





void update_security_mode_and_ports(void);







void config_get_mac_addr (char *maddr);




void config_set_ccm_ip_mac ();





void config_setup_elements ( const char *sipUser, const char *sipPassword, const char *sipDomain);





void config_setup_server_address (const char *sipDomain);




void config_setup_transport_udp(const cc_boolean is_udp);




void config_setup_local_voip_control_port(const int voipControlPort);




void config_setup_remote_voip_control_port(const int voipControlPort);




int config_get_local_voip_control_port();




int config_get_remote_voip_control_port();




const char* config_get_version();




void config_setup_p2p_mode(const cc_boolean is_p2p);




void config_setup_sdp_mode(const cc_boolean is_sdp);




void config_setup_avp_mode(const cc_boolean is_rtpsavpf);

















int config_parser_main( char *config, int complete_config);




int config_setup_main( const char *sipUser, const char *sipPassword, const char *sipDomain);


#endif 

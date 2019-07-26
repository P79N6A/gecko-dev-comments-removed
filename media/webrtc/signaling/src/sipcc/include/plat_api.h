






































#ifndef _PLAT_API_H_
#define _PLAT_API_H_

#include "cc_constants.h"
#include "cpr_socket.h"
#include "cc_types.h"




#define CC_UNREG_REASON_UNSPECIFIED                       0

#define CC_UNREG_REASON_TCP_TIMEOUT                       10
#define CC_UNREG_REASON_CM_RESET_TCP                      12
#define CC_UNREG_REASON_CM_ABORTED_TCP                    13
#define CC_UNREG_REASON_CM_CLOSED_TCP                     14
#define CC_UNREG_REASON_REG_TIMEOUT                       17
#define CC_UNREG_REASON_FALLBACK                          18
#define CC_UNREG_REASON_PHONE_KEYPAD                      20
#define CC_UNREG_REASON_RESET_RESET                       22
#define CC_UNREG_REASON_RESET_RESTART                     23
#define CC_UNREG_REASON_PHONE_REG_REJ                     24
#define CC_UNREG_REASON_PHONE_INITIALIZED                 25
#define CC_UNREG_REASON_VOICE_VLAN_CHANGED                26
#define CC_UNREG_REASON_POWER_SAVE_PLUS                   32

#define CC_UNREG_REASON_VERSION_STAMP_MISMATCH            100
#define CC_UNREG_REASON_VERSION_STAMP_MISMATCH_CONFIG     101
#define CC_UNREG_REASON_VERSION_STAMP_MISMATCH_SOFTKEY    102
#define CC_UNREG_REASON_VERSION_STAMP_MISMATCH_DIALPLAN   103
#define CC_UNREG_REASON_APPLY_CONFIG_RESTART              104
#define CC_UNREG_REASON_CONFIG_RETRY_RESTART              105
#define CC_UNREG_REASON_TLS_ERROR                         106
#define CC_UNREG_REASON_RESET_TO_INACTIVE_PARTITION       107
#define CC_UNREG_REASON_VPN_CONNECTIVITY_LOST             108

#define CC_IPPROTO_UDP 17
#define CC_IPPROTO_TCP 6





typedef enum
{
    PLAT_SOCK_SECURE,
    PLAT_SOCK_NONSECURE
} plat_soc_status_e;




typedef enum
{
    PLAT_SOCK_CONN_OK,
    PLAT_SOCK_CONN_WAITING,
    PLAT_SOCK_CONN_FAILED
} plat_soc_connect_status_e;




typedef enum
{
    PLAT_SOCK_CUCM
} plat_soc_connect_type_e;




typedef enum
{
    PLAT_SOCK_AUTHENTICATED,
    PLAT_SOCK_ENCRYPTED,
    PLAT_SOCK_NON_SECURE
} plat_soc_connect_mode_e;




typedef enum
{
    CC_DEBUG_CCAPP,
    CC_DEBUG_CONFIG_CACHE,
    CC_DEBUG_SIP_ADAPTER,
    CC_DEBUG_CCAPI,
    CC_DEBUG_CC_MSG,
    CC_DEBUG_FIM,
    CC_DEBUG_FSM,
    CC_DEBUG_AUTH,
    CC_DEBUG_GSM,
    CC_DEBUG_LSM,
    CC_DEBUG_FSM_CAC,
    CC_DEBUG_DCSM,
    CC_DEBUG_SIP_TASK,
    CC_DEBUG_SIP_STATE,
    CC_DEBUG_SIP_MSG,
    CC_DEBUG_SIP_REG_STATE,
    CC_DEBUG_SIP_TRX,
    CC_DEBUG_TIMERS,
    CC_DEBUG_SIP_DM,
    CC_DEBUG_CCDEFAULT, 
    CC_DEBUG_DIALPLAN,
    CC_DEBUG_KPML,
    CC_DEBUG_REMOTE_CC,
    CC_DEBUG_SIP_PRESENCE,
    CC_DEBUG_CONFIG_APP,
    CC_DEBUG_CALL_EVENT,
    CC_DEBUG_PLAT,
    CC_DEBUG_NOTIFY,
    CC_DEBUG_CPR_MEMORY, 
    CC_DEBUG_MAX        
} cc_debug_category_e;





typedef enum
{
    CC_DEBUG_SHOW_FSMCNF,
    CC_DEBUG_SHOW_FSMDEF,
    CC_DEBUG_SHOW_FSMXFR,
    CC_DEBUG_SHOW_FSMB2BCNF,
    CC_DEBUG_SHOW_DCSM,
    CC_DEBUG_SHOW_FIM,
    CC_DEBUG_SHOW_FSM,
    CC_DEBUG_SHOW_LSM,
    CC_DEBUG_SHOW_BULK_REGISTER,
    CC_DEBUG_SHOW_KPML,
    CC_DEBUG_SHOW_REMOTE_CC,
    CC_DEBUG_SHOW_CONFIG_CACHE,
    CC_DEBUG_SHOW_SUBS_STATS,
    CC_DEBUG_SHOW_PUBLISH_STATS,
    CC_DEBUG_SHOW_REGISTER,
    CC_DEBUG_SHOW_DIALPLAN,
    CC_DEBUG_SHOW_CPR_MEMORY, 

    CC_DEBUG_SHOW_MAX
} cc_debug_show_options_e;




typedef enum
{
    CC_DEBUG_CLEAR_CPR_MEMORY,
    CC_DEBUG_CLEAR_MAX
} cc_debug_clear_options_e;




typedef enum
{
    CC_DEBUG_CPR_MEM_TRACKING,
    CC_DEBUG_CPR_MEM_POISON
} cc_debug_cpr_mem_options_e;




typedef enum
{
    CC_DEBUG_CLEAR_CPR_TRACKING,
    CC_DEBUG_CLEAR_CPR_STATISTICS
} cc_debug_clear_cpr_options_e;




typedef enum
{
    CC_DEBUG_SHOW_CPR_CONFIG,
    CC_DEBUG_SHOW_CPR_HEAP_GUARD,
    CC_DEBUG_SHOW_CPR_STATISTICS,
    CC_DEBUG_SHOW_CPR_TRACKING
} cc_debug_show_cpr_options_e;




typedef enum
{
    CC_DEBUG_DISABLE,
    CC_DEBUG_ENABLE
} cc_debug_flag_e;


typedef enum {
       DHCP_STATUS_GOOD = 1,
       DHCP_STATUS_TIMEOUT,
       DHCP_STATUS_DISABLED
 } dhcp_status_e;



typedef enum {
   DNS_STATUS_GOOD = 1,
   DNS_STATUS_TIMEOUT,
   DNS_STATUS_DID_NOT_RESOLVE,
   DNS_STATUS_NA_IP_CONFIGURED
} ucm_dns_resolution_status_e;

#define LEN32   32
#define IP_ADDR_MAX_LEN       32
#define PORT_MAX_LEN          20
#define STATUS_MAX_LEN        4
#define LEN80   80
#define WIRED_PROP_PREFIX     "dhcp.eth0"
#define WIRELESS_PROP_PREFIX  "dhcp.mlan0"
#define WIRED_INT 1
#define WIFI_INT  2











int platThreadInit(char * tname);









int platInit();






void debugInit();






char *platGetModel();





typedef enum vcm_audio_device_type {
    VCM_AUDIO_DEVICE_NONE,
    VCM_AUDIO_DEVICE_HEADSET,
    VCM_AUDIO_DEVICE_SPEAKER
} plat_audio_device_t;



















void platAddCallControlClassifiers(unsigned long myIPAddr, unsigned short myPort,
	unsigned long cucm1IPAddr, unsigned short cucm1Port,
	unsigned long cucm2IPAddr, unsigned short cucm2Port,
	unsigned long cucm3IPAddr, unsigned short cucm3Port,
	unsigned char  protocol);






void platRemoveCallControlClassifiers();






cc_boolean	platWlanISActive();









boolean	platIsNetworkInterfaceChanged();














int platGetActiveInactivePhoneLoadName(char * image_a, char * image_b, int len);










int platGetPhraseText(int index, char* phrase, unsigned int len);









void platSetUnregReason(int reason);







int platGetUnregReason();






void platSetCucmRegTime (void);






void platSetKPMLConfig(cc_kpml_config_t kpml_config);












boolean platGetMWIStatus(cc_lineid_t line);






boolean platGetSpeakerHeadsetMode();






















plat_soc_status_e platSecIsServerSecure(void);









































cpr_socket_t
platSecSocConnect (char *host,
                  int     port,
                  int     ipMode,
                  boolean mode,
                  unsigned int tos,
                  plat_soc_connect_mode_e connectionMode,
                  cc_uint16_t *localPort);

















plat_soc_connect_status_e platSecSockIsConnected (cpr_socket_t sock);





































int platGenerateCryptoRand(cc_uint8_t *buf, int *len);



































ssize_t
platSecSocSend (cpr_socket_t soc,
         CONST void *buf,
         size_t len);





























ssize_t
platSecSocRecv (cpr_socket_t soc,
         void * RESTRICT buf,
         size_t len);
















cpr_status_e
platSecSocClose (cpr_socket_t soc);













void platSetSISProtocolVer(cc_uint32_t a, cc_uint32_t b, cc_uint32_t c, char* name);











void
platGetSISProtocolVer (cc_uint32_t *a, cc_uint32_t *b, cc_uint32_t *c, char* name);






char *platGetIPAddr();








void platGetMacAddr(char *addr);







void platGetDefaultGW(char *addr);












int platGetFeatureAllowed(cc_sis_feature_id_e featureId);







void platSetStatusMessage(char *msg);



























void debugSet(cc_debug_category_e category, cc_debug_flag_e flag, ...);

















int debugShow(cc_debug_show_options_e category, ...);










void debugShowTech(void);

















int debugClear(cc_debug_clear_options_e category, ...);

















int debugif_printf(const char *_format, ...);









void platSetSpeakerMode(cc_boolean state);









int platGetAudioDeviceStatus(plat_audio_device_t device_type);







cc_ulong_t platGetDefaultgw();


#endif 

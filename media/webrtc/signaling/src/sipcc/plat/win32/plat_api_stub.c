



#include "cpr_types.h"
#include "cc_constants.h"
#include "cpr_socket.h"
#include "plat_api.h"
#include "plat_debug.h"
#include "phone_types.h"





int sipcc_platThreadInit(char * tname)
{
    return 0;
}









int sipcc_platInit()
{
    return 0;
}









void sipcc_debugInit()
{
    return ;
}




void sipcc_platAddCallControlClassifiers(unsigned long myIPAddr, unsigned short myPort,
	unsigned long cucm1IPAddr, unsigned short cucm1Port,
	unsigned long cucm2IPAddr, unsigned short cucm2Port,
	unsigned long cucm3IPAddr, unsigned short cucm3Port,
	unsigned char  protocol)
{
    return;
}






cpr_ip_mode_e sipcc_platGetIpAddressMode()
{
    return CPR_IP_MODE_IPV4;
}




void sipcc_platRemoveCallControlClassifiers()
{
    return;
}




cc_boolean	sipcc_platWlanISActive()
{
    return TRUE;
}




boolean	sipcc_platIsNetworkInterfaceChanged()
{
    return TRUE;
}






int sipcc_platGetActiveInactivePhoneLoadName(char * image_a, char * image_b, int len)
{
    return 0;
}








int sipcc_platGetPhraseText(int index, char* phrase, unsigned int len)
{
    return 0;
}






void sipcc_platSetUnregReason(int reason)
{
    return;
}





int sipcc_platGetUnregReason()
{
    return 0;
}






void sipcc_platSetKPMLConfig(cc_kpml_config_t kpml_config)
{
    return ;
}






boolean sipcc_platGetMWIStatus(cc_lineid_t line)
{
    return TRUE;
}























plat_soc_status_e sipcc_platSecIsServerSecure(void)
{
    return PLAT_SOCK_NONSECURE;
}










































cpr_socket_t
sipcc_platSecSocConnect (char *host,
                  int     port,
                  int     ipMode,
                  boolean mode,
                  unsigned int tos,
                  plat_soc_connect_mode_e connectionMode,
                  uint16_t *localPort)
{
    return 0;
}

















plat_soc_connect_status_e sipcc_platSecSockIsConnected (cpr_socket_t sock)
{
    return PLAT_SOCK_CONN_OK;
}





































int sipcc_platGenerateCryptoRand(cc_uint8_t *buf, int *len)
{
     return 0;
}



































ssize_t
platSecSocSend (cpr_socket_t soc,
         CONST void *buf,
         size_t len)
{
    return 0;
}





























ssize_t
platSecSocRecv (cpr_socket_t soc,
         void * RESTRICT buf,
         size_t len)
{
    return 0;
}
















cpr_status_e
platSecSocClose (cpr_socket_t soc)
{
    return CPR_SUCCESS;
}












void sipcc_platSetSISProtocolVer(uint32_t a, uint32_t b, uint32_t c, char* name)
{
    return;
}










void
sipcc_platGetSISProtocolVer (uint32_t *a, uint32_t *b, uint32_t *c, char* name)
{
    return;
}


void sipcc_debug_bind_keyword(const char *cmd, int32_t *flag_ptr)
{
    return;
}
void bind_debug_func_keyword(const char *cmd, debug_callback func)
{
    return;
}
void bind_show_keyword(const char *cmd, show_callback func)
{
    return;
}
void bind_show_tech_keyword(const char *cmd, show_callback func,
                            boolean show_tech)
{
    return;
}
void bind_clear_keyword(const char *cmd, clear_callback func)
{
    return;
}

void ci_bind_cmd(const char *cmd, ci_callback func, ci_cmd_block_t *blk)
{
    return;
}

boolean sipcc_platGetSpeakerHeadsetMode()
{
    return TRUE;
}












int sipcc_platGetFeatureAllowed(cc_sis_feature_id_e featureId)
{
    return TRUE;
}


int sipcc_platGetAudioDeviceStatus(plat_audio_device_t device_type)
{
    return 0;
}

void sipcc_platSetSpeakerMode(cc_boolean state)
{
    return;
}


void sipcc_platGetMacAddr (char *maddr)
{
	return;
}


void sipcc_NotifyStateChange(callid_t callid, int32_t state)
{
	
}







cc_ulong_t sipcc_platGetDefaultgw(){
	return 0;
}

 




 void sipcc_platSetCucmRegTime (void) {
     
 }





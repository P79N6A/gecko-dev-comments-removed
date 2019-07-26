






































#include "cpr_types.h"
#include "cc_constants.h"
#include "cpr_socket.h"
#include "plat_api.h"
#include "plat_debug.h"
#include "phone_types.h"





int platThreadInit(char * tname)
{
    return 0;
}









int platInit()
{
    return 0;
}









void debugInit()
{
    return ;
}




void platAddCallControlClassifiers(unsigned long myIPAddr, unsigned short myPort,
	unsigned long cucm1IPAddr, unsigned short cucm1Port,
	unsigned long cucm2IPAddr, unsigned short cucm2Port,
	unsigned long cucm3IPAddr, unsigned short cucm3Port,
	unsigned char  protocol)
{
    return;
}






cpr_ip_mode_e platGetIpAddressMode()
{
    return CPR_IP_MODE_IPV4;
}




void platRemoveCallControlClassifiers()
{
    return;
}




cc_boolean	platWlanISActive()
{
    return TRUE;
}




boolean	platIsNetworkInterfaceChanged()
{
    return TRUE;
}






int platGetActiveInactivePhoneLoadName(char * image_a, char * image_b, int len)
{
    return 0;
}








int platGetPhraseText(int index, char* phrase, unsigned int len)
{
    return 0;
}






void platSetUnregReason(int reason)
{
    return;
}





int platGetUnregReason()
{
    return 0;
}






void platSetKPMLConfig(cc_kpml_config_t kpml_config)
{
    return ;
}






boolean platGetMWIStatus(cc_lineid_t line)
{
    return TRUE;
}























plat_soc_status_e platSecIsServerSecure(void)
{
    return PLAT_SOCK_NONSECURE;
}










































cpr_socket_t
platSecSocConnect (char *host,
                  int     port,
                  int     ipMode,
                  boolean mode,
                  unsigned int tos,
                  plat_soc_connect_mode_e connectionMode,
                  uint16_t *localPort)
{
    return 0;
}

















plat_soc_connect_status_e platSecSockIsConnected (cpr_socket_t sock)
{
    return PLAT_SOCK_CONN_OK;
}





































int platGenerateCryptoRand(cc_uint8_t *buf, int *len)
{
     return 0;
}





















































































































void platSetSISProtocolVer(uint32_t a, uint32_t b, uint32_t c, char* name)
{
    return;
}










void
platGetSISProtocolVer (uint32_t *a, uint32_t *b, uint32_t *c, char* name)
{
    return;
}


void debug_bind_keyword(const char *cmd, int32_t *flag_ptr)
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

boolean platGetSpeakerHeadsetMode()
{
    return TRUE;
}












int platGetFeatureAllowed(cc_sis_feature_id_e featureId)
{
    return TRUE;
}


int platGetAudioDeviceStatus(plat_audio_device_t device_type)
{
    return 0;
}

void platSetSpeakerMode(cc_boolean state)
{
    return;
}


void platGetMacAddr (char *maddr)
{
	return;
}


void NotifyStateChange(callid_t callid, int32_t state)
{
	
}







void platGetDefaultGW (char* addr)
{
    return;
}







cc_ulong_t platGetDefaultgw(){
	return 0;
}

 




 void platSetCucmRegTime (void) {
     
 }












































#ifdef UTESTONLY
#include <stdio.h>
#else
#include "cpr_stdio.h"
#include "cpr_strings.h"
#include "cpr_stdlib.h"
#endif

#include "cpr_types.h"
#include "cc_constants.h"
#include "cpr_socket.h"
#include "plat_api.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/if.h>





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




void platRemoveCallControlClassifiers()
{
    return;
}




cc_boolean	platWlanISActive()
{
    return FALSE;
}






int platGetActiveInactivePhoneLoadName(char * image_a, char * image_b, int len)
{
    
    memset(image_a, 0, len);
    memset(image_b, 0, len);

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




































































































#define MAX_LEN_SIS_VER 64
static cc_uint32_t major=1, minor=0, addtnl =0;
static char sis_ver_name[MAX_LEN_SIS_VER] = {0};












void platSetSISProtocolVer(cc_uint32_t a, cc_uint32_t b, cc_uint32_t c, char* name)
{
    major = a;
    minor = b;
    addtnl = c;
    if (name) {
        strncpy(sis_ver_name, name, strlen(name));
    } else {
        memset(sis_ver_name, 0, MAX_LEN_SIS_VER);
    }
}










void
platGetSISProtocolVer (cc_uint32_t *a, cc_uint32_t *b, cc_uint32_t *c, char* name)
{
    if ( a ) {
        *a = major;
    }

    if ( b ) {
        *b = minor;
    }

    if ( c ) {
        *c = addtnl;
    }

    if (name) {
        strncpy(name, sis_ver_name, strlen(sis_ver_name));
    }
}

void debug_bind_keyword(const char *cmd, int32_t *flag_ptr)
{
    return;
}

void platSetSpeakerMode(cc_boolean state)
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

void platGetMacAddr (char *maddr)
{
	maddr[0] = 0x44;
	maddr[1] = 0x22;
	maddr[2] = 0x33;
	maddr[3] = 0x44;
	maddr[4] = 0x55;
	maddr[5] = 0x66;
}





char * platGetIPAddr () 
{
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    
    ifr.ifr_addr.sa_family = AF_INET;

    
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);

    ioctl(fd, SIOCGIFADDR, &ifr);
    
    close(fd);
    
    return (inet_ntoa((((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr)));
}






void platSetCucmRegTime (void) {
    
}







cc_ulong_t platGetDefaultgw(){
	return 0;
}


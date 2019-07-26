



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "cpr.h"
#include "cpr_socket.h"
#include "cpr_in.h"
#include "cpr_stdlib.h"
#include "phntask.h"
#include <stdarg.h>
#include "configmgr.h"
#include "ci.h"
#include "debug.h"
#include "config.h"
#include "vcm.h"
#include "direct.h"
#include "dialplan.h"
#include "dns_utils.h"
#include "time2.h"
#include "debug.h"
#include "phone_debug.h"
#include "util_string.h"
#include "sip_common_transport.h"


uint32_t gtick;




#define DNS_ERR_HOST_UNAVAIL   5
typedef void *srv_handle_t;















enum AUDIOFLAGS {
    NO_AUDIOFLAGS                  = 0x0000,
    HANDSET_AUDIOFLAG              = 0x0001<<0,
    SPEAKERPHONE_AUDIOFLAG         = 0x0001<<1,
    HEADSET_AUDIOFLAG              = 0x0001<<2,
    SPEAKER_ON_REQUEST_AUDIOFLAG   = 0x0001<<3,
    SPEAKER_OFF_REQUEST_AUDIOFLAG  = 0x0001<<4,
    ANY_AUDIOFLAGS                 = (HANDSET_AUDIOFLAG      |
                                      SPEAKERPHONE_AUDIOFLAG |
                                      HEADSET_AUDIOFLAG)
};


static boolean b_Locked = TRUE;
boolean CFGIsLocked (void)
{
    return (b_Locked);
}








void cfg_sanity_check_media_range (void)
{
    int32_t start_port = 0;
    int32_t end_port = 0;
    boolean changed = FALSE;

    config_get_value(CFGID_MEDIA_PORT_RANGE_START,
                     &start_port, sizeof(start_port));
    config_get_value(CFGID_MEDIA_PORT_RANGE_END,
                     &end_port, sizeof(end_port));

    
    if (start_port & 0x1) {
        start_port =  start_port & ~0x1;
        changed = TRUE;
    }
    if (end_port & 0x1) {
        end_port = end_port & ~0x1;
        changed = TRUE;
    }

    


    if (end_port < start_port) {
        unsigned int temp = end_port;
        end_port = start_port;
        start_port = temp;
        changed = TRUE;
    }

    if ((end_port - start_port) < 4) {
        start_port  = RTP_START_PORT;
        end_port = RTP_END_PORT;
        changed = TRUE;
    }

    















    if ((start_port < RTP_START_PORT) || (start_port > (RTP_END_PORT - 4))) {
        start_port = RTP_START_PORT;
        changed = TRUE;
    }

    if ((end_port < (RTP_START_PORT + 4)) || (end_port > RTP_END_PORT)) {
        end_port = RTP_END_PORT;
        changed = TRUE;
    }

    if (changed) {
        config_set_value(CFGID_MEDIA_PORT_RANGE_START,
                         &start_port, sizeof(start_port));
        config_set_value(CFGID_MEDIA_PORT_RANGE_END,
                         &end_port, sizeof(end_port));
    }
}





#define MAX_DEBUG_CMDS 40
static debug_entry_t debug_table[MAX_DEBUG_CMDS];











extern struct tm *gmtime_r(const time_t *timer, struct tm *pts)
{
    errno_t err;

    if (pts == NULL)
    {
        return NULL;
    }

    memset(pts, 0, sizeof(struct tm));

    if (timer == NULL)
    {
        return NULL;
    }

    
    
    err = gmtime_s(pts, timer);
    if (err)
    {
        memset(pts, 0, sizeof(struct tm));
        return NULL;
    }

    return pts;
}



unsigned long seconds_to_gmt_string (unsigned long seconds, char *gmt_string)
{
    sprintf(gmt_string, "%s, %02d %s %04d %02d:%02d:%02d GMT",
        "Mon",
        1,
        "Jan",
        1,
        1,
        1,
        1);
    return 0;
}







int PhoneAudioFlags = NO_AUDIOFLAGS;

#include "lsm.h"
void OnTerminateCall()
{
	terminate_active_calls();
}






int32_t
cpr_debug_memory_cli (int32_t argc, const char *argv[])
{
        return 1;
}


int debugif_printf_response(int responseType,const char *_format, ...)
{
    return 0;
}


cc_int32_t
cpr_clear_memory (cc_int32_t argc, const char *argv[])
{
    return 0;
}


cc_int32_t
cpr_show_memory (cc_int32_t argc, const char *argv[])
{
    return 0;
}

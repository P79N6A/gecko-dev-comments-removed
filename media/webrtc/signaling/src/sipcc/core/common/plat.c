






































#include <string.h>
#include "cpr.h"
#include "phone_debug.h"
#include "CCProvider.h"
#include "ccsip_pmh.h"
#include "sessionTypes.h"
#include "ccapp_task.h"


#define STRLIB_CREATE(str)  (str)?strlib_malloc((str), strlen((str))):strlib_empty()

void
platform_apply_config (char * configVersionStamp,
                       char * dialplanVersionStamp,
                       char * fcpVersionStamp,
                       char * cucmResult,
                       char * loadId, 
                       char * inactiveLoadId, 
                       char * loadServer,
                       char * logServer,
                       boolean ppid);
















void
platform_apply_config (char * configVersionStamp,
                       char * dialplanVersionStamp,
                       char * fcpVersionStamp,
                       char * cucmResult,
                       char * loadId,
                       char * inactiveLoadId, 
                       char * loadServer,
                       char * logServer,
                       boolean ppid)
{
    static const char fname[] = "platform_apply_config";
    session_mgmt_t msg;

    fcpVersionStamp = (fcpVersionStamp != NULL) ? fcpVersionStamp : "";

    
    CCAPP_DEBUG(DEB_F_PREFIX"   configVersionStamp=%s \ndialplanVersionStamp=%s"
           "\nfcpVersionStamp=%s \ncucmResult=%s "
           "\nloadId=%s \ninactiveLoadId=%s \nloadServer=%s \nlogServer=%s "
           "\nppid=%s\n", DEB_F_PREFIX_ARGS(PLAT_API, fname),
           (configVersionStamp != NULL) ? configVersionStamp : "",
           (dialplanVersionStamp != NULL) ? dialplanVersionStamp:"",
           fcpVersionStamp,
           cucmResult != NULL ? cucmResult: "",
           (loadId != NULL) ? loadId : "",
           (inactiveLoadId != NULL) ? inactiveLoadId : "",
           (loadServer != NULL) ? loadServer : "",
           (logServer != NULL) ? logServer : "",
           ppid == TRUE? "True": "False");


    
    msg.func_id = SESSION_MGMT_APPLY_CONFIG;
    msg.data.config.config_version_stamp = STRLIB_CREATE(configVersionStamp);
    msg.data.config.dialplan_version_stamp = STRLIB_CREATE(dialplanVersionStamp);
    msg.data.config.fcp_version_stamp = STRLIB_CREATE(fcpVersionStamp);
    msg.data.config.cucm_result = STRLIB_CREATE(cucmResult);
    msg.data.config.load_id = STRLIB_CREATE(loadId);
    msg.data.config.inactive_load_id = STRLIB_CREATE(inactiveLoadId);
    msg.data.config.load_server = STRLIB_CREATE(loadServer);
    msg.data.config.log_server = STRLIB_CREATE(logServer);
    msg.data.config.ppid = ppid;

    if ( ccappTaskPostMsg(CCAPP_SESSION_MGMT, &msg, sizeof(session_mgmt_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_DEBUG(DEB_F_PREFIX"failed to send platform_apply_config msg\n", DEB_F_PREFIX_ARGS(PLAT_API, fname));
    }
}

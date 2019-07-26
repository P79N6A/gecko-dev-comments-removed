



#include "cpr_types.h"
#include "cpr_stdio.h"
#include "cpr_string.h"
#include "phone.h"
#include "phone_debug.h"
#include "ccsip_register.h"
#include "ccsip_task.h"
#include "ccsip_pmh.h"
#include "config.h"
#include "sip_common_transport.h"
#include "sip_csps_transport.h"
#include "uiapi.h"
#include "sip_interface_regmgr.h"

#include "platform_api.h"

extern void platform_sync_cfg_vers(char *cfg_ver, char *dp_ver, char *softkey_ver);
extern void platform_reg_failover_ind(void *data);
extern void platform_reg_fallback_ind(void *data);
extern int platGetUnregReason();
extern void ccsip_add_wlan_classifiers();
void ccsip_remove_wlan_classifiers();












void
sip_platform_init (void)
{

    
    
    

    
    ccsip_register_cancel(FALSE, TRUE);
    ccsip_register_reset_proxy();

    


    if (PHNGetState() > STATE_IP_CFG) {

        ccsip_add_wlan_classifiers();
        






        ccsip_register_all_lines();
        ui_sip_config_done();
    } else {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX "IP Stack Not Initialized.\n", "sip_platform_init");
    }
}









int
sip_platform_ui_restart (void)
{
    phone_reset(DEVICE_RESTART);
    return TRUE;
}


void
sip_platform_handle_service_control_notify (sipServiceControl_t *scp)
{
    switch (scp->action) {

    case SERVICE_CONTROL_ACTION_RESET:
        platform_reset_req(DEVICE_RESET);
        break;

    case SERVICE_CONTROL_ACTION_RESTART:
        platform_reset_req(DEVICE_RESTART);
        break;

    case SERVICE_CONTROL_ACTION_CHECK_VERSION:
        platform_sync_cfg_vers(scp->configVersionStamp,
                               scp->dialplanVersionStamp,
                               scp->softkeyVersionStamp);
        break;

    case SERVICE_CONTROL_ACTION_APPLY_CONFIG:
        
        platform_apply_config(scp->configVersionStamp, 
                               scp->dialplanVersionStamp,
                               scp->fcpVersionStamp,
                               scp->cucm_result,
                               scp->firmwareLoadId,
                               scp->firmwareInactiveLoadId,
                               scp->loadServer,
                               scp->logServer,
                               scp->ppid);
        break;        
    default:
        break;

    }
}

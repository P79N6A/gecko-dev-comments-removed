



#include <time.h>
#include <string.h>
#include "cpr.h"
#include "cpr_string.h"
#include "phone_debug.h"
#include "prot_configmgr.h"
#include "phone.h"
#include "CCProvider.h"
#include "cpr_stdlib.h"
#include "ccsip_pmh.h"
#include "platform_api.h"
#include <fcntl.h>
#include "ccapp_task.h"





#define PLT_F_PREFIX "PLT : %s : " // requires 1 arg: fname

#define STRLIB_CREATE(str)  (str)?strlib_malloc((str), strlen((str))):strlib_empty()











void platform_sync_cfg_vers(char *cfg_ver, char *dp_ver, char *softkey_ver);
void platform_reg_fallback_ind(int fallback_to);
void platform_reg_failover_ind(int failover_to);



















cpr_ip_mode_e
platform_get_ip_address_mode (void)
{
    cpr_ip_mode_e ip_mode;

    

    
    ip_mode = CPR_IP_MODE_IPV4;

    return (ip_mode);
}









void
platform_reset_req (DeviceResetType action)
{
    static const char fname[] = "platform_reset_req";
    feature_update_t msg;

    DEF_DEBUG(DEB_F_PREFIX"***********%s, requested***********\n",
            DEB_F_PREFIX_ARGS(PLAT_API, fname),
            (action==1)? "RESET":"RESTART");

    msg.sessionType = SESSIONTYPE_CALLCONTROL;
    msg.featureID = DEVICE_SERVICE_CONTROL_REQ;
    msg.update.ccFeatUpd.data.reset_type = action;

    if ( ccappTaskPostMsg(CCAPP_FEATURE_UPDATE, &msg, sizeof(feature_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_DEBUG(DEB_F_PREFIX"failed to send platform_reset_req(%d) msg \n", DEB_F_PREFIX_ARGS(PLAT_API, fname), action);
    }
}












void
platform_sync_cfg_vers (char *cfg_ver, char *dp_ver, char *softkey_ver)
{
    static const char fname[] = "platform_sync_cfg_vers";
    char empty_string[] = "";
    feature_update_t msg;

    if (cfg_ver == NULL) {
        cfg_ver = empty_string;
    }
    if (dp_ver == NULL) {
        dp_ver = empty_string;
    }
    if (softkey_ver == NULL) {
        softkey_ver = empty_string;
    }

    CCAPP_DEBUG(DEB_F_PREFIX"cfg_ver=%s dp_ver=%s sk_ver=%s\n", DEB_F_PREFIX_ARGS(PLAT_API, fname),
        cfg_ver, dp_ver, softkey_ver);

    msg.sessionType = SESSIONTYPE_CALLCONTROL;
    msg.featureID = DEVICE_SYNC_CONFIG_VERSION;
    msg.update.ccFeatUpd.data.cfg_ver_data.cfg_ver = strlib_malloc(cfg_ver, strlen(cfg_ver));
    msg.update.ccFeatUpd.data.cfg_ver_data.dp_ver = strlib_malloc(dp_ver, strlen(dp_ver));
    msg.update.ccFeatUpd.data.cfg_ver_data.softkey_ver = strlib_malloc(softkey_ver, strlen(softkey_ver));

    if ( ccappTaskPostMsg(CCAPP_FEATURE_UPDATE, &msg, sizeof(feature_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_DEBUG(DEB_F_PREFIX"failed to send platform_sync_cfg_vers msg \n",
                DEB_F_PREFIX_ARGS(PLAT_API, fname));
    }
}












void
platform_set_time (long gmt_time)
{
    static const char fname[] = "platform_set_time";
    session_mgmt_t msg;

    CCAPP_DEBUG(DEB_F_PREFIX"setting time to=%ld", DEB_F_PREFIX_ARGS(PLAT_API, fname), gmt_time);

    msg.func_id = SESSION_MGMT_SET_TIME;
    msg.data.time.gmt_time = gmt_time;

    if ( ccappTaskPostMsg(CCAPP_SESSION_MGMT, &msg, sizeof(session_mgmt_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_DEBUG(DEB_F_PREFIX"failed to send platform_set_time msg\n", DEB_F_PREFIX_ARGS(PLAT_API, fname));
    }
}












void
platform_reg_failover_ind (int failover_to)
{
    static const char fname[] = "platform_reg_failover_ind";
    feature_update_t msg;

    DEF_DEBUG(DEB_F_PREFIX"***********Failover to %s=%d ***********\n",
            DEB_F_PREFIX_ARGS(PLAT_API, fname),
            failover_to == CC_TYPE_CCM ? "CC_TYPE_CCM" :
            "Other", failover_to);

    msg.sessionType = SESSIONTYPE_CALLCONTROL;
    msg.featureID = CCAPP_FAILOVER_IND;
    msg.update.ccFeatUpd.data.line_info.info = failover_to;

    if ( ccappTaskPostMsg(CCAPP_FAILOVER_IND, &msg, sizeof(feature_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(PLT_F_PREFIX"failed to send platform_reg_failover_ind(%d) msg \n", fname, failover_to);
    }

}











void
platform_reg_fallback_ind (int fallback_to)
{
    static const char fname[] = "platform_reg_fallback_ind";
    feature_update_t msg;

    DEF_DEBUG(DEB_F_PREFIX"***********Fallback to %d CUCM.***********\n",
            DEB_F_PREFIX_ARGS(PLAT_API, fname),
                                fallback_to);

    msg.sessionType = SESSIONTYPE_CALLCONTROL;
    msg.featureID = CCAPP_FALLBACK_IND;
    msg.update.ccFeatUpd.data.line_info.info = fallback_to;

    if ( ccappTaskPostMsg(CCAPP_FALLBACK_IND, &msg, sizeof(feature_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(PLT_F_PREFIX"failed to send platform_reg_fallback_ind(%d) msg \n", fname, fallback_to);
    }
}












void
platform_reg_fallback_cfm (void)
{
    static const char fname[] = "platform_reg_fallback_cfm";

    DEF_DEBUG(DEB_F_PREFIX"***********Fallback completed.***********\n",
            DEB_F_PREFIX_ARGS(PLAT_API, fname));
}












void
platform_reg_failover_cfm (void)
{
    static const char fname[] = "platform_reg_failover_cfm";

    DEF_DEBUG(DEB_F_PREFIX"***********Failover completed.***********\n",
            DEB_F_PREFIX_ARGS(PLAT_API, fname));
}










void
shutdownCCAck (void)
{
    static const char fname[] = "shutdownCCAck";
    feature_update_t msg;

    CCAPP_DEBUG(DEB_F_PREFIX"\n", DEB_F_PREFIX_ARGS(PLAT_API, fname));

    msg.sessionType = SESSIONTYPE_CALLCONTROL;
    msg.featureID = CCAPP_SHUTDOWN_ACK;

    if ( ccappTaskPostMsg(CCAPP_SHUTDOWN_ACK, &msg, sizeof(feature_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(PLT_F_PREFIX"failed to send shutdownCCAck(%d) msg \n", fname);
    }
}










void
platform_cc_mode_notify (int mode)
{
    static const char fname[] = "platform_cc_mode_notify";
    feature_update_t msg;

    CCAPP_DEBUG(DEB_F_PREFIX"mode =%d\n", DEB_F_PREFIX_ARGS(PLAT_API, fname), mode);

    msg.sessionType = SESSIONTYPE_CALLCONTROL;
    msg.featureID = CCAPP_MODE_NOTIFY;
    msg.update.ccFeatUpd.data.line_info.info = mode;

    if ( ccappTaskPostMsg(CCAPP_MODE_NOTIFY, &msg, sizeof(feature_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(PLT_F_PREFIX"failed to send platform_cc_mode_notify(%d) msg \n", fname, mode);
    }
}


int
platform_get_phrase_text (int ndx, char *outstr, uint32_t len)
{
    static const char fname[] = "platform_get_phrase_text";
    session_mgmt_t msg;

    CCAPP_DEBUG(DEB_F_PREFIX "index=%d\n", DEB_F_PREFIX_ARGS(PLAT_API, fname), ndx);

    msg.func_id = SESSION_MGMT_GET_PHRASE_TEXT;
    msg.data.phrase_text.ndx = ndx;
    msg.data.phrase_text.outstr = outstr;
    msg.data.phrase_text.len = len;

    ccappSyncSessionMgmt(&msg);

    return msg.data.phrase_text.ret_val;
}





void
update_kpmlconfig(int kpmlVal)
{
    static const char fname[] = "update_kpmlconfig";
    session_mgmt_t msg;

    CCAPP_DEBUG(DEB_F_PREFIX "kpml=%d\n", DEB_F_PREFIX_ARGS(PLAT_API, fname), kpmlVal);

    msg.func_id = SESSION_MGMT_UPDATE_KPMLCONFIG;
    msg.data.kpmlconfig.kpml_val = kpmlVal;

    if ( ccappTaskPostMsg(CCAPP_SESSION_MGMT, &msg, sizeof(session_mgmt_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_DEBUG(DEB_F_PREFIX"failed to send update_kpmlconfig msg\n", DEB_F_PREFIX_ARGS(PLAT_API, fname));
    }
}


boolean
check_speaker_headset_mode()
{
    static const char fname[] = "check_speaker_headset_mode";

    CCAPP_DEBUG(DEB_F_PREFIX "checking SPEAKER and HEADSET active or not\n", DEB_F_PREFIX_ARGS(PLAT_API, fname));

    return platGetSpeakerHeadsetMode();
}











void
platform_logout_reset_req(void){
    static const char fname[] = "platform_logout_reset_req";
    feature_update_t msg;

    CCAPP_DEBUG(DEB_F_PREFIX"\n", DEB_F_PREFIX_ARGS(PLAT_API, fname));

    msg.sessionType = SESSIONTYPE_CALLCONTROL;
    msg.featureID = CCAPP_LOGOUT_RESET;

    if ( ccappTaskPostMsg(CCAPP_FALLBACK_IND, &msg, sizeof(feature_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(PLT_F_PREFIX"failed to send Logout_Reset(%d) msg \n", fname);
    }
    return;
}


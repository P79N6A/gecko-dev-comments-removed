






































#include "cpr_types.h"
#include "cpr_stdio.h"
#include "cpr_string.h"
#include "cpr_memory.h"
#include "ccsip_task.h"
#include "debug.h"
#include "phone_debug.h"
#include "phntask.h"
#include "phone.h"
#include "text_strings.h"
#include "string_lib.h"
#include "gsm.h"
#include "sip_common_transport.h"
#include "sip_common_regmgr.h"
#include "sip_interface_regmgr.h"
#include "ccsip_subsmanager.h"
#include "platform_api.h"

extern ccm_act_stdby_table_t CCM_Active_Standby_Table;
extern cc_config_table_t CC_Config_Table[];














void
sip_regmgr_send_status (reg_srcs_t src_id, reg_status_t msg_id)
{
    static const char fname[] = "sip_regmgr_send_status";

    CCSIP_DEBUG_STATE(DEB_F_PREFIX"src_id: %d msg_id: %d", DEB_F_PREFIX_ARGS(SIP_REG, fname), src_id, msg_id);

    if (msg_id == REG_ALL_FAIL) {
        
        ui_reg_all_failed();
        CCSIP_DEBUG_REG_STATE(DEB_F_PREFIX"REG ALL FAILED \n", DEB_F_PREFIX_ARGS(SIP_REG, fname));
    }
    return;
}















reg_mode_t
sip_regmgr_get_cc_mode (line_t line)
{
    if (CCM_Active_Standby_Table.active_ccm_entry) {
        return (REG_MODE_CCM);
    } else {
        return (REG_MODE_NON_CCM);
    }
}


boolean
sip_platform_is_phone_idle (void)
{
    if (gsm_is_idle()) {
        return (TRUE);
    }
    return (FALSE);
}














void
sip_platform_fallback_ind (CCM_ID ccm_id)
{
    static const char fname[] = "sip_platform_fallback_ind";
    int from_id = CC_TYPE_CCM;

    CCSIP_DEBUG_REG_STATE(DEB_F_PREFIX"ccm-id: %d", DEB_F_PREFIX_ARGS(SIP_FALLBACK, fname), ccm_id);

    platform_reg_fallback_ind((void *)(long) from_id);
}













boolean plat_is_network_interface_changed(void );
void
sip_platform_failover_ind (CCM_ID ccm_id)
{
    static const char fname[] = "sip_platform_failover_ind";
    int to_id = CC_TYPE_CCM;

    CCSIP_DEBUG_REG_STATE(DEB_F_PREFIX"ccm-id=%s=%d", DEB_F_PREFIX_ARGS(SIP_FAILOVER, fname), 
        ccm_id == PRIMARY_CCM ? "PRIMARY_CCM" :
        ccm_id == SECONDARY_CCM ? "SECONDARY_CCM" :
        ccm_id == TERTIARY_CCM ? "TERTIARY_CCM" : "Unknown",
        ccm_id);

    if (plat_is_network_interface_changed()) {
        CCSIP_DEBUG_REG_STATE(DEB_F_PREFIX"network i/f changed, sending REG_ALL_FAIL instead", DEB_F_PREFIX_ARGS(SIP_FAILOVER, fname));
        ui_reg_all_failed();
        return;
    }

    if (ccm_id == UNUSED_PARAM){
		to_id = 3;
	}
    platform_reg_failover_ind((void *)(long) to_id);
}













void
sip_platform_logout_reset_req(void)
{
	platform_logout_reset_req();
}
																			 












void
sip_platform_set_ccm_status (void)
{
    static const char fname[] = "sip_platform_set_ccm_status";
    ti_config_table_t *ccm_table_entry;
    char dest_addr_str[MAX_IPADDR_STR_LEN];

    CCSIP_DEBUG_STATE(DEB_F_PREFIX"\n", DEB_F_PREFIX_ARGS(SIP_REG, fname));
    ccm_table_entry = CCM_Active_Standby_Table.active_ccm_entry;
    if (ccm_table_entry) {
        sstrncpy(dest_addr_str, ccm_table_entry->ti_common.addr_str,
                 MAX_IPADDR_STR_LEN);
        CCSIP_DEBUG_STATE(DEB_F_PREFIX"addr str1 %s\n", DEB_F_PREFIX_ARGS(SIP_REG, fname), dest_addr_str);

        ui_set_ccm_conn_status(dest_addr_str, CCM_STATUS_ACTIVE);
    }
    ccm_table_entry = CCM_Active_Standby_Table.standby_ccm_entry;
    if (ccm_table_entry) {

        ui_set_ccm_conn_status(ccm_table_entry->ti_common.addr_str,
                               CCM_STATUS_STANDBY);
    }
}












CCM_ID
sip_regmgr_get_ccm_id (ccsipCCB_t *ccb)
{
    ti_config_table_t *ccm_table_ptr = NULL;

    ccm_table_ptr = (ti_config_table_t *) ccb->cc_cfg_table_entry;
    if (ccm_table_ptr) {
        return (ccm_table_ptr->ti_specific.ti_ccm.ccm_id);
    }
    return (UNUSED_PARAM);
}












void
sip_platform_cc_mode_notify (void)
{
    int mode;

    if (CC_Config_Table[0].cc_type == CC_CCM) {
        mode = REG_MODE_CCM;
    } else {
        mode = REG_MODE_NON_CCM;
    }
    platform_cc_mode_notify(mode);
}
















sec_level_t
sip_regmgr_get_sec_level (line_t line)
{
    ti_config_table_t *ccm_table_entry;
    ti_ccm_t *ti_ccm;

    if (CCM_Active_Standby_Table.active_ccm_entry) {
        ccm_table_entry = CCM_Active_Standby_Table.active_ccm_entry;
        ti_ccm = &ccm_table_entry->ti_specific.ti_ccm;
        return ((sec_level_t) ti_ccm->sec_level);
    } else {
        return (NON_SECURE);
    }
}













boolean
sip_regmgr_srtp_fallback_enabled (line_t line)
{
    ccsipCCB_t *ccb;
    line_t      ndx;

    if ((line == 0) || (line > MAX_REG_LINES)) {
        
        return 0;
    }

    
    ndx = line - 1 + REG_CCB_START;
    ccb = sip_sm_get_ccb_by_index(ndx);
    if (ccb != NULL) {
        if (ccb->supported_tags & cisco_srtp_fallback_tag) {
            return (TRUE);
        }
    }
    return (FALSE);
}


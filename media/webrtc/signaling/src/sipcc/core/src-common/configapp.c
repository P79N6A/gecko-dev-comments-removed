



#include "cpr_types.h"
#include "cpr_stdlib.h"
#include "cpr_timers.h"
#include "cpr_strings.h"
#include "ccsip_subsmanager.h"
#include "subapi.h"
#include "debug.h"
#include "phone_debug.h"
#include "phntask.h"

int32_t g_configappDebug = 1;

extern void update_kpmlconfig (int kpmlVal);










void
configapp_init (void)
{
    static const char fname[] = "configapp_init";

    CONFIGAPP_DEBUG(DEB_F_PREFIX"Subscribing to SUB/NOT manager.\n", 
                    DEB_F_PREFIX_ARGS(CONFIG_APP, fname));

    (void) sub_int_subnot_register(CC_SRC_MISC_APP, CC_SRC_SIP,
                                   CC_SUBSCRIPTIONS_CONFIGAPP,
                                   NULL, CC_SRC_MISC_APP,
                                   SUB_MSG_CONFIGAPP_SUBSCRIBE, NULL,
                                   SUB_MSG_CONFIGAPP_TERMINATE, 0, 0);

}











void
configapp_shutdown (void)
{
}











void 
configapp_free_event_data (ccsip_event_data_t *data)
{
    ccsip_event_data_t *next_data;

    while(data) {
        next_data = data->next;
        cpr_free(data);
        data = next_data;
    }
}















void
configapp_process_request (ccsip_sub_not_data_t *msg)
{
    static const char fname[] = "configapp_process_request";
    ConfigApp_req_data_t *configdata;

    configdata = &(msg->u.subs_ind_data.eventData->u.configapp_data);

    update_kpmlconfig(configdata->sip_profile.kpml_val);
    CONFIGAPP_DEBUG(DEB_F_PREFIX"Updated kpml config value to %d.\n", 
                           DEB_F_PREFIX_ARGS(CONFIG_APP, fname), 
                           configdata->sip_profile.kpml_val);
  
    (void)sub_int_subscribe_ack(CC_SRC_MISC_APP, CC_SRC_SIP, msg->sub_id, 
                                (uint16_t)SIP_SUCCESS_SETUP, 0);

    (void)sub_int_subscribe_term(msg->sub_id, TRUE, 0, CC_SUBSCRIPTIONS_CONFIGAPP);
    configapp_free_event_data(msg->u.subs_ind_data.eventData);
    return;
}












void
configapp_process_msg (uint32_t cmd, void *msg)
{
    static const char fname[] = "configapp_process_msg";

    switch (cmd) {
    case SUB_MSG_CONFIGAPP_SUBSCRIBE:
         configapp_process_request((ccsip_sub_not_data_t *)msg);
         break;
    case SUB_MSG_CONFIGAPP_TERMINATE:
         break;
    default:
         CONFIGAPP_DEBUG(DEB_F_PREFIX"Received invalid event.\n",
                         DEB_F_PREFIX_ARGS(CONFIG_APP, fname));
         break;
    }
}



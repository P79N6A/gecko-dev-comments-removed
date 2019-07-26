



#include "publish_int.h"
#include "subapi.h"
#include "ccsip_subsmanager.h"
#include "phntask.h"












static cc_rcs_t pub_int_req (pub_req_t *pub_req_p)
{
    cc_rcs_t ret_code;

    ret_code = app_send_message(pub_req_p, sizeof(pub_req_t), CC_SRC_SIP,  SIPSPI_EV_CC_PUBLISH_REQ);

    if (ret_code != CC_RC_SUCCESS) {
        free_event_data(pub_req_p->event_data_p);
    }
    return ret_code;
}


















void publish_init (pub_handle_t             app_handle,
                   char                    *ruri,
                   char                    *esc,
                   unsigned int             expires,
                   cc_subscriptions_t       event_type,
                   ccsip_event_data_t      *event_data_p,
                   cc_srcs_t                callback_task,
                   int                      message_id
                  )
{

    pub_req_t pub_req;

    


    pub_req.pub_handle = NULL_PUBLISH_HANDLE; 
    pub_req.app_handle = app_handle;
    sstrncpy(pub_req.ruri, ruri, MAX_URI_LENGTH);
    sstrncpy(pub_req.esc, esc, MAX_URI_LENGTH);
    pub_req.expires = expires;
    pub_req.event_type = event_type;
    pub_req.event_data_p = event_data_p;
    pub_req.callback_task = callback_task;
    pub_req.resp_msg_id = message_id;

    (void)pub_int_req(&pub_req);
}














void publish_update (pub_handle_t          pub_handle,
                     cc_subscriptions_t    event_type,
                     ccsip_event_data_t   *event_data_p,
                     cc_srcs_t             callback_task,
                     int                   message_id
                    )
{
    pub_req_t pub_req;

    


    memset(&pub_req, 0, sizeof(pub_req));
    pub_req.pub_handle = pub_handle;
    pub_req.event_type = event_type;
    pub_req.event_data_p = event_data_p;
    pub_req.callback_task = callback_task;
    pub_req.resp_msg_id = message_id;

    (void)pub_int_req(&pub_req);
}













void publish_terminate (pub_handle_t          pub_handle,
                        cc_subscriptions_t    event_type,
                        cc_srcs_t             callback_task,
                        int                   message_id
                       )
{
   pub_req_t pub_req;

    


    memset(&pub_req, 0, sizeof(pub_req));
    pub_req.pub_handle = pub_handle;
    pub_req.event_type = event_type;
    pub_req.callback_task = callback_task;
    pub_req.resp_msg_id = message_id;

    (void)pub_int_req(&pub_req);
}













cc_rcs_t publish_int_response (pub_rsp_t               *pub_rsp_p,
                               cc_srcs_t               callback_task,
                               int                     message_id
                              )
{
    pub_rsp_t *pmsg;

    pmsg = (pub_rsp_t *) cc_get_msg_buf(sizeof(*pmsg));
    if (!pmsg) {
        return CC_RC_ERROR;
    }

    memcpy(pmsg, pub_rsp_p, sizeof(*pmsg));

    return sub_send_msg((cprBuffer_t)pmsg, message_id, sizeof(*pmsg), callback_task);
}


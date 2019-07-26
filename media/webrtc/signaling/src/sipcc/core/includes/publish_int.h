






































#ifndef PUBLISH_INT_H
#define PUBLISH_INT_H

#include "ccsip_subsmanager.h"
#include "cpr_types.h"
#include "ccsip_callinfo.h"


typedef uint32_t pub_handle_t; 
#define NULL_PUBLISH_HANDLE  0 /* this is to indicate that handle is not assigned  yet */


typedef struct  {
    pub_handle_t         pub_handle;  
    pub_handle_t         app_handle;  
    char                 ruri[MAX_URI_LENGTH];  
    char                 esc[MAX_URI_LENGTH];   
    uint32_t             expires;    
    cc_subscriptions_t   event_type; 
    ccsip_event_data_t  *event_data_p;  
    cc_srcs_t            callback_task;  
    int                  resp_msg_id; 
} pub_req_t;

typedef struct {
     unsigned int       resp_code;  
     pub_handle_t       pub_handle; 
     pub_handle_t       app_handle; 
} pub_rsp_t;

extern 
void publish_init(pub_handle_t             app_handle,
                  char                    *ruri,
                  char                    *esc,
                  unsigned int             expires,
                  cc_subscriptions_t       event_type,
                  ccsip_event_data_t      *event_data_p,
                  cc_srcs_t                callback_task,
                  int                      message_id 
                 );

extern
void publish_update(pub_handle_t          pub_handle,
                    cc_subscriptions_t    event_type,
                    ccsip_event_data_t   *event_data_p,
                    cc_srcs_t             callback_task,
                    int                   message_id
                   );

extern
void publish_terminate(pub_handle_t          pub_handle,
                       cc_subscriptions_t    event_type,
                       cc_srcs_t             callback_task,
                       int                   message_id
                      );

extern
cc_rcs_t publish_int_response(pub_rsp_t               *pub_rsp_p,
                              cc_srcs_t                callback_task,
                              int                      message_id
                             );

#endif  




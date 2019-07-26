






































#ifndef PRES_SUB_NOT_HANDLER_H
#define PRES_SUB_NOT_HANDLER_H

#include "cpr_types.h"

extern void pres_get_state(int request_id, int duration, const char *watcher,
                           const char *presentity, int app_id, int feature_mask);
extern void pres_terminate_req(int request_id);
extern void pres_terminate_req_all(void);
extern void pres_process_msg_from_msgq(uint32_t cmd, void *msg_p);
extern cpr_status_e pres_create_retry_after_timers(void);
extern void pres_destroy_retry_after_timers(void);
extern void pres_play_blf_audible_alert(void);
extern void pres_sub_handler_initialized(void);

#endif 

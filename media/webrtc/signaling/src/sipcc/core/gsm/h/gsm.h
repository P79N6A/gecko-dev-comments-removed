



#ifndef _GSM_H_
#define _GSM_H_

#include "cpr_types.h"
#include "cpr_memory.h"
#include "cpr_ipc.h"
#include "cpr_stdio.h"

#define GSM_ERR_MSG err_msg
typedef void(* media_timer_callback_fp) (void);
void gsm_set_media_callback(media_timer_callback_fp* callback);

void gsm_set_initialized(void);
cpr_status_e gsm_send_msg(uint32_t cmd, cprBuffer_t buf, uint16_t len);
cprBuffer_t gsm_get_buffer(uint16_t size);
boolean gsm_is_idle(void);







typedef enum {
    GSM_ERROR_ONHOOK_TIMER,
    GSM_AUTOANSWER_TIMER,
    GSM_DIAL_TIMEOUT_TIMER,
    GSM_KPML_INTER_DIGIT_TIMER,
    GSM_KPML_CRITICAL_DIGIT_TIMER,
    GSM_KPML_EXTRA_DIGIT_TIMER,
    GSM_KPML_SUBSCRIPTION_TIMER,
    GSM_MULTIPART_TONES_TIMER,
    GSM_CONTINUOUS_TONES_TIMER,
    GSM_REQ_PENDING_TIMER,
    GSM_RINGBACK_DELAY_TIMER,
    GSM_REVERSION_TIMER,
    GSM_FLASH_ONCE_TIMER,
    GSM_CAC_FAILURE_TIMER,
    GSM_TONE_DURATION_TIMER
} gsmTimerList_t;







extern cprMsgQueue_t gsm_msgq;

extern void kpml_process_msg(uint32_t cmd, void *msg);
extern void dp_process_msg(uint32_t cmd, void *msg);
extern void kpml_init(void);
extern void kpml_shutdown(void);

#endif

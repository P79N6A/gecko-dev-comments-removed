



#include "cpr_types.h"
#include "cpr_memory.h"
#include "cpr_timers.h"
#include "cpr_locks.h"
#include "phntask.h"
#include "ccsip_subsmanager.h"
#include "singly_link_list.h"
#include "ccapi.h"
#include "subapi.h"
#include "fsm.h"
#include "uiapi.h"
#include "rtp_defs.h"
#include "lsm.h"
#include "lsm_private.h"
#include "kpmlmap.h"
#include "dialplanint.h"
#include "uiapi.h"
#include "debug.h"
#include "phone_debug.h"
#include "kpml_common_util.h"
#include "gsm.h"

cc_int32_t KpmlDebug = 0;
static sll_handle_t s_kpml_list = NULL;
static int s_kpml_config;

static void kpml_generate_notify(kpml_data_t *kpml_data, boolean no_body,
                                 unsigned int resp_code, char *resp_text);
static void kpml_generate_subscribe_response(kpml_data_t *kpml_data,
                                             int resp_code);
static void kpmlmap_show(void);
void kpml_inter_digit_timer_callback(void *kpml_key_p);
void kpml_subscription_timer_callback(void *kpml_key);
static uint32_t g_kpml_id = CC_NO_CALL_ID;
static cprMutex_t kpml_mutex;











boolean
kpml_get_state (void)
{
    config_get_value(CFGID_KPML_ENABLED, &s_kpml_config, sizeof(s_kpml_config));

    if ((s_kpml_config == KPML_NONE) || (s_kpml_config == KPML_DTMF_ONLY)) {
        return FALSE;
    }
    return TRUE;
}











kpml_config_e
kpml_get_config_value (void)
{
    int kpml_config = KPML_NONE;
    config_get_value(CFGID_KPML_ENABLED, &kpml_config, sizeof(kpml_config));

    return (kpml_config_e) kpml_config;
}










static kpml_data_t *
kpml_get_new_data (void)
{
    kpml_data_t *kpml_mem;

    kpml_mem = (kpml_data_t *) cpr_malloc(sizeof(kpml_data_t));
    if (kpml_mem == NULL)
    {
        return (NULL);
    }

    memset(kpml_mem, 0, sizeof(kpml_data_t));
	kpml_mem->kpml_id = ++g_kpml_id;

    return (kpml_mem);
}











static void
kpml_release_data (kpml_data_t * kpml_data)
{
    cpr_free(kpml_data);
}












static void
kpml_create_sm_key (kpml_key_t *key_p, line_t line, callid_t call_id,
                    void *tmr_ptr)
{
    static const char fname[] = "kpml_create_sm_key";

    KPML_DEBUG(DEB_L_C_F_PREFIX" timer=0x%0x\n",
			   DEB_L_C_F_PREFIX_ARGS(KPML_INFO, line, call_id, fname), tmr_ptr);

    key_p->line = line;
    key_p->call_id = call_id;
    key_p->timer = tmr_ptr;
}












static sll_match_e
kpml_match_line_call_id (kpml_data_t * kpml_data_p, kpml_key_t * key_p)
{
    static const char fname[] = "kpml_match_line_call_id";

    if ((kpml_data_p->call_id == key_p->call_id) &&
        (kpml_data_p->line == key_p->line)) {

        KPML_DEBUG(DEB_L_C_F_PREFIX"Match Found.\n",
                   DEB_L_C_F_PREFIX_ARGS(KPML_INFO, key_p->line, key_p->call_id, fname));
        return SLL_MATCH_FOUND;
    }

    return SLL_MATCH_NOT_FOUND;
}











static kpml_data_t *
kpml_data_for_subid(sub_id_t sub_id)
{
    kpml_data_t *kpml_data;

    kpml_data = (kpml_data_t *) sll_next(s_kpml_list, NULL);

    while (kpml_data != NULL) {
        if (kpml_data->sub_id == sub_id) {
            return kpml_data;
        }
        kpml_data = (kpml_data_t *) sll_next(s_kpml_list, kpml_data);
    }
    return NULL;
}
















static void
kpml_start_timer (line_t line, callid_t callId,
                  void *timer_ptr, unsigned int duration,
                  uint32_t kpml_id)
{
    if (timer_ptr) {

        (void) cprCancelTimer(timer_ptr);

        (void) cprStartTimer(timer_ptr, duration, (void *)(long)kpml_id);
    }
}










static void
kpml_stop_timer (void *timer_ptr)
{
    if (timer_ptr) {
        (void) cprCancelTimer(timer_ptr);

        (void) cprDestroyTimer(timer_ptr);
    }
}










static void
kpml_clear_timers (kpml_data_t *kpml_data)
{
    static const char fname[] = "kpml_clear_timers";

    KPML_DEBUG(DEB_L_C_F_PREFIX"Release kpml timers.\n",
                DEB_L_C_F_PREFIX_ARGS(KPML_INFO, kpml_data->line, kpml_data->call_id, fname));

    kpml_stop_timer(kpml_data->inter_digit_timer);
    kpml_data->inter_digit_timer = NULL;

    kpml_stop_timer(kpml_data->critical_timer);
    kpml_data->critical_timer = NULL;

    kpml_stop_timer(kpml_data->extra_digit_timer);
    kpml_data->extra_digit_timer = NULL;

    kpml_stop_timer(kpml_data->sub_timer);
    kpml_data->sub_timer = NULL;
}












static void
kpml_start_timers (kpml_data_t *kpml_data)
{
    static const char *fname ="kpml_start_timers";

    kpml_data->inter_digit_timer =
        cprCreateTimer("Interdigit timer", GSM_KPML_INTER_DIGIT_TIMER,
                       TIMER_EXPIRATION, gsm_msgq);

    kpml_data->critical_timer =
        cprCreateTimer("Criticaldigit timer", GSM_KPML_CRITICAL_DIGIT_TIMER,
                       TIMER_EXPIRATION, gsm_msgq);

    kpml_data->extra_digit_timer =
        cprCreateTimer("Extradigit timer", GSM_KPML_EXTRA_DIGIT_TIMER,
                       TIMER_EXPIRATION, gsm_msgq);

    
    if (kpml_data->inter_digit_timer == NULL ||
        kpml_data->critical_timer == NULL ||
        kpml_data->extra_digit_timer == NULL) {

        
        KPML_ERROR(KPML_F_PREFIX"No memory to allocate timer\n",
                    fname);
        return;
    }

    
    kpml_start_timer(kpml_data->line, kpml_data->call_id,
                     kpml_data->inter_digit_timer,
                     kpml_data->inttimeout, kpml_data->kpml_id);

    
    kpml_start_timer(kpml_data->line, kpml_data->call_id,
                     kpml_data->critical_timer,
                     kpml_data->crittimeout, kpml_data->kpml_id);

    
    kpml_start_timer(kpml_data->line, kpml_data->call_id,
                     kpml_data->extra_digit_timer,
                     kpml_data->extratimeout, kpml_data->kpml_id);

}











static void
kpml_restart_timers (kpml_data_t * kpml_data)
{
    static const char fname[] = "kpml_restart_timers";

    KPML_DEBUG(DEB_L_C_F_PREFIX"Restart all timers\n",
               DEB_L_C_F_PREFIX_ARGS(KPML_INFO, kpml_data->line, kpml_data->call_id, fname));

    kpml_stop_timer(kpml_data->critical_timer);

    kpml_stop_timer(kpml_data->inter_digit_timer);

    kpml_stop_timer(kpml_data->extra_digit_timer);

    kpml_start_timers(kpml_data);

    return;
}

















static boolean
kpml_clear_data (kpml_data_t *kpml_data, kpml_sub_type_e sub_type)
{
    static const char fname[] = "kpml_clear_data";

    KPML_DEBUG(DEB_L_C_F_PREFIX"sub_type=%d",
               DEB_L_C_F_PREFIX_ARGS(KPML_INFO, kpml_data->line, kpml_data->call_id, fname), sub_type);

    switch (sub_type) {

    case KPML_ONE_SHOT:
        KPML_DEBUG(DEB_F_PREFIX"One shot\n", DEB_F_PREFIX_ARGS(KPML_INFO, fname));

        kpml_stop_timer(kpml_data->inter_digit_timer);
        kpml_data->inter_digit_timer = NULL;

        kpml_stop_timer(kpml_data->critical_timer);
        kpml_data->critical_timer = NULL;

        kpml_stop_timer(kpml_data->extra_digit_timer);
        kpml_data->extra_digit_timer = NULL;

        kpml_stop_timer(kpml_data->sub_timer);
        kpml_data->sub_timer = NULL;

        (void) sll_remove(s_kpml_list, kpml_data);

        kpml_release_data(kpml_data);

        kpmlmap_show();
        return (TRUE);

    case KPML_PERSISTENT:
        KPML_DEBUG(DEB_F_PREFIX"Persistent\n", DEB_F_PREFIX_ARGS(KPML_INFO, fname));
        

    case KPML_SINGLY_NOTIFY:
        KPML_DEBUG(DEB_F_PREFIX"Singly notify\n", DEB_F_PREFIX_ARGS(KPML_INFO, fname));

        



        kpml_data->kpmlDialed[0] = 00;

        kpml_restart_timers(kpml_data);
        







        kpmlmap_show();
        return (FALSE);

    default:
        KPML_DEBUG(DEB_F_PREFIX"KPML type not specified\n", DEB_F_PREFIX_ARGS(KPML_INFO, fname));
        return (FALSE);
    }
}


















void
kpml_quarantine_digits (line_t line, callid_t call_id, char digit)
{
    static const char fname[] = "kpml_quarantine_digits";
    kpml_data_t *kpml_data;
    kpml_key_t kpml_key;

    if (kpml_get_config_value() == KPML_NONE) {
        return;
    }

    KPML_DEBUG(DEB_L_C_F_PREFIX"digit=0x%0x\n",
               DEB_L_C_F_PREFIX_ARGS(KPML_INFO, line, call_id, fname), digit);

    kpml_create_sm_key(&kpml_key, line, call_id, NULL);

    kpml_data = (kpml_data_t *) sll_find(s_kpml_list, &kpml_key);

    if (!kpml_data) {

        kpml_data = kpml_get_new_data();

        if (kpml_data == NULL) {
            KPML_ERROR(KPML_F_PREFIX"No memory for subscription data\n",
                    fname);
            return;
        }

        (void) sll_append(s_kpml_list, kpml_data);

        kpml_data->line = line;

        kpml_data->call_id = call_id;

        kpml_data->pending_sub = FALSE;

        kpml_data->dig_head = kpml_data->dig_tail = 0;

    }

    if (kpml_data->dig_head == (kpml_data->dig_tail + 1) % MAX_DIALSTRING) {

        
        kpml_data->dig_head = (kpml_data->dig_head + 1) % MAX_DIALSTRING;
    }

    kpml_data->q_digits[kpml_data->dig_tail] = digit;

    kpml_data->dig_tail = (kpml_data->dig_tail + 1) % MAX_DIALSTRING;
}













void
kpml_flush_quarantine_buffer (line_t line, callid_t call_id)
{
    static const char fname[] = "kpml_flush_quarantine_buffer";
    kpml_data_t *kpml_data;
    kpml_key_t kpml_key;

    if (kpml_get_config_value() == KPML_NONE) {
        return;
    }

    KPML_DEBUG(DEB_L_C_F_PREFIX"Flush buffer\n", DEB_L_C_F_PREFIX_ARGS(KPML_INFO, line, call_id, fname));

    kpml_create_sm_key(&kpml_key, line, call_id, NULL);

    kpml_data = (kpml_data_t *) sll_find(s_kpml_list, &kpml_key);

    if (kpml_data) {

        if (!kpml_data->pending_sub) {

            kpml_data->dig_head = kpml_data->dig_tail = 0;
            (void) kpml_clear_data(kpml_data, KPML_ONE_SHOT);
        }

    }

}











static void
kpml_update_quarantined_digits (kpml_data_t *kpml_data)
{
    static const char fname[] = "kpml_update_quarantined_digits";

    KPML_DEBUG(DEB_L_C_F_PREFIX"Update quarantined digits\n",
               DEB_L_C_F_PREFIX_ARGS(KPML_INFO, kpml_data->line, kpml_data->call_id, fname));

    while (kpml_data->dig_head != kpml_data->dig_tail) {

        kpml_update_dialed_digits(kpml_data->line,
                                          kpml_data->call_id,
                                          kpml_data->q_digits[kpml_data->dig_head]);

        kpml_data->dig_head = (kpml_data->dig_head + 1) % MAX_DIALSTRING;
    }
}













void
kpml_inter_digit_timer_callback (void *kpml_key_p)
{
    (void) app_send_message(&kpml_key_p, sizeof(void *), CC_SRC_GSM,
                            SUB_MSG_KPML_DIGIT_TIMER);
}

kpml_data_t *kpml_get_kpml_data_from_kpml_id(uint32_t kpml_id)
{
    kpml_data_t *kpml_data;

    kpml_data = (kpml_data_t *) sll_next(s_kpml_list, NULL);

    while (kpml_data != NULL && kpml_data->kpml_id != kpml_id) {

        kpml_data = (kpml_data_t *) sll_next(s_kpml_list, kpml_data);
    }
    return(kpml_data);
}













static void
kpml_inter_digit_timer_event (void **kpml_key_p)
{
    const char fname[] = "kpml_inter_digit_timer_callback";
    kpml_data_t *kpml_data;

    KPML_DEBUG("%s: kpml_id=%d \n ",
               fname, (long)*kpml_key_p);


    kpml_data = kpml_get_kpml_data_from_kpml_id((long)*kpml_key_p);

    if (kpml_data == NULL) {
        KPML_ERROR(KPML_F_PREFIX"KPML data not found.\n", fname);
        return;
    }

        KPML_DEBUG(": Interdigit Timer\n");

        kpml_generate_notify(kpml_data, FALSE, KPML_TIMER_EXPIRE,
                             KPML_TIMER_EXPIRE_STR);

        
        (void) kpml_clear_data(kpml_data, kpml_data->persistent);
}










static void *
kpml_start_subscription_timer (kpml_data_t * kpml_data, unsigned long duration)
{
    static const char fname[] = "kpml_start_subscription_timer";

    KPML_DEBUG(DEB_L_C_F_PREFIX"duration=%u\n",
               DEB_L_C_F_PREFIX_ARGS(KPML_INFO, kpml_data->line, kpml_data->call_id, fname), duration);

    kpml_data->sub_timer = cprCreateTimer("sub timer",
                                          GSM_KPML_SUBSCRIPTION_TIMER,
                                          TIMER_EXPIRATION, gsm_msgq);

    kpml_data->sub_duration = duration;

    kpml_create_sm_key(&(kpml_data->subtimer_key), kpml_data->line,
                       kpml_data->call_id, kpml_data->sub_timer);

    
    kpml_start_timer(kpml_data->line,
                     kpml_data->call_id,
                     kpml_data->sub_timer,
                     kpml_data->sub_duration * 1000,
                     kpml_data->kpml_id);

    return (kpml_data->sub_timer);
}












void
kpml_subscription_timer_callback (void *kpml_key_p)
{
    (void) app_send_message(&kpml_key_p, sizeof(void *), CC_SRC_GSM,
                            SUB_MSG_KPML_SUBSCRIBE_TIMER);
}











static void
kpml_subscription_timer_event (void **kpml_key_p)
{
    static const char fname[] = "kpml_subscription_timer_event";
    kpml_data_t *kpml_data;

    KPML_DEBUG("%s: kpml_id=%d \n ",
               fname, (long)*kpml_key_p);

    kpml_data = kpml_get_kpml_data_from_kpml_id((long)*kpml_key_p);

    
    if (kpml_data) {
        kpml_generate_notify(kpml_data, FALSE, KPML_SUB_EXPIRE,
                             KPML_SUB_EXPIRE_STR);

        (void) kpml_clear_data(kpml_data, KPML_ONE_SHOT);
    }
}












static boolean
kpml_match_requested_digit (kpml_regex_match_t *regex_match, char input)
{
    uint32_t *bitmask;
    uint32_t result = 0;

    bitmask = (uint32_t *) &(regex_match->u.single_digit_bitmask);

    switch (input) {
    case '1':
        result = (*bitmask & REGEX_1);
        break;
    case '2':
        result = (*bitmask & REGEX_2);
        break;
    case '3':
        result = (*bitmask & REGEX_3);
        break;
    case '4':
        result = (*bitmask & REGEX_4);
        break;
    case '5':
        result = (*bitmask & REGEX_5);
        break;
    case '6':
        result = (*bitmask & REGEX_6);
        break;
    case '7':
        result = (*bitmask & REGEX_7);
        break;
    case '8':
        result = (*bitmask & REGEX_8);
        break;
    case '9':
        result = (*bitmask & REGEX_9);
        break;
    case '0':
        result = (*bitmask & REGEX_0);
        break;
    case '*':
        result = (*bitmask & REGEX_STAR);
        break;
    case '#':
        result = (*bitmask & REGEX_POUND);
        break;
    case 'A':
        result = (*bitmask & REGEX_A);
        break;
    case 'B':
        result = (*bitmask & REGEX_B);
        break;
    case 'C':
        result = (*bitmask & REGEX_C);
        break;
    case 'D':
        result = (*bitmask & REGEX_D);
        break;
    case '+':
        result = (*bitmask & REGEX_PLUS);
        break;
    default:
        result = 0;
        break;
    }

    if (result) {
        return (TRUE);
    }

    return (FALSE);
}












static kpml_match_action_e
kpml_match_pattern (kpml_data_t *ptempl)
{
    char *pinput;
    int regex_index = 0;

    pinput = &ptempl->kpmlDialed[0];

    if (strchr(pinput, '#') && ptempl->enterkey) {

        return (KPML_IMMEDIATELY);
    }

    if (kpml_match_requested_digit
        (&(ptempl->regex_match[regex_index]), pinput[0])) {
        return (KPML_FULLPATTERN);
    }

    return (KPML_NOMATCH);
}












boolean kpml_is_subscribed (callid_t call_id, line_t line)
{
    static const char fname[] = "kpml_is_subscribed";
    kpml_data_t *kpml_data, *kpml_next_data;

    KPML_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(KPML_INFO, line, call_id, fname));

    kpml_data = (kpml_data_t *) sll_next(s_kpml_list, NULL);
    while (kpml_data) {
        kpml_next_data = (kpml_data_t *) sll_next(s_kpml_list, kpml_data);
        if (kpml_data->pending_sub &&
            kpml_data->line == line &&
            kpml_data->call_id == call_id) {
            return TRUE;
        }
        kpml_data = kpml_next_data;
    }
    return FALSE;
}

















kpml_state_e
kpml_update_dialed_digits (line_t line, callid_t call_id, char digit)
{
    static const char fname[] = "kpml_update_dialed_digits";
    kpml_data_t *kpml_data, *kpml_next_data;
    kpml_match_action_e result = KPML_NOMATCH;
    int dial_len = 0;
    kpml_state_e state = NO_SUB_DATA;


    if (kpml_get_config_value() == KPML_NONE) {
        return (state);
    }

    KPML_DEBUG(DEB_L_C_F_PREFIX"digits=0x%x\n",
               DEB_L_C_F_PREFIX_ARGS(KPML_INFO, line, call_id, fname), digit);

    kpml_data = (kpml_data_t *) sll_next(s_kpml_list, NULL);

    while (kpml_data) {

        kpml_next_data = (kpml_data_t *) sll_next(s_kpml_list, kpml_data);

        if (kpml_data->pending_sub &&
            kpml_data->line == line &&
            kpml_data->call_id == call_id) {

            state = SUB_DATA_FOUND;

            
            dial_len = strlen(kpml_data->kpmlDialed);
            if (dial_len >= MAX_DIALSTRING-1)
            {  
                KPML_ERROR(DEB_L_C_F_PREFIX"dial_len = [%d] too large\n", DEB_L_C_F_PREFIX_ARGS(KPML_INFO, line, call_id, fname), dial_len);
                return (state);
            }

            
            
            if (digit == (char)0x82) {
                kpml_generate_notify(kpml_data, FALSE, KPML_TIMER_EXPIRE,
                             KPML_TIMER_EXPIRE_STR);
                memset(&kpml_data->kpmlDialed[0], 0, MAX_DIALSTRING);
                (void) kpml_clear_data(kpml_data, kpml_data->persistent);
                state = NOTIFY_SENT;
                kpml_data = kpml_next_data;
                continue;
            }

            if (digit == 0x0F) {

                kpml_data->kpmlDialed[dial_len] = '#';

            } else if (digit == 0x0E) {

                kpml_data->kpmlDialed[dial_len] = '*';

            } else {

                kpml_data->kpmlDialed[dial_len] = digit;
            }

            kpml_data->kpmlDialed[dial_len + 1] = 00;

            if (digit == BKSPACE_KEY) {

                kpml_data->last_dig_bkspace = TRUE;

                sstrncpy(kpml_data->kpmlDialed, "bs", sizeof(kpml_data->kpmlDialed));

                result = KPML_FULLPATTERN;

            } else {

                result = kpml_match_pattern(kpml_data);
            }

            switch (result) {

            case KPML_FULLPATTERN:

                kpml_generate_notify(kpml_data, FALSE, KPML_SUCCESS,
                                     KPML_SUCCESS_STR);

                dp_store_digits(line, call_id,
                                (unsigned char)((digit == (char) BKSPACE_KEY) ?
                                                 BKSPACE_KEY : kpml_data->
                                                 kpmlDialed[dial_len]));
                
                memset(&kpml_data->kpmlDialed[0], 0, MAX_DIALSTRING);

                (void) kpml_clear_data(kpml_data, kpml_data->persistent);

                state = NOTIFY_SENT;
                break;

            case KPML_IMMEDIATELY:
                
                kpml_data->kpmlDialed[dial_len] = 00;

                kpml_generate_notify(kpml_data, FALSE, KPML_USER_TERM_NOMATCH,
                                     KPML_USER_TERM_NOMATCH_STR);

                dp_store_digits(line, call_id,
                                (unsigned char)((digit == (char) BKSPACE_KEY) ?
                                                 BKSPACE_KEY : kpml_data->
                                                 kpmlDialed[dial_len]));

                memset(&kpml_data->kpmlDialed[0], 0, MAX_DIALSTRING);

                
                (void) kpml_clear_data(kpml_data, kpml_data->persistent);

                state = NOTIFY_SENT;
                break;

            default:
                memset(&kpml_data->kpmlDialed[0], 0, MAX_DIALSTRING);
                
                kpml_restart_timers(kpml_data);
                break;
            }
        }

        


        kpml_data = kpml_next_data;
    }

    return (state);
}














void
kpml_set_subscription_reject (line_t line, callid_t call_id)
{
    static const char fname[] = "kpml_set_subscription_reject";
    kpml_data_t *kpml_data;
    kpml_key_t kpml_key;

    if (kpml_get_config_value() == KPML_NONE) {
        return;
    }

    KPML_DEBUG(DEB_L_C_F_PREFIX"Reject\n", DEB_L_C_F_PREFIX_ARGS(KPML_INFO, line, call_id, fname));

    kpml_create_sm_key(&kpml_key, line, call_id, NULL);

    kpml_data = (kpml_data_t *) sll_find(s_kpml_list, &kpml_key);

    if (kpml_data == NULL) {

        kpml_data = kpml_get_new_data();

        if (kpml_data == NULL) {
            KPML_ERROR(KPML_F_PREFIX"No memory for subscription data\n",
                        fname);
            return;
        }

        (void) sll_append(s_kpml_list, kpml_data);

        kpml_data->line = line;

        kpml_data->call_id = call_id;

        kpml_data->pending_sub = FALSE;

        kpml_data->dig_head = kpml_data->dig_tail = 0;
    }

    kpml_data->sub_reject = TRUE;
}













static kpml_resp_code_e
check_subcription_create_error (kpml_data_t *kpml_data)
{
    static const char fname[] = "check_subcription_create_error";
    lsm_states_t lsm_state;

    lsm_state = lsm_get_state(kpml_data->call_id);

    if (lsm_state == LSM_S_NONE) {

        KPML_ERROR(KPML_L_C_F_PREFIX"NO call with id\n",
                    kpml_data->line, kpml_data->call_id, fname);
        return (KPML_BAD_EVENT);
    }

    if ((lsm_state < LSM_S_CONNECTED) && kpml_data->sub_reject) {

        KPML_ERROR(KPML_L_C_F_PREFIX"Call not in connected state\n",
                    kpml_data->line, kpml_data->call_id, fname);
        return (KPML_BAD_EVENT);
    }

    
    kpml_data->sub_reject = FALSE;

    return (KPML_SUCCESS);
}











static kpml_resp_code_e
check_if_kpml_attributes_supported (KPMLRequest *kpml_sub_data)
{

    if (kpml_sub_data == NULL) {

        return (KPML_BAD_DOC);
    }

    


    if ((kpml_sub_data->pattern.longhold != 0) ||
        (kpml_sub_data->pattern.longrepeat != 0) ||
        (kpml_sub_data->pattern.nopartial != 0)) {
        return (KPML_BAD_DOC);
    }

    return (KPML_SUCCESS);
}










static kpml_resp_code_e
check_attributes_range (KPMLRequest * kpml_data)
{
    return (KPML_SUCCESS);
}










static kpml_resp_code_e
check_kpml_config (line_t line, callid_t call_id)
{
    static const char fname[] = "check_kpml_config";
    lsm_states_t lsm_state;

    lsm_state = lsm_get_state(call_id);

    if (lsm_state == LSM_S_NONE) {
        KPML_ERROR(KPML_L_C_F_PREFIX"NO call\n",
                    line, call_id, fname);
        return (KPML_BAD_EVENT);
    }

    
    config_get_value(CFGID_KPML_ENABLED, &s_kpml_config, sizeof(s_kpml_config));

    switch (lsm_state) {
    case LSM_S_OFFHOOK:
    case LSM_S_PROCEED:

        if ((s_kpml_config == KPML_SIGNAL_ONLY) || (s_kpml_config == KPML_BOTH)) {
            return (KPML_SUCCESS);
        }

        break;

    case LSM_S_RINGOUT:
    case LSM_S_CONNECTED:
    case LSM_S_HOLDING:

        if ((s_kpml_config == KPML_DTMF_ONLY) || (s_kpml_config == KPML_BOTH)) {
            return (KPML_SUCCESS);
        }

        break;
    default:

        break;
    }

    KPML_ERROR(KPML_L_C_F_PREFIX"KPML disabled - Check your conifg 0- None, \
            1-signaling, 2-dtmf 3-both\n", line, call_id, fname);
    return (KPML_BAD_EVENT);
}












static kpml_resp_code_e
kpml_treat_enterkey (kpml_data_t *kpml_data, char *enter_str)
{
    

    if (enter_str[0] == NUL) {

        kpml_data->enterkey = FALSE;

    } else if (!strcmp(enter_str, KPML_ENTER_STR)) {

        kpml_data->enterkey = TRUE;

    } else {

        return (KPML_BAD_DOC);

    }

    return (KPML_SUCCESS);
}














static kpml_resp_code_e
kpml_treat_regex (kpml_data_t *kpml_data)
{
    static const char fname[] = "kpml_treat_regex";
    short indx = 0, char_inx, i, regex_idx = 0;
    char regex_temp[32];

    kpml_data->enable_backspace = FALSE;

    KPML_DEBUG(DEB_L_C_F_PREFIX"regex=%u\n",
               DEB_L_C_F_PREFIX_ARGS(KPML_INFO, kpml_data->line, kpml_data->call_id, fname),
               kpml_data->regex[indx].regexData);

    
    while (indx < NUM_OF_REGX) {
        char_inx = 0;
        i = 0;
        regex_idx = 0;

        while (kpml_data->regex[indx].regexData[char_inx]) {
            switch (kpml_data->regex[indx].regexData[char_inx]) {
            case ' ':
                break;
            case '|':
                break;
            case 'b':
                if (kpml_data->regex[indx].regexData[char_inx + 1] == 's') {
                    char_inx++;
                    kpml_data->enable_backspace = TRUE;
                } else {
                    return (KPML_BAD_DOC);
                }

                break;
            case 'x':
            default:
                regex_temp[i++] = kpml_data->regex[indx].regexData[char_inx];
                break;
            }
            char_inx++;
        }

        regex_temp[i] = NUL;

        
        if (kpml_parse_regex_str(&(regex_temp[0]),
                                 &(kpml_data->regex_match[indx])) !=
            KPML_STATUS_OK) {
            KPML_ERROR(KPML_F_PREFIX"Regex parse error.\n",fname);
            return (KPML_BAD_DOC);
        }

        
        while (regex_idx < kpml_data->regex_match[indx].num_digits) {
            kpml_data->regex[indx].regexData[regex_idx++] = 'x';
        }

        kpml_data->regex[indx].regexData[regex_idx] = NUL;


        
        indx++;
    }
    return (KPML_SUCCESS);
}










static kpml_data_t *
kpml_update_data (kpml_data_t *kpml_data, KPMLRequest *kpml_sub_data)
{
    static const char fname[] = "kpml_update_data";

    if ((kpml_sub_data == NULL) || (kpml_data == NULL)) {
        return (kpml_data);
    }

    memcpy((char *) &(kpml_data->regex),
           (char *) &(kpml_sub_data->pattern.regex),
           sizeof(Regex) * NUM_OF_REGX);

    kpml_data->persistent = kpml_sub_data->pattern.persist;

    kpml_data->inttimeout = kpml_sub_data->pattern.interdigittimer;
    kpml_data->crittimeout = kpml_sub_data->pattern.criticaldigittimer;
    kpml_data->extratimeout = kpml_sub_data->pattern.extradigittimer;

    kpml_data->flush = kpml_sub_data->pattern.flush;

    
    if (kpml_sub_data->pattern.flush) {

        kpml_data->kpmlDialed[0] = 00;
    }

    kpml_data->longhold = kpml_sub_data->pattern.longhold;

    kpml_data->longrepeat = kpml_sub_data->pattern.longrepeat;

    kpml_data->nopartial = kpml_sub_data->pattern.nopartial;

    KPML_DEBUG(DEB_L_C_F_PREFIX"regex=%u"
               "persistent=%d int-timer=%u critic-timer=%u, extra-timer=%u"
               "flush=%d longhold=%d longrepeat=%d nopartial=%d\n",
               DEB_L_C_F_PREFIX_ARGS(KPML_INFO, kpml_data->line, kpml_data->call_id, fname),
			   kpml_data->regex, kpml_data->persistent, kpml_data->inttimeout,
               kpml_data->crittimeout, kpml_data->extratimeout,
               kpml_data->flush, kpml_data->longhold,
               kpml_data->longrepeat, kpml_data->nopartial);

    return (kpml_data);
}











static void
kpml_terminate_subscription (ccsip_sub_not_data_t *msg)
{
    static const char fname[] = "kpml_terminate_subscribe";
    kpml_data_t *kpml_data = NULL;
    boolean     normal_terminate;
    lsm_lcb_t *lcb;

    KPML_DEBUG(DEB_F_PREFIX"entered.\n", DEB_F_PREFIX_ARGS(KPML_INFO, fname));

    if (kpml_get_config_value() == KPML_NONE) {
        return;
    }

    if (msg->sub_id == (unsigned int)-1) {
        KPML_ERROR(KPML_L_C_F_PREFIX"Invalid sub_id=%d\n", msg->line_id,
                   msg->gsm_id, fname, msg->sub_id);
        return;
    }

    KPML_DEBUG(DEB_L_C_F_PREFIX"sub_id=%d, reason=%d\n",
                DEB_L_C_F_PREFIX_ARGS(KPML_INFO, msg->line_id, msg->gsm_id, fname),
                msg->sub_id, msg->reason_code);
    



    switch (msg->reason_code) {
    case SM_REASON_CODE_SHUTDOWN:
    case SM_REASON_CODE_ROLLOVER:
    case SM_REASON_CODE_RESET_REG:
        





        normal_terminate = FALSE;
        break;
    default:
        normal_terminate = TRUE;
        break;
    }

    
    cprGetMutex(kpml_mutex);

    kpml_data = kpml_data_for_subid(msg->sub_id);

    if (kpml_data) {

        kpml_data->persistent = KPML_ONE_SHOT;

        



        if (normal_terminate) {
            kpml_generate_notify(kpml_data, FALSE,
                                 KPML_SUB_EXPIRE,
                                 KPML_SUB_EXPIRE_STR);

            
            lcb = lsm_get_lcb_by_call_id(kpml_data->call_id);
            if (lcb && lcb->state < LSM_S_RINGOUT) {
                cc_release(CC_SRC_GSM, kpml_data->call_id, kpml_data->line,
                           CC_CAUSE_CONGESTION, NULL, NULL);
            }
        }

        (void) kpml_clear_data(kpml_data, KPML_ONE_SHOT);
    }

    
    cprReleaseMutex(kpml_mutex);

    



    if (normal_terminate) {
        (void) sub_int_subscribe_term(msg->sub_id, TRUE,
                                      msg->request_id, msg->event);
    }
    KPML_DEBUG(DEB_F_PREFIX"exit.\n", DEB_F_PREFIX_ARGS(KPML_INFO, fname));

}












static void
kpml_receive_subscribe (ccsip_sub_not_data_t *msg)
{
    static const char fname[] = "kpml_receive_subscribe";
    kpml_data_t *kpml_data;
    kpml_key_t kpml_key;
    kpml_resp_code_e resp_code = KPML_SUCCESS;
    KPMLRequest *kpml_sub_data = NULL;
    lsm_states_t lsm_state;
    char      *regx_prnt = NULL;
    boolean is_empty_resubscribe = FALSE;

    if (kpml_get_config_value() == KPML_NONE) {
        KPML_DEBUG(DEB_L_C_F_PREFIX"KPML disabled in config.\n",
                   DEB_L_C_F_PREFIX_ARGS(KPML_INFO, msg->line_id, msg->gsm_id, fname));
        return;
    }

    if (msg->line_id == 0 || msg->gsm_id == 0) {

        KPML_ERROR(KPML_L_C_F_PREFIX"Line or call_id not correct\n",
                    msg->line_id, msg->gsm_id, fname);
        (void) sub_int_subscribe_ack(CC_SRC_GSM, CC_SRC_SIP, msg->sub_id,
                                     KPML_BAD_EVENT, msg->sub_duration);
        return;
    }

    kpml_create_sm_key(&kpml_key, (line_t) msg->line_id, (callid_t) msg->gsm_id,
                       NULL);

    kpml_data = (kpml_data_t *) sll_find(s_kpml_list, &kpml_key);

    if (msg->u.subs_ind_data.eventData) {

        kpml_sub_data = &(msg->u.subs_ind_data.eventData->u.kpml_request);
    }


    



    if (kpml_data) {
        if (kpml_data->pending_sub == TRUE) {

            kpml_data->sub_duration = msg->sub_duration;
            
            kpml_generate_subscribe_response(kpml_data, KPML_SUCCESS);

            



            if (kpml_data->sub_id != msg->sub_id) {

                KPML_DEBUG(DEB_L_C_F_PREFIX"Terminate previous subscription \
                           sub_id = %x\n",
						   DEB_L_C_F_PREFIX_ARGS(KPML_INFO, kpml_data->line, kpml_data->call_id, fname),
						   kpml_data->sub_id);

                kpml_generate_notify(kpml_data, FALSE,
                                     KPML_SUB_EXPIRE,
                                     KPML_SUB_EXPIRE_STR);

                (void) sub_int_subscribe_term(kpml_data->sub_id, TRUE,
                                              msg->request_id, msg->event);
            }

            KPML_DEBUG(DEB_L_C_F_PREFIX"Refresh Subscription\n",
                       DEB_L_C_F_PREFIX_ARGS(KPML_INFO, kpml_data->line, kpml_data->call_id, fname));
            




            if (kpml_sub_data == NULL) {
                 kpml_clear_timers(kpml_data);
                 is_empty_resubscribe = TRUE;
            } else if (kpml_clear_data(kpml_data, KPML_ONE_SHOT)) {
                kpml_data = NULL;
            }
        } else {

            



            kpml_data = kpml_update_data(kpml_data, kpml_sub_data);

            KPML_DEBUG(DEB_L_C_F_PREFIX"Activate Subscription\n",
                       DEB_L_C_F_PREFIX_ARGS(KPML_INFO, kpml_data->line, kpml_data->call_id, fname));
        }

    }

    if (kpml_sub_data) {
        regx_prnt = kpml_sub_data->pattern.regex.regexData;
    }

    DEF_DEBUG(DEB_L_C_F_PREFIX"Regex=%s\n",
            DEB_L_C_F_PREFIX_ARGS(KPML_INFO, msg->line_id, msg->gsm_id, fname), regx_prnt);

    
    if (!kpml_data) {

        
        kpml_data = kpml_get_new_data();

        if (kpml_data == NULL) {
            KPML_ERROR(KPML_L_C_F_PREFIX"No memory for subscription data\n",
                    msg->line_id, msg->gsm_id, fname);
            return;
        }

        (void) kpml_update_data(kpml_data, kpml_sub_data);

        (void) sll_append(s_kpml_list, kpml_data);

    }

    kpml_data->call_id = msg->gsm_id;

    kpml_data->line = msg->line_id;

    kpml_data->sub_id = msg->sub_id;

    
    if (msg->sub_duration == 0) {

        
        kpml_data = kpml_update_data(kpml_data, kpml_sub_data);

        KPML_DEBUG(DEB_L_C_F_PREFIX"Terminate Subscription.\n",
                   DEB_L_C_F_PREFIX_ARGS(KPML_INFO, kpml_data->line, kpml_data->call_id, fname));

        
        (void) kpml_treat_regex(kpml_data);

        
        ui_control_featurekey_bksp(msg->line_id, msg->gsm_id,
                                   kpml_data->last_dig_bkspace);

        
        cc_proceeding(CC_SRC_SIP, kpml_data->call_id, kpml_data->line, NULL);

        kpml_data->persistent = KPML_ONE_SHOT;

        kpml_generate_notify(kpml_data, FALSE,
                             KPML_SUB_EXPIRE,
                             KPML_SUB_EXPIRE_STR);
        
        kpml_data->dig_head = kpml_data->dig_tail = 0;

        (void) kpml_clear_data(kpml_data, KPML_ONE_SHOT);

        
        if (msg->u.subs_ind_data.eventData) {
            cpr_free(msg->u.subs_ind_data.eventData);
        }

        return;

    }

    kpml_data->pending_sub = TRUE;

    (void) kpml_start_subscription_timer(kpml_data, msg->sub_duration);

    kpml_start_timers(kpml_data);

    


    if (((resp_code = check_kpml_config(msg->line_id, msg->gsm_id)) != KPML_SUCCESS) ||
        ((resp_code = check_subcription_create_error(kpml_data)) != KPML_SUCCESS)) {

        kpml_generate_subscribe_response(kpml_data, resp_code);

        if (kpml_clear_data(kpml_data, KPML_ONE_SHOT)) {
            kpml_data = NULL;
        }

        if (msg->u.subs_ind_data.eventData) {
            cpr_free(msg->u.subs_ind_data.eventData);
        }
        return;

    } else {
        
        kpml_generate_subscribe_response(kpml_data, SIP_SUCCESS_SETUP);
    }

    



    if (!is_empty_resubscribe && ((kpml_sub_data == NULL) ||
        ((resp_code = check_if_kpml_attributes_supported(kpml_sub_data)) != KPML_SUCCESS) ||
        ((resp_code = check_attributes_range(kpml_sub_data)) != KPML_SUCCESS) ||
        ((resp_code = kpml_treat_regex(kpml_data)) != KPML_SUCCESS) ||
        ((resp_code = kpml_treat_enterkey(kpml_data,
                                          kpml_sub_data->pattern.enterkey)) != KPML_SUCCESS))) {


        KPML_ERROR(KPML_F_PREFIX"Error Resp code = %d\n", fname, resp_code);

        kpml_generate_notify(kpml_data, FALSE, resp_code,
                             KPML_ATTR_NOT_SUPPORTED_STR);

        if (kpml_clear_data(kpml_data, KPML_ONE_SHOT)) {
            kpml_data = NULL;
        }

        cpr_free(msg->u.subs_ind_data.eventData);

        return;
    } else {

        lsm_state = lsm_get_state(kpml_data->call_id);

        





        if ((lsm_state != LSM_S_NONE) && (lsm_state < LSM_S_RINGOUT)) {

            cc_feature(CC_SRC_GSM, kpml_data->call_id, kpml_data->line,
                       CC_FEATURE_SUBSCRIBE, NULL);
        }

        kpml_generate_notify(kpml_data, TRUE, KPML_SUCCESS, KPML_TRYING_STR);

        kpml_update_quarantined_digits(kpml_data);
    }

    
    ui_control_featurekey_bksp(msg->line_id, msg->gsm_id,
                               kpml_data->enable_backspace);

    
    if (msg->u.subs_ind_data.eventData) {
    cpr_free(msg->u.subs_ind_data.eventData);
    }
}











static void
kpml_generate_subscribe_response (kpml_data_t * kpml_data, int resp_code)
{
    static const char fname[] = "kpml_generate_subscribe_response";

    KPML_DEBUG(DEB_L_C_F_PREFIX"SUB response\n",
		       DEB_L_C_F_PREFIX_ARGS(KPML_INFO, kpml_data->line, kpml_data->call_id, fname));

    (void) sub_int_subscribe_ack(CC_SRC_GSM, CC_SRC_SIP, kpml_data->sub_id,
                                 (uint16_t) resp_code, kpml_data->sub_duration);
}











void
kpml_receive_notify_response (ccsip_sub_not_data_t *msg)
{
    static const char fname[] = "kpml_receive_notify_response";
    kpml_data_t *kpml_data;
    kpml_key_t kpml_key;

    KPML_DEBUG(DEB_L_C_F_PREFIX"Notify response\n",
               DEB_L_C_F_PREFIX_ARGS(KPML_INFO, msg->line_id, msg->gsm_id, fname));

    kpml_create_sm_key(&kpml_key, (line_t) msg->line_id, (callid_t) msg->gsm_id,
                       NULL);

    kpml_data = (kpml_data_t *) sll_find(s_kpml_list, &kpml_key);


    



    if (kpml_data) {
        if (kpml_data->last_dig_bkspace &&
            msg->u.notify_result_data.status_code == SIP_SUCCESS_SETUP) {
            
            dp_delete_last_digit(msg->line_id, msg->gsm_id);
            kpml_data->last_dig_bkspace = FALSE;
        } else if (msg->u.notify_result_data.status_code == REQUEST_TIMEOUT) {

            (void) kpml_clear_data(kpml_data, KPML_ONE_SHOT);
            (void) sub_int_subscribe_term(msg->sub_id, TRUE, msg->request_id,
                                          msg->event);
            return;
        }

        


        kpml_update_quarantined_digits(kpml_data);
    } else {
        (void) sub_int_subscribe_term(msg->sub_id, TRUE, msg->request_id,
                                      msg->event);
    }
}












static void
kpml_generate_notify (kpml_data_t *kpml_data, boolean no_body,
                      unsigned int resp_code, char *resp_text)
{
    static const char fname[] = "kpml_generate_notify";
    char resp_str[10];
    ccsip_event_data_t *peventData = NULL;

    DEF_DEBUG(DEB_L_C_F_PREFIX"RESP %u: \n",
        DEB_L_C_F_PREFIX_ARGS(KPML_INFO, kpml_data->line, kpml_data->call_id, fname), resp_code);

    if (no_body == FALSE) {
        
        peventData = (ccsip_event_data_t *)
            cpr_malloc(sizeof(ccsip_event_data_t));

        if (peventData == NULL) {
            KPML_ERROR(KPML_L_C_F_PREFIX"No memory for eventdata\n",
                kpml_data->line, kpml_data->call_id, fname);
            return;
        }

        memset(peventData, 0, sizeof(ccsip_event_data_t));

        sstrncpy(peventData->u.kpml_response.version, KPML_VER_STR, sizeof(peventData->u.kpml_response.version));

        snprintf(resp_str, 10, "%d", resp_code);
        sstrncpy(peventData->u.kpml_response.code, resp_str, sizeof(peventData->u.kpml_response.code));

        if (resp_code == KPML_SUCCESS) {

            sstrncpy(&(peventData->u.kpml_response.digits[0]),
                    &(kpml_data->kpmlDialed[0]), sizeof(peventData->u.kpml_response.digits));
        }

        if (kpml_data->flush == FALSE) {
            sstrncpy(peventData->u.kpml_response.forced_flush, "false",
                    sizeof(peventData->u.kpml_response.forced_flush));
        } else {
            sstrncpy(peventData->u.kpml_response.forced_flush, "true",
                    sizeof(peventData->u.kpml_response.forced_flush));
        }

        sstrncpy(peventData->u.kpml_response.tag,
                &(kpml_data->regex->tag[0]), sizeof(peventData->u.kpml_response.tag));

        sstrncpy(peventData->u.kpml_response.text,
                resp_text, sizeof(peventData->u.kpml_response.text));

        peventData->type = EVENT_DATA_KPML_RESPONSE;
        peventData->next = NULL;
    }

    (void) sub_int_notify(CC_SRC_GSM, CC_SRC_SIP, kpml_data->sub_id,
                           NULL,
                          SUB_MSG_KPML_NOTIFY_ACK, peventData,
                          (kpml_data->persistent ==
                           KPML_ONE_SHOT ? SUBSCRIPTION_TERMINATE :
                           SUBSCRIPTION_NULL));
}












char *
kpml_get_msg_string (uint32_t cmd)
{
    switch (cmd) {

    case SUB_MSG_KPML_SUBSCRIBE:
        return("KPML_SUB");
    case SUB_MSG_KPML_TERMINATE:
        return("KPML_TERMINATE");
    case SUB_MSG_KPML_NOTIFY_ACK:
        return("KPML_NOT_ACK");
    case SUB_MSG_KPML_SUBSCRIBE_TIMER:
        return("KPML_SUB_TIMER");
    case SUB_MSG_KPML_DIGIT_TIMER:
        return("KPML_DIGIT_TIMER");
    default:
        return ("KPML_UNKNOWN_CMD");
    }
}

void
kpml_process_msg (uint32_t cmd, void *msg)
{
    static const char fname[] = "kpml_process_msg";

    KPML_DEBUG(DEB_F_PREFIX"cmd= %s\n", DEB_F_PREFIX_ARGS(KPML_INFO, fname), kpml_get_msg_string(cmd));

    if (s_kpml_list == NULL) {
        
        KPML_DEBUG(DEB_F_PREFIX"KPML is down.\n", DEB_F_PREFIX_ARGS(KPML_INFO, fname));
        return;
    }

    switch (cmd) {

    case SUB_MSG_KPML_SUBSCRIBE:
        kpml_receive_subscribe((ccsip_sub_not_data_t *) msg);
        break;

    case SUB_MSG_KPML_TERMINATE:
        kpml_terminate_subscription((ccsip_sub_not_data_t *) msg);
        break;

    case SUB_MSG_KPML_NOTIFY_ACK:
        kpml_receive_notify_response((ccsip_sub_not_data_t *) msg);
        break;

    case SUB_MSG_KPML_SUBSCRIBE_TIMER:
        kpml_subscription_timer_event((void **) msg);
        break;

    case SUB_MSG_KPML_DIGIT_TIMER:
        kpml_inter_digit_timer_event((void **) msg);
        break;

    default:
        KPML_ERROR(KPML_F_PREFIX"Bad Cmd received: 0x%x.\n", fname, cmd);
        break;
    }
}











static void
kpmlmap_show (void)
{
    static const char *fname="kpmlmap_show";
    kpml_data_t *kpml_data;
    int counter;

    kpml_data = (kpml_data_t *) sll_next(s_kpml_list, NULL);

    while (kpml_data != NULL) {


        KPML_DEBUG(DEB_L_C_F_PREFIX"Pending sub duration=%-8d",
                   DEB_L_C_F_PREFIX_ARGS(KPML_INFO, kpml_data->line, kpml_data->call_id, fname),
                   kpml_data->sub_duration);

        for (counter = 0; counter < NUM_OF_REGX; counter++) {
            KPML_DEBUG(DEB_F_PREFIX"%-4s  %-10s  %-5s\n", DEB_F_PREFIX_ARGS(KPML_INFO, fname),
                       kpml_data->regex[counter].regexData,
                       kpml_data->regex->tag, kpml_data->kpmlDialed);
        }

        kpml_data = (kpml_data_t *) sll_next(s_kpml_list, kpml_data);
    }
}












cc_int32_t
show_kpmlmap_cmd (cc_int32_t argc, const char *argv[])
{
    kpml_data_t *kpml_data;
    int counter;

    debugif_printf("Pending KPML requests are....\n");

    kpml_data = (kpml_data_t *) sll_next(s_kpml_list, NULL);

    debugif_printf("\n--------------- KPML SUBSCRIPTIONS-------------------");
    debugif_printf("\nLine    Call_Id    Expire   Regx    Tag       Digits ");
    debugif_printf
        ("\n------------------------------------------------------\n");

    while (kpml_data != NULL) {

        debugif_printf("%-4d %-5d  %-8lu ",
                       kpml_data->line, kpml_data->call_id,
                       kpml_data->sub_duration);

        for (counter = 0; counter < NUM_OF_REGX; counter++) {
            debugif_printf("%-4s  %-10s  %-5s\n",
                           kpml_data->regex[counter].regexData,
                           kpml_data->regex->tag, kpml_data->kpmlDialed);
        }

        kpml_data = (kpml_data_t *) sll_next(s_kpml_list, kpml_data);
    }
    return (0);
}












void
kpml_init (void)
{
    KPML_DEBUG(DEB_F_PREFIX"entered.\n", DEB_F_PREFIX_ARGS(KPML_INFO, "kpml_init"));

    if (!kpml_mutex) {
        kpml_mutex = cprCreateMutex("kpml lock");
        if (!kpml_mutex) {
            KPML_ERROR(DEB_F_PREFIX"unable to create kpml lock \n", "kpml_init");
        }
    }

    (void) sub_int_subnot_register(CC_SRC_GSM, CC_SRC_SIP,
                                   CC_SUBSCRIPTIONS_KPML,
                                    NULL, CC_SRC_GSM,
                                   SUB_MSG_KPML_SUBSCRIBE,
                                   NULL, SUB_MSG_KPML_TERMINATE, 0, 0);

    
    s_kpml_list = sll_create((sll_match_e(*)(void *, void *))
                             kpml_match_line_call_id);

}



















void
kpml_shutdown (void)
{
    kpml_data_t *kpml_data;
    KPML_DEBUG(DEB_F_PREFIX"entered.\n", DEB_F_PREFIX_ARGS(KPML_INFO, "kpml_shutdown"));

    
    (void) cprGetMutex(kpml_mutex);


    kpml_data = (kpml_data_t *) sll_next(s_kpml_list, NULL);

    while (kpml_data != NULL) {

        


        (void) kpml_clear_data(kpml_data, KPML_ONE_SHOT);

        



        kpml_data = (kpml_data_t *) sll_next(s_kpml_list, NULL);
    }

    sll_destroy(s_kpml_list);

    s_kpml_list = NULL;
    (void) cprReleaseMutex(kpml_mutex);

    KPML_DEBUG(DEB_F_PREFIX"exit.\n", DEB_F_PREFIX_ARGS(KPML_INFO, "kpml_shutdown"));
}

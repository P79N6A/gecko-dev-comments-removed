



#include "cpr.h"
#include "cpr_in.h"
#include "cpr_stdlib.h"
#include "cpr_ipc.h"
#include "phntask.h"
#include <stdarg.h>
#include "configmgr.h"
#include "debug.h"
#include "config.h"
#include "vcm.h"
#include "dialplan.h"
#include "debug.h"
#include "phone_debug.h"
#include "CCProvider.h"
#include "ccsip_task.h"
#include "gsm.h"
#include "misc_apps_task.h"
#include "plat_api.h"
#include "ccapp_task.h"
#include "thread_monitor.h"
#include "mozilla/Assertions.h"

#include "phone_platform_constants.h"



#define BLK_SZ  1024



#define MEM_BASE_BLK 500 //500 blocks, ~ 0.5M
#define MEM_MISC_BLK 500 //500 blocks, ~ 0.5M




#define MEM_PER_CALL_BLK 20 //20 block, ~20k
#define PRIVATE_SYS_MEM_SIZE ((MEM_BASE_BLK + MEM_MISC_BLK + (MEM_PER_CALL_BLK) * MAX_CALLS) * BLK_SZ)


const boolean gHardCodeSDPMode = TRUE;
boolean gStopTickTask = FALSE;






#define  GSMSTKSZ   61440

































#define  GSMQSZ        (MAX_CALLS*3) /* GSM message queue size           */
#define  SIPQSZ        (MAX_CALLS*2) /* SIP message queue size           */
#define  DEFQSZ         0            /* default message queue size       */
#define  DEFAPPQSZ     MAX_REG_LINES






cprMsgQueue_t ccapp_msgq;
cprThread_t ccapp_thread;

cprMsgQueue_t sip_msgq;
cprThread_t sip_thread;
#ifdef NO_SOCKET_POLLING
cprThread_t sip_msgqwait_thread;
#endif

cprMsgQueue_t gsm_msgq;
cprThread_t gsm_thread;

cprMsgQueue_t misc_app_msgq;
cprThread_t misc_app_thread;

#ifdef JINDO_DEBUG_SUPPORTED
cprMsgQueue_t debug_msgq;
cprThread_t debug_thread;
#endif

#ifdef EXTERNAL_TICK_REQUIRED
cprMsgQueue_t ticker_msgq;
cprThread_t ticker_thread;
#endif


boolean platform_initialized = FALSE;

static int thread_init(void);











extern void gsm_set_initialized(void);
extern void vcm_init(void);
extern void dp_init(void *);
extern cprBuffer_t SIPTaskGetBuffer(uint16_t size);

extern void sip_platform_task_loop(void *arg);
#ifdef NO_SOCKET_POLLING
extern void sip_platform_task_msgqwait(void *arg);
#endif
extern void GSMTask(void *);
#ifndef VENDOR_BUILD
extern void debug_task(void *);
#endif
extern void MiscAppTask(void *);
extern void cpr_timer_tick(void);

extern void cprTimerSystemInit(void);
extern int32_t ui_clear_mwi(int32_t argc, const char *argv[]);
void gsm_shutdown(void);
void dp_shutdown(void);
void MiscAppTaskShutdown(void);
void CCAppShutdown(void);
cprBuffer_t gsm_get_buffer (uint16_t size);








#ifdef EXTERNAL_TICK_REQUIRED
int TickerTask(void *);
#endif

void send_protocol_config_msg(void);




extern
int ccMemInit(size_t size) {
    return CPR_SUCCESS;
}












int
ccPreInit ()
{
	static boolean ccPreInit_called = FALSE;

	if (ccPreInit_called == FALSE) {
		ccPreInit_called = TRUE;
		
		ccMemInit(PRIVATE_SYS_MEM_SIZE);
		cprPreInit();
	}

    return CPR_SUCCESS;
}

int
ccInit ()
{

    TNP_DEBUG(DEB_F_PREFIX"started init of SIP call control\n", DEB_F_PREFIX_ARGS(SIP_CC_INIT, "ccInit"));

    platInit();

    strlib_init();

    


#if defined(_WIN32) && defined(CPR_TIMERS_ENABLED)
    cprTimerSystemInit();
#endif

    
    (void) thread_init();

    platform_initialized = TRUE;

    return 0;
}

static int
thread_init ()
{
    gStopTickTask = FALSE;
    



    (void) cprPreInit();


    PHNChangeState(STATE_FILE_CFG);

    
    sip_msgq = cprCreateMessageQueue("SIPQ", SIPQSZ);
    gsm_msgq = cprCreateMessageQueue("GSMQ", GSMQSZ);

    if (FALSE == gHardCodeSDPMode) {
        misc_app_msgq = cprCreateMessageQueue("MISCAPPQ", DEFQSZ);
    }
    ccapp_msgq = cprCreateMessageQueue("CCAPPQ", DEFQSZ);
#ifdef JINDO_DEBUG_SUPPORTED
    debug_msgq = cprCreateMessageQueue("DEBUGAPPQ", DEFQSZ);
#endif
#ifdef EXTERNAL_TICK_REQUIRED
    ticker_msgq = cprCreateMessageQueue("Ticker", DEFQSZ);
#endif


    


    debugInit();

    
    ccapp_thread = cprCreateThread("CCAPP Task",
                                 (cprThreadStartRoutine) CCApp_task,
                                 GSMSTKSZ, CCPROVIDER_THREAD_RELATIVE_PRIORITY , ccapp_msgq);
    MOZ_ASSERT(ccapp_thread);
    if (ccapp_thread) {
        thread_started(THREADMON_CCAPP, ccapp_thread);
    } else {
        err_msg("failed to create CCAPP task \n");
    }

#ifdef JINDO_DEBUG_SUPPORTED
#ifndef VENDOR_BUILD
    debug_thread = cprCreateThread("Debug Task",
                                   (cprThreadStartRoutine) debug_task, STKSZ,
                                   0  , debug_msgq);

    if (debug_thread == NULL) {
        err_msg("failed to create debug task\n");
    }
#endif
#endif

    
    sip_thread = cprCreateThread("SIPStack task",
                                 (cprThreadStartRoutine) sip_platform_task_loop,
                                 STKSZ, SIP_THREAD_RELATIVE_PRIORITY , sip_msgq);
    MOZ_ASSERT(sip_thread);
    if (sip_thread) {
        thread_started(THREADMON_SIP, sip_thread);
    } else {
        err_msg("failed to create sip task \n");
    }

#ifdef NO_SOCKET_POLLING
    
    sip_msgqwait_thread = cprCreateThread("SIP MsgQueueWait task",
                                          (cprThreadStartRoutine)
                                          sip_platform_task_msgqwait,
                                          STKSZ, SIP_THREAD_RELATIVE_PRIORITY , sip_msgq);
    MOZ_ASSERT(sip_msgqwait_thread);
    if (sip_msgqwait_thread) {
        thread_started(THREADMON_MSGQ, sip_msgqwait_thread);
    } else {
        err_msg("failed to create sip message queue wait task\n");
    }
#endif

    gsm_thread = cprCreateThread("GSM Task",
                                 (cprThreadStartRoutine) GSMTask,
                                 GSMSTKSZ, GSM_THREAD_RELATIVE_PRIORITY , gsm_msgq);
    MOZ_ASSERT(gsm_thread);
    if (gsm_thread) {
        thread_started(THREADMON_GSM, gsm_thread);
    } else {
        err_msg("failed to create gsm task \n");
    }

    if (FALSE == gHardCodeSDPMode) {
    	misc_app_thread = cprCreateThread("MiscApp Task",
    			(cprThreadStartRoutine) MiscAppTask,
    			STKSZ, 0 , misc_app_msgq);
    	if (misc_app_thread == NULL) {
    		err_msg("failed to create MiscApp task \n");
    	}
    }

#ifdef EXTERNAL_TICK_REQUIRED
    ticker_thread = cprCreateThread("Ticker task",
                                    (cprThreadStartRoutine) TickerTask,
                                    STKSZ, 0, ticker_msgq);
    if (ticker_thread == NULL) {
        err_msg("failed to create ticker task \n");
    }
#endif

    
    (void) cprSetMessageQueueThread(sip_msgq, sip_thread);
    (void) cprSetMessageQueueThread(gsm_msgq, gsm_thread);

    if (FALSE == gHardCodeSDPMode) {
    	(void) cprSetMessageQueueThread(misc_app_msgq, misc_app_thread);
    }

    (void) cprSetMessageQueueThread(ccapp_msgq, ccapp_thread);
#ifdef JINDO_DEBUG_SUPPORTED
    (void) cprSetMessageQueueThread(debug_msgq, debug_thread);
#endif
#ifdef EXTERNAL_TICK_REQUIRED
    (void) cprSetMessageQueueThread(ticker_msgq, ticker_thread);
#endif

    













    config_init();
    vcmInit();
    dp_init(gsm_msgq);

    if (sip_minimum_config_check() != 0) {
        PHNChangeState(STATE_UNPROVISIONED);
    } else {
        PHNChangeState(STATE_CONNECTED);
    }

    (void) cprPostInit();

    if ( vcmGetVideoCodecList(VCM_DSP_FULLDUPLEX) ) {
        cc_media_update_native_video_support(TRUE);
    }

    return (0);
}


#ifdef EXTERNAL_TICK_REQUIRED

uint16_t SecTimer = 50;

unsigned long timeofday_in_seconds = 0;

void
MAIN0Timer (void)
{
    if (SecTimer-- == 0) {
        SecTimer = 50;
        timeofday_in_seconds++;
    }
    
    cpr_timer_tick();
}

int
TickerTask (void *a)
{
    TNP_DEBUG(DEB_F_PREFIX"Ticker Task initialized..\n", DEB_F_PREFIX_ARGS(SIP_CC_INIT, "TickerTask"));
    while (FALSE == gStopTickTask) {
        cprSleep(20);
        MAIN0Timer();
    }
    return 0;
}
#endif

void
send_protocol_config_msg (void)
{
    const char *fname = "send_protocol_config_msg";
    char *msg;

    TNP_DEBUG(DEB_F_PREFIX"send TCP_DONE message to sip thread..\n", DEB_F_PREFIX_ARGS(SIP_CC_INIT, fname));

    msg = (char *) SIPTaskGetBuffer(4);
    if (msg == NULL) {
        TNP_DEBUG(DEB_F_PREFIX"failed to allocate message..\n", DEB_F_PREFIX_ARGS(SIP_CC_INIT, fname));
        return;
    }
    
    if (SIPTaskSendMsg(TCP_PHN_CFG_TCP_DONE, msg, 0, NULL) == CPR_FAILURE) {
        err_msg("%s: notify SIP stack ready failed", fname);
        cpr_free(msg);
    }
    gsm_set_initialized();
    PHNChangeState(STATE_CONNECTED);
}
















void
send_task_unload_msg(cc_srcs_t dest_id)
{
    const char *fname = "send_task_unload_msg";
    uint16_t len = 4;
    cprBuffer_t  msg =  gsm_get_buffer(len);
    int  sdpmode = 0;

    config_get_value(CFGID_SDPMODE, &sdpmode, sizeof(sdpmode));

    if (msg == NULL) {
        err_msg("%s: failed to allocate  msg cprBuffer_t\n", fname);
        return;
    }

    DEF_DEBUG(DEB_F_PREFIX"send Unload message to %s task ..\n",
        DEB_F_PREFIX_ARGS(SIP_CC_INIT, fname),
        dest_id == CC_SRC_SIP ? "SIP" :
        dest_id == CC_SRC_GSM ? "GSM" :
        dest_id == CC_SRC_MISC_APP ? "Misc App" :
        dest_id == CC_SRC_CCAPP ? "CCApp" : "Unknown");

    switch(dest_id) {
        case CC_SRC_SIP:
        {
            
            SIPTaskPostShutdown(SIP_EXTERNAL, CC_CAUSE_SHUTDOWN, "");
            

            if (!sdpmode) {
                cprSleep(2000);
            }
            
            msg =  SIPTaskGetBuffer(len);
            if (msg == NULL) {
                err_msg("%s:%d: failed to allocate sip msg buffer\n", fname);
                return;
            }

            if (SIPTaskSendMsg(THREAD_UNLOAD, (cprBuffer_t)msg, len, NULL) == CPR_FAILURE)
            {
                cpr_free(msg);
                err_msg("%s: Unable to send THREAD_UNLOAD msg to sip thread", fname);
            }
        }
        break;
        case CC_SRC_GSM:
        {
            msg =  gsm_get_buffer(len);
            if (msg == NULL) {
                err_msg("%s: failed to allocate  gsm msg cprBuffer_t\n", fname);
                return;
            }
            if (CPR_FAILURE == gsm_send_msg(THREAD_UNLOAD, msg, len)) {
                err_msg("%s: Unable to send THREAD_UNLOAD msg to gsm thread", fname);
            }
        }
        break;
        case CC_SRC_MISC_APP:
        {
            msg = cpr_malloc(len);
            if (msg == NULL) {
                err_msg("%s: failed to allocate  misc msg cprBuffer_t\n", fname);
                return;
            }
            if (CPR_FAILURE == MiscAppTaskSendMsg(THREAD_UNLOAD, msg, len)) {
                err_msg("%s: Unable to send THREAD_UNLOAD msg to Misc App thread", fname);
            }
        }
        break;
        case CC_SRC_CCAPP:
        {
            msg = cpr_malloc(len);
            if (msg == NULL) {
                err_msg("%s: failed to allocate  ccapp msg cprBuffer_t\n", fname);
                return;
            }
            if (ccappTaskPostMsg(CCAPP_THREAD_UNLOAD, msg, len, CCAPP_CCPROVIER) == CPR_FAILURE )
            {
                err_msg("%s: Unable to send THREAD_UNLOAD msg to CCapp thread", fname);
            }
            err_msg("%s:  send UNLOAD msg to CCapp thread good", fname);
        }
        break;

        default:
            err_msg("%s: Unknown destination task passed=%d.", fname, dest_id);
        break;
    }
}













void
ccUnload (void)
{
    static const char fname[] = "ccUnload";

    DEF_DEBUG(DEB_F_PREFIX"ccUnload called..\n", DEB_F_PREFIX_ARGS(SIP_CC_INIT, fname));
    if (platform_initialized == FALSE)
    {
        TNP_DEBUG(DEB_F_PREFIX"system is not loaded, ignore unload\n", DEB_F_PREFIX_ARGS(SIP_CC_INIT, fname));
        return;
    }
    



    send_task_unload_msg(CC_SRC_SIP);
    send_task_unload_msg(CC_SRC_GSM);

    if (FALSE == gHardCodeSDPMode) {
    	send_task_unload_msg(CC_SRC_MISC_APP);
    }

    send_task_unload_msg(CC_SRC_CCAPP);

    gStopTickTask = TRUE;

    


    join_all_threads();
}


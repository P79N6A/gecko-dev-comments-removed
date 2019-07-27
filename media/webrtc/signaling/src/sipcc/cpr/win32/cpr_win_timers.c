



#include "cpr.h"
#include "cpr_types.h"
#include "cpr_timers.h"
#include "cpr_win_timers.h"
#include "cpr_stdio.h"
#include "cpr_memory.h"
#include "cpr_stdlib.h"
#include "platform_api.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>




#define TIMER_FREE 0x1          /* Indicates timer is free */
#define TIMER_INITIALIZED 0x2   /* Indicates timer is initialized */
#define TIMER_ACTIVE 0x4        /* Indicates timer is in list */

#define CPRTMR_STK 0            /* CPR Timer stack size */
#define CPRTMR_PRIO 0           /* CPR Timer task priority */

extern void cprDisableSwap(void);
extern void cprEnableSwap(void);


typedef struct cprTimerDef {
    const char *name;
    cprMsgQueue_t callBackMsgQueue;
    uint16_t applicationTimerId;
    uint16_t applicationMsgId;
    void *data;
    uint32_t expiration_time;
    uint32_t flags;
    struct cprTimerDef *previous;
    struct cprTimerDef *next;
} cprTimerBlk;


static uint32_t expired_count;  
static uint32_t removed_count;  
static uint32_t inserted_count; 
static uint32_t cpr_timer_init; 








extern void *fillInSysHeader(void *buffer, uint16_t cmd, uint16_t len,
                             void *timerMsg);


static uint32_t ticker;
unsigned long current_time(void);





cprTimerBlk *cprTimerPendingList;




static char tmr_msg[] = "WakeUpTimer";











void
cprSleep (uint32_t duration)
{
    Sleep(duration);
};















void *
timer_event_allocate (void)
{
    cprTimerBlk *new_timer;

    cprDisableSwap();

    new_timer = (cprTimerBlk *) cpr_malloc(sizeof(cprTimerBlk));
    if (new_timer == NULL) {
        
        CPR_ERROR("!timer_event_allocate:  Could not malloc a new timer\n");
    } else {
        new_timer->flags &= ~TIMER_FREE;
    }

    cprEnableSwap();

    return (new_timer);
}

















static void
timer_insert (cprTimerBlk * timer)
{

    cprTimerBlk *element; 
    cprTimerBlk *pred;    
    int insert_at_front;  

    cprDisableSwap();

    if (cprTimerPendingList == NULL) {
        insert_at_front = 1;
    } else if (timer->expiration_time <= cprTimerPendingList->expiration_time) {
        insert_at_front = 1;
    } else {
        insert_at_front = 0;
    }

    if (insert_at_front) {
        
        if (cprTimerPendingList != NULL) {
            cprTimerPendingList->previous = timer;
        }
        timer->next = cprTimerPendingList;
        timer->previous = NULL;
        cprTimerPendingList = timer;
    } else {
        
        pred = cprTimerPendingList;
        element = cprTimerPendingList->next;

        
        while (element != NULL) {
            if (timer->expiration_time <= element->expiration_time) {
                
                break;
            } else {
                
                pred = element;
                element = element->next;
            }
        }

        
        timer->previous = pred;
        timer->next = pred->next;
        pred->next = timer;

        if (element != NULL) {
            element->previous = timer;
        }
    }

    cprEnableSwap();
}

















static void
timer_remove (cprTimerBlk *timer)
{

    cprDisableSwap();
    if (cprTimerPendingList) {
        if (cprTimerPendingList == timer) {
            
            cprTimerPendingList = timer->next;
            if (timer->next != NULL) {
                timer->next->previous = NULL;
            }
        } else {
            
            timer->previous->next = timer->next;
            if (timer->next != NULL) {
                timer->next->previous = timer->previous;
            }
        }
        timer->next = NULL;
        timer->previous = NULL;
    }
    cprEnableSwap();
}

















cprTimer_t
cprCreateTimer (const char *name,
                uint16_t applicationTimerId,
                uint16_t applicationMsgId,
                cprMsgQueue_t callBackMsgQueue)
{

    cprTimerBlk *cprTimerPtr = NULL;

    if ((cprTimerPtr = (cprTimerBlk*)timer_event_allocate()) != NULL) {
        if (name != NULL) {
            cprTimerPtr->name = name;
        }
        
        cprTimerPtr->applicationTimerId = applicationTimerId;
        cprTimerPtr->applicationMsgId = applicationMsgId;
        if (callBackMsgQueue == NULL) {
            CPR_ERROR("Callback msg queue for timer %s is NULL.\n", name);
            return (NULL);
        }
        cprTimerPtr->callBackMsgQueue = callBackMsgQueue;
        cprTimerPtr->flags |= TIMER_INITIALIZED;
    } else {
        CPR_ERROR("Failed to CreateTimer.\n");
    }
    return cprTimerPtr;
}













cprRC_t
cprStartTimer (cprTimer_t timer, uint32_t duration, void *data)
{
    static const char fname[] = "cprStartTimer";

    cprTimerBlk *cprTimerPtr = (cprTimerBlk*)timer;

    cprTimerPtr = (cprTimerBlk *) timer;
    if (cprTimerPtr != NULL) {
        if (cprTimerPtr->flags & TIMER_FREE) {
            CPR_ERROR("!timer_event_activate: %x %s\n", timer,
                      "Attempting to activate free timer\n");
        } else if (cprTimerPtr->flags & TIMER_ACTIVE) {
            CPR_ERROR("!timer_event_activate: %x %s\n", timer,
                      "Attempting to activate active timer");
        } else if ((cprTimerPtr->flags & TIMER_INITIALIZED) == 0) {
            CPR_ERROR("!timer_event_activate: %x %s\n", timer,
                      "Attempting to activate uninitialized timer");
        } else {
            
            cprTimerPtr->data = data;
            cprTimerPtr->expiration_time = current_time() + duration;

            
            timer_insert(cprTimerPtr);
            cprTimerPtr->flags |= TIMER_ACTIVE;
        }
        
    } else {
        CPR_ERROR("%s - NULL pointer passed in.\n", fname);
        return (CPR_FAILURE);
    }
    return (CPR_SUCCESS);
}












boolean
cprIsTimerRunning (cprTimer_t timer)
{
    static const char fname[] = "cprIsTimerRunning";
    cprTimerBlk *cprTimerPtr = (cprTimerBlk*)timer;

    if (cprTimerPtr != NULL) {
        return ((uint8_t) (cprTimerPtr->flags & TIMER_ACTIVE));

        
    } else {
        CPR_ERROR("%s - NULL pointer passed in.\n", fname);
        return (FALSE);
    }
}











cprRC_t
cprCancelTimer (cprTimer_t timer)
{
    static const char fname[] = "cprCancelTimer";
    cprTimerBlk *cprTimerPtr = (cprTimerBlk *) timer;

    if (cprTimerPtr != NULL) {
        if (cprTimerPtr->flags & TIMER_ACTIVE) {
            
            timer_remove((cprTimerBlk*)timer);
            cprTimerPtr->flags &= ~TIMER_ACTIVE;
        } else {
            
        }
        removed_count++;
        return (CPR_SUCCESS);
        
    } else {
        CPR_ERROR("%s - NULL pointer passed in.\n", fname);
        return (CPR_FAILURE);
    }
}












cprRC_t
cprUpdateTimer (cprTimer_t timer, uint32_t duration)
{
    void *timerData;
    static const char fname[] = "cprUpdateTimer";
    cprTimerBlk *cprTimerPtr = (cprTimerBlk*)timer;

    if (cprTimerPtr != NULL) {
        
        timerData = cprTimerPtr->data;
    } else {
        CPR_ERROR("%s - NULL pointer passed in.\n", fname);
        return (CPR_FAILURE);
    }

    if (cprCancelTimer(timer) == CPR_SUCCESS) {
        if (cprStartTimer(timer, duration, timerData) == CPR_SUCCESS) {
            return (CPR_SUCCESS);
        } else {
            CPR_ERROR("%s - Failed to start timer %s\n", fname,
                      cprTimerPtr->name);
            return (CPR_FAILURE);
        }
    }

    CPR_ERROR("%s - Failed to cancel timer %s\n", fname,
              cprTimerPtr->name);
    return (CPR_FAILURE);
}












cprRC_t
cprDestroyTimer (cprTimer_t timer)
{
    static const char fname[] = "cprDestroyTimer";
    cprTimerBlk *cprTimerPtr = (cprTimerBlk *) timer;
    cprRC_t returnCode = CPR_FAILURE;

    cprDisableSwap();

    if (cprTimerPtr != NULL) {
        if (cprTimerPtr->flags & TIMER_FREE) {
            CPR_ERROR("!timer_event_free: %x %s\n", timer,
                      "Attempting to free timer that is already free");
        } else if (cprTimerPtr->flags & TIMER_ACTIVE) {
            CPR_ERROR("!timer_event_free: %x %s\n", timer,
                      "Attempting to free active timer");
        } else {
            
            cprTimerPtr->flags = TIMER_FREE;
            cprTimerPtr->next = NULL;
            cpr_free(cprTimerPtr);
            returnCode = CPR_SUCCESS;
        }

        
    } else {
        CPR_ERROR("%s - NULL pointer passed in.\n", fname);
    }

    cprEnableSwap();
    return (returnCode);
}












boolean
cpr_timer_expired (void)
{
    unsigned int now;           
    cprTimerBlk *cprTimerPtr;   

    cprDisableSwap();

    now = current_time();

    cprTimerPtr = cprTimerPendingList;

    while (cprTimerPtr != NULL) {
        if (cprTimerPtr->expiration_time <= now) {
            cprEnableSwap();
            return TRUE;
        } else {
            
            break;
        }
    }
    cprEnableSwap();
    return FALSE;
}



















void
cpr_timer_event_process (void)
{
}

















void
cpr_timer_tick (void)
{
}















unsigned long
current_time (void)
{
    return (ticker);
}
















void
cprTimerSystemInit (void)
{
    
    cprTimerPendingList = NULL;

    expired_count = 0;
    removed_count = 0;
    inserted_count = 0;

    ticker = 0;
    cpr_timer_init = 1;

    
}


void
cpr_timer_stop (void)
{
    cpr_timer_init = 0;
}

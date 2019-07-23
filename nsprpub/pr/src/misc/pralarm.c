




































#include "primpl.h"





#include "obsolete/pralarm.h"

struct PRAlarmID {                       
    PRCList list;                        
    PRAlarm *alarm;                      
    PRPeriodicAlarmFn function;          
    void *clientData;                    
    PRIntervalTime period;               
    PRUint32 rate;                       

    PRUint32 accumulator;                
    PRIntervalTime epoch;                
    PRIntervalTime nextNotify;           
    PRIntervalTime lastNotify;           
};

typedef enum {alarm_active, alarm_inactive} _AlarmState;

struct PRAlarm {                         
    PRCList timers;                      
    PRLock *lock;                        
    PRCondVar *cond;                     
    PRThread *notifier;                  
    PRAlarmID *current;                  
    _AlarmState state;                   
};

static PRAlarmID *pr_getNextAlarm(PRAlarm *alarm, PRAlarmID *id)
{







    PRCList *timer;
    PRAlarmID *result = id;
    PRIntervalTime now = PR_IntervalNow();

    if (!PR_CLIST_IS_EMPTY(&alarm->timers))
    {    
        if (id != NULL)  
        {        
            PRIntervalTime idDelta = now - id->nextNotify;
            timer = alarm->timers.next;
            do
            {
                result = (PRAlarmID*)timer;
                if ((PRIntervalTime)(now - result->nextNotify) > idDelta)
                {
                    PR_INSERT_BEFORE(&id->list, &alarm->timers);
                    break;
                }
                timer = timer->next;
            } while (timer != &alarm->timers);
        }
        result = (PRAlarmID*)(timer = PR_LIST_HEAD(&alarm->timers));
        PR_REMOVE_LINK(timer);  
    }

    return result;
}  

static PRIntervalTime pr_PredictNextNotifyTime(PRAlarmID *id)
{
    PRIntervalTime delta;
    PRFloat64 baseRate = (PRFloat64)id->period / (PRFloat64)id->rate;
    PRFloat64 offsetFromEpoch = (PRFloat64)id->accumulator * baseRate;

    id->accumulator += 1;  
    id->lastNotify = id->nextNotify;  
    id->nextNotify = (PRIntervalTime)(offsetFromEpoch + 0.5);

    delta = id->nextNotify - id->lastNotify;
    return delta;
}  

static void PR_CALLBACK pr_alarmNotifier(void *arg)
{
    





    PRAlarmID *id = NULL;
    PRAlarm *alarm = (PRAlarm*)arg;
    enum {notify, abort, scan} why = scan;

    while (why != abort)
    {
        PRIntervalTime pause;

        PR_Lock(alarm->lock);
        while (why == scan)
        {
            alarm->current = NULL;  
            if (alarm->state == alarm_inactive) why = abort;  
            else if (why == scan)  
            {
                id = pr_getNextAlarm(alarm, id);  
                if (id == NULL)  
                    (void)PR_WaitCondVar(alarm->cond, PR_INTERVAL_NO_TIMEOUT);
                else
                {
                    pause = id->nextNotify - (PR_IntervalNow() - id->epoch);
                    if ((PRInt32)pause <= 0)  
                    {
                        why = notify;  
                        alarm->current = id;  
                    }
                    else
                        (void)PR_WaitCondVar(alarm->cond, pause);  
                }
            }
        }
        PR_Unlock(alarm->lock);

        if (why == notify)
        {
            (void)pr_PredictNextNotifyTime(id);
            if (!id->function(id, id->clientData, ~pause))
            {
                




                PR_DELETE(id);  
                id = NULL;  
            }
            why = scan;  
        }
    }

}  

PR_IMPLEMENT(PRAlarm*) PR_CreateAlarm(void)
{
    PRAlarm *alarm = PR_NEWZAP(PRAlarm);
    if (alarm != NULL)
    {
        if ((alarm->lock = PR_NewLock()) == NULL) goto done;
        if ((alarm->cond = PR_NewCondVar(alarm->lock)) == NULL) goto done;
        alarm->state = alarm_active;
        PR_INIT_CLIST(&alarm->timers);
        alarm->notifier = PR_CreateThread(
            PR_USER_THREAD, pr_alarmNotifier, alarm,
            PR_GetThreadPriority(PR_GetCurrentThread()),
            PR_LOCAL_THREAD, PR_JOINABLE_THREAD, 0);
        if (alarm->notifier == NULL) goto done;
    }
    return alarm;

done:
    if (alarm->cond != NULL) PR_DestroyCondVar(alarm->cond);
    if (alarm->lock != NULL) PR_DestroyLock(alarm->lock);
    PR_DELETE(alarm);
    return NULL;
}  

PR_IMPLEMENT(PRStatus) PR_DestroyAlarm(PRAlarm *alarm)
{
    PRStatus rv;

    PR_Lock(alarm->lock);
    alarm->state = alarm_inactive;
    rv = PR_NotifyCondVar(alarm->cond);
    PR_Unlock(alarm->lock);

    if (rv == PR_SUCCESS)
        rv = PR_JoinThread(alarm->notifier);
    if (rv == PR_SUCCESS)
    {
        PR_DestroyCondVar(alarm->cond);
        PR_DestroyLock(alarm->lock);
        PR_DELETE(alarm);
    }
    return rv;
}  

PR_IMPLEMENT(PRAlarmID*) PR_SetAlarm(
    PRAlarm *alarm, PRIntervalTime period, PRUint32 rate,
    PRPeriodicAlarmFn function, void *clientData)
{
    






    PRAlarmID *id = PR_NEWZAP(PRAlarmID);

    if (!id)
        return NULL;

    id->alarm = alarm;
    PR_INIT_CLIST(&id->list);
    id->function = function;
    id->clientData = clientData;
    id->period = period;
    id->rate = rate;
    id->epoch = id->nextNotify = PR_IntervalNow();
    (void)pr_PredictNextNotifyTime(id);

    PR_Lock(alarm->lock);
    PR_INSERT_BEFORE(&id->list, &alarm->timers);
    PR_NotifyCondVar(alarm->cond);
    PR_Unlock(alarm->lock);

    return id;
}  

PR_IMPLEMENT(PRStatus) PR_ResetAlarm(
    PRAlarmID *id, PRIntervalTime period, PRUint32 rate)
{
    




    if (id != id->alarm->current)
        return PR_FAILURE;
    id->period = period;
    id->rate = rate;
    id->accumulator = 1;
    id->epoch = PR_IntervalNow();
    (void)pr_PredictNextNotifyTime(id);
    return PR_SUCCESS;
}  




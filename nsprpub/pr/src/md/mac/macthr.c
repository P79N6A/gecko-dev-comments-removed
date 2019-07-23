




































#include "primpl.h"

#include <string.h>

#include <MacTypes.h>
#include <Timer.h>
#include <OSUtils.h>
#include <Math64.h>
#include <LowMem.h>
#include <Multiprocessing.h>
#include <Gestalt.h>

#include "mdcriticalregion.h"

TimerUPP	gTimerCallbackUPP	= NULL;
PRThread *	gPrimaryThread		= NULL;

ProcessSerialNumber		gApplicationProcess;

PR_IMPLEMENT(PRThread *) PR_GetPrimaryThread()
{
	return gPrimaryThread;
}



#pragma mark -
#pragma mark CREATING MACINTOSH THREAD STACKS

#if defined(GC_LEAK_DETECTOR)
extern void* GC_malloc_atomic(PRUint32 size);
#endif








PRStatus _MD_AllocSegment(PRSegment *seg, PRUint32 size, void *vaddr)
{
	PR_ASSERT(seg != 0);
	PR_ASSERT(size != 0);
	PR_ASSERT(vaddr == 0);

	



#if defined(GC_LEAK_DETECTOR)
	seg->vaddr = (char *)GC_malloc_atomic(size);
#else
	seg->vaddr = (char *)malloc(size);
#endif

	if (seg->vaddr == NULL) {

#if DEBUG
		DebugStr("\p_MD_AllocSegment failed.");
#endif

		return PR_FAILURE;
	}

	seg->size = size;	

	return PR_SUCCESS;
}





void _MD_FreeSegment(PRSegment *seg)
{
	PR_ASSERT((seg->flags & _PR_SEG_VM) == 0);

	if (seg->vaddr != NULL)
		free(seg->vaddr);
}









void _MD_InitStack(PRThreadStack *ts, int redZoneBytes)
	{
#pragma unused (redZoneBytes)
#if DEVELOPER_DEBUG
	
	
	
	memset(ts->allocBase, 0xDC, ts->stackSize);
	
	((UInt32 *)ts->stackTop)[-1] = 0xBEEFCAFE;
	((UInt32 *)ts->stackTop)[-2] = (UInt32)gPrimaryThread;
	((UInt32 *)ts->stackTop)[-3] = (UInt32)(ts);
	((UInt32 *)ts->stackBottom)[0] = 0xCAFEBEEF;
#else
#pragma unused (ts)
#endif	
	}

extern void _MD_ClearStack(PRThreadStack *ts)
	{
#if DEVELOPER_DEBUG
	
	
	memset(ts->allocBase, 0xEF, ts->allocSize);
	((UInt32 *)ts->stackTop)[-1] = 0;
	((UInt32 *)ts->stackTop)[-2] = 0;
	((UInt32 *)ts->stackTop)[-3] = 0;
	((UInt32 *)ts->stackBottom)[0] = 0;
#else
#pragma unused (ts)
#endif
	}




#pragma mark -
#pragma mark TIME MANAGER-BASED CLOCK













static Boolean  gTimeManagerTaskDoesWUP;

static TMTask   gTimeManagerTaskElem;

extern void _MD_IOInterrupt(void);
_PRInterruptTable _pr_interruptTable[] = {
    { "clock", _PR_MISSED_CLOCK, _PR_ClockInterrupt, },
    { "i/o", _PR_MISSED_IO, _MD_IOInterrupt, },
    { 0 }
};

#define kMacTimerInMiliSecs 8L

pascal void TimerCallback(TMTaskPtr tmTaskPtr)
{
    _PRCPU *cpu = _PR_MD_CURRENT_CPU();
    PRIntn is;

    if (_PR_MD_GET_INTSOFF()) {
        cpu->u.missed[cpu->where] |= _PR_MISSED_CLOCK;
        PrimeTime((QElemPtr)tmTaskPtr, kMacTimerInMiliSecs);
        return;
    }

    _PR_INTSOFF(is);

    
    _PR_ClockInterrupt();
	
    if ((_PR_RUNQREADYMASK(cpu)) >> ((_PR_MD_CURRENT_THREAD()->priority))) {
        if (gTimeManagerTaskDoesWUP) {
            
            
            
            
            if (UnsignedWideToUInt64(cpu->md.lastThreadSwitch) > UnsignedWideToUInt64(cpu->md.lastWakeUpProcess))
            {
                WakeUpProcess(&gApplicationProcess);
                cpu->md.lastWakeUpProcess = UpTime();
            }
        }
        _PR_SET_RESCHED_FLAG();
	}
	
    _PR_FAST_INTSON(is);

    
    PrimeTime((QElemPtr)tmTaskPtr, kMacTimerInMiliSecs);
}


void _MD_StartInterrupts(void)
{
	gPrimaryThread = _PR_MD_CURRENT_THREAD();

	gTimeManagerTaskDoesWUP = RunningOnOSX();

	if ( !gTimerCallbackUPP )
		gTimerCallbackUPP = NewTimerUPP(TimerCallback);

	
	
	gTimeManagerTaskElem.tmAddr = (TimerUPP)gTimerCallbackUPP;
	gTimeManagerTaskElem.tmCount = 0;
	gTimeManagerTaskElem.tmWakeUp = 0;
	gTimeManagerTaskElem.tmReserved = 0;

	
	InsTime((QElemPtr)&gTimeManagerTaskElem);
	
	PrimeTime((QElemPtr)&gTimeManagerTaskElem, kMacTimerInMiliSecs);
}

void _MD_StopInterrupts(void)
{
	if (gTimeManagerTaskElem.tmAddr != NULL) {
		RmvTime((QElemPtr)&gTimeManagerTaskElem);
		gTimeManagerTaskElem.tmAddr = NULL;
	}
}


#define MAX_PAUSE_TIMEOUT_MS    500

void _MD_PauseCPU(PRIntervalTime timeout)
{
    if (timeout != PR_INTERVAL_NO_WAIT)
    {
        
        
        
        
        
        if (PR_IntervalToMilliseconds(timeout) > MAX_PAUSE_TIMEOUT_MS)
            timeout = PR_MillisecondsToInterval(MAX_PAUSE_TIMEOUT_MS);

        WaitOnIdleSemaphore(timeout);
        (void) _MD_IOInterrupt();
    }
}

void _MD_InitRunningCPU(_PRCPU* cpu)
{
    cpu->md.trackScheduling = RunningOnOSX();
    if (cpu->md.trackScheduling) {
        AbsoluteTime    zeroTime = {0, 0};
        cpu->md.lastThreadSwitch = UpTime();
        cpu->md.lastWakeUpProcess = zeroTime;
    }
}




#pragma mark -
#pragma mark THREAD SUPPORT FUNCTIONS

#include <OpenTransport.h> 

PRStatus _MD_InitThread(PRThread *thread)
{
	thread->md.asyncIOLock = PR_NewLock();
	PR_ASSERT(thread->md.asyncIOLock != NULL);
	thread->md.asyncIOCVar = PR_NewCondVar(thread->md.asyncIOLock);
	PR_ASSERT(thread->md.asyncIOCVar != NULL);

	if (thread->md.asyncIOLock == NULL || thread->md.asyncIOCVar == NULL)
		return PR_FAILURE;
	else
		return PR_SUCCESS;
}

PRStatus _MD_wait(PRThread *thread, PRIntervalTime timeout)
{
#pragma unused (timeout)

	_MD_SWITCH_CONTEXT(thread);
	return PR_SUCCESS;
}


void WaitOnThisThread(PRThread *thread, PRIntervalTime timeout)
{
    intn is;
    PRIntervalTime timein = PR_IntervalNow();
	PRStatus status = PR_SUCCESS;

    
    
    
    
    
	_PR_INTSOFF(is);
	PR_Lock(thread->md.asyncIOLock);
	if (timeout == PR_INTERVAL_NO_TIMEOUT) {
	    while ((thread->io_pending) && (status == PR_SUCCESS))
	        status = PR_WaitCondVar(thread->md.asyncIOCVar, PR_INTERVAL_NO_TIMEOUT);
	} else {
	    while ((thread->io_pending) && ((PRIntervalTime)(PR_IntervalNow() - timein) < timeout) && (status == PR_SUCCESS))
	        status = PR_WaitCondVar(thread->md.asyncIOCVar, timeout);
	}
	if ((status == PR_FAILURE) && (PR_GetError() == PR_PENDING_INTERRUPT_ERROR)) {
		thread->md.osErrCode = kEINTRErr;
	} else if (thread->io_pending) {
		thread->md.osErrCode = kETIMEDOUTErr;
		PR_SetError(PR_IO_TIMEOUT_ERROR, kETIMEDOUTErr);
	}

	thread->io_pending = PR_FALSE;
	PR_Unlock(thread->md.asyncIOLock);
	_PR_FAST_INTSON(is);
}


void DoneWaitingOnThisThread(PRThread *thread)
{
    intn is;

    PR_ASSERT(thread->md.asyncIOLock->owner == NULL);

	
	
	
	
	
	
	
	
	
	
	
	
	
	_PR_INTSOFF(is);
	PR_Lock(thread->md.asyncIOLock);
	thread->io_pending = PR_FALSE;
	
	PR_NotifyCondVar(thread->md.asyncIOCVar);
	PR_Unlock(thread->md.asyncIOLock);
	_PR_FAST_INTSON(is);
}


PR_IMPLEMENT(void) PR_Mac_WaitForAsyncNotify(PRIntervalTime timeout)
{
    intn is;
    PRIntervalTime timein = PR_IntervalNow();
	PRStatus status = PR_SUCCESS;
    PRThread *thread = _PR_MD_CURRENT_THREAD();

    
	_PR_INTSOFF(is);
	PR_Lock(thread->md.asyncIOLock);
	if (timeout == PR_INTERVAL_NO_TIMEOUT) {
	    while ((!thread->md.asyncNotifyPending) && (status == PR_SUCCESS))
	        status = PR_WaitCondVar(thread->md.asyncIOCVar, PR_INTERVAL_NO_TIMEOUT);
	} else {
	    while ((!thread->md.asyncNotifyPending) && ((PRIntervalTime)(PR_IntervalNow() - timein) < timeout) && (status == PR_SUCCESS))
	        status = PR_WaitCondVar(thread->md.asyncIOCVar, timeout);
	}
	if ((status == PR_FAILURE) && (PR_GetError() == PR_PENDING_INTERRUPT_ERROR)) {
		thread->md.osErrCode = kEINTRErr;
	} else if (!thread->md.asyncNotifyPending) {
		thread->md.osErrCode = kETIMEDOUTErr;
		PR_SetError(PR_IO_TIMEOUT_ERROR, kETIMEDOUTErr);
	}
	thread->md.asyncNotifyPending = PR_FALSE;
	PR_Unlock(thread->md.asyncIOLock);
	_PR_FAST_INTSON(is);
}


void AsyncNotify(PRThread *thread)
{
    intn is;
	
    PR_ASSERT(thread->md.asyncIOLock->owner == NULL);

    
	_PR_INTSOFF(is);
	PR_Lock(thread->md.asyncIOLock);
	thread->md.asyncNotifyPending = PR_TRUE;
	
	PR_NotifyCondVar(thread->md.asyncIOCVar);
	PR_Unlock(thread->md.asyncIOLock);
	_PR_FAST_INTSON(is);
}


PR_IMPLEMENT(void) PR_Mac_PostAsyncNotify(PRThread *thread)
{
	_PRCPU *  cpu = _PR_MD_CURRENT_CPU();
	
	if (_PR_MD_GET_INTSOFF()) {
		thread->md.missedAsyncNotify = PR_TRUE;
		cpu->u.missed[cpu->where] |= _PR_MISSED_IO;
	} else {
		AsyncNotify(thread);
	}
}




#pragma mark -
#pragma mark PROCESS SUPPORT FUNCTIONS

PRProcess * _MD_CreateProcess(
    const char *path,
    char *const *argv,
    char *const *envp,
    const PRProcessAttr *attr)
{
#pragma unused (path, argv, envp, attr)

	PR_SetError(PR_NOT_IMPLEMENTED_ERROR, unimpErr);
	return NULL;
}

PRStatus _MD_DetachProcess(PRProcess *process)
{
#pragma unused (process)

	PR_SetError(PR_NOT_IMPLEMENTED_ERROR, unimpErr);
	return PR_FAILURE;
}

PRStatus _MD_WaitProcess(PRProcess *process, PRInt32 *exitCode)
{
#pragma unused (process, exitCode)

	PR_SetError(PR_NOT_IMPLEMENTED_ERROR, unimpErr);
	return PR_FAILURE;
}

PRStatus _MD_KillProcess(PRProcess *process)
{
#pragma unused (process)

	PR_SetError(PR_NOT_IMPLEMENTED_ERROR, unimpErr);
	return PR_FAILURE;
}



#pragma mark -
#pragma mark ATOMIC OPERATIONS

#ifdef _PR_HAVE_ATOMIC_OPS
PRInt32
_MD_AtomicSet(PRInt32 *val, PRInt32 newval)
{
    PRInt32 rv;
    do  {
        rv = *val;
    } while (!OTCompareAndSwap32(rv, newval, (UInt32*)val));

    return rv;
}

#endif 



#pragma mark -
#pragma mark INTERRUPT SUPPORT

#if TARGET_CARBON




























PRBool  gUseCriticalRegions;










PRInt32 gCriticalRegionEntryCount;


void _MD_SetIntsOff(PRInt32 ints)
{
    ENTER_CRITICAL_REGION();
    gCriticalRegionEntryCount ++;
    
    _pr_intsOff = ints;
    
    if (!ints)
    {
        PRInt32     i = gCriticalRegionEntryCount;

        gCriticalRegionEntryCount = 0;
        for ( ;i > 0; i --) {
            LEAVE_CRITICAL_REGION();
        }
    }
}


#endif 




#pragma mark -
#pragma mark CRITICAL REGION SUPPORT


static PRBool RunningOnOSX()
{
    long    systemVersion;
    OSErr   err = Gestalt(gestaltSystemVersion, &systemVersion);
    return (err == noErr) && (systemVersion >= 0x00001000);
}


#if MAC_CRITICAL_REGIONS

MDCriticalRegionID  gCriticalRegion;

void InitCriticalRegion()
{
    OSStatus    err;    
    
    
    gUseCriticalRegions = RunningOnOSX();
    if (!gUseCriticalRegions) return;
    
    err = MD_CriticalRegionCreate(&gCriticalRegion);
    PR_ASSERT(err == noErr);
}

void TermCriticalRegion()
{
    OSStatus    err;    

    if (!gUseCriticalRegions) return;

    err = MD_CriticalRegionDelete(gCriticalRegion);
    PR_ASSERT(err == noErr);
}


void EnterCritialRegion()
{
    OSStatus    err;
    
    if (!gUseCriticalRegions) return;

    PR_ASSERT(gCriticalRegion != kInvalidID);
    
    
    err = MD_CriticalRegionEnter(gCriticalRegion, kDurationForever  );
    PR_ASSERT(err == noErr);
}

void LeaveCritialRegion()
{
    OSStatus    err;    

    if (!gUseCriticalRegions) return;

    PR_ASSERT(gCriticalRegion != kInvalidID);

    err = MD_CriticalRegionExit(gCriticalRegion);
    PR_ASSERT(err == noErr);
}


#endif 



#pragma mark -
#pragma mark IDLE SEMAPHORE SUPPORT







#if TARGET_CARBON
PRBool					gUseIdleSemaphore = PR_FALSE;
MPSemaphoreID			gIdleSemaphore = NULL;
#endif

void InitIdleSemaphore()
{
    
#if TARGET_CARBON
	gUseIdleSemaphore = RunningOnOSX();
	if (gUseIdleSemaphore)
	{
		OSStatus  err = MPCreateSemaphore(1 , 0 , &gIdleSemaphore);
		PR_ASSERT(err == noErr);
	}
#endif
}

void TermIdleSemaphore()
{
#if TARGET_CARBON
	if (gUseIdleSemaphore)
	{
		OSStatus  err = MPDeleteSemaphore(gIdleSemaphore);
		PR_ASSERT(err == noErr);
		gUseIdleSemaphore = NULL;
	}
#endif
}


void WaitOnIdleSemaphore(PRIntervalTime timeout)
{
#if TARGET_CARBON
	if (gUseIdleSemaphore)
	{
		OSStatus  err = MPWaitOnSemaphore(gIdleSemaphore, kDurationMillisecond * PR_IntervalToMilliseconds(timeout));
		PR_ASSERT(err == noErr);
	}
	else
#endif
	{
		EventRecord   theEvent;
		






		(void)WaitNextEvent(nullEvent, &theEvent, 1, NULL);
	}
}


void SignalIdleSemaphore()
{
#if TARGET_CARBON
	if (gUseIdleSemaphore)
	{
		
		(void)MPSignalSemaphore(gIdleSemaphore);
	}
	else
#endif
	{
		WakeUpProcess(&gApplicationProcess);
	}
}



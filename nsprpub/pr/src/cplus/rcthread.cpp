






































#include "rcthread.h"
#include "rcinrval.h"

#include <prmem.h>
#include <prlog.h>
#include <stdio.h>
#include <prinit.h>

static RCPrimordialThread *primordial = NULL;

void nas_Root(void *arg)
{
    RCThread *him = (RCThread*)arg;
    while (RCThread::ex_unstarted == him->execution)
        (void)PR_Sleep(PR_INTERVAL_NO_TIMEOUT);  
    him->RootFunction();  
    if (PR_UNJOINABLE_THREAD == PR_GetThreadState(him->identity))
        delete him;
}  

RCThread::~RCThread() { }

RCThread::RCThread(): RCBase() { }

RCThread::RCThread(const RCThread&): RCBase()
{
    PR_NOT_REACHED("Cannot call thread copy constructor");
}  

RCThread::RCThread(
    RCThread::Scope scope, RCThread::State join, PRUint32 stackSize):
    RCBase()
{
    execution = ex_unstarted;
    identity = PR_CreateThread(
        PR_USER_THREAD, nas_Root, this,
        PR_GetThreadPriority(PR_GetCurrentThread()),
        (PRThreadScope)scope, (PRThreadState)join, stackSize);
}  

void RCThread::operator=(const RCThread&)
{
    PR_NOT_REACHED("Cannot call thread assignment operator");
}  


PRStatus RCThread::Start()
{
    PRStatus rv;
    
    if (RCThread::ex_unstarted == execution)
    {
        execution = RCThread::ex_started;
        rv = PR_Interrupt(identity);
        PR_ASSERT(PR_SUCCESS == rv);
    }
    else
    {
        rv = PR_FAILURE;
        PR_SetError(PR_INVALID_STATE_ERROR, 0);
    }
    return rv;
}  

PRStatus RCThread::Join()
{
    PRStatus rv;
    if (RCThread::ex_unstarted == execution)
    {
        rv = PR_FAILURE;
        PR_SetError(PR_INVALID_STATE_ERROR, 0);
    }
    else rv = PR_JoinThread(identity);
    if (PR_SUCCESS == rv) delete this;
    return rv;
}  

PRStatus RCThread::Interrupt()
{
    PRStatus rv;
    if (RCThread::ex_unstarted == execution)
    {
        rv = PR_FAILURE;
        PR_SetError(PR_INVALID_STATE_ERROR, 0);
    }
    else rv = PR_Interrupt(identity);
    return rv;
}  

void RCThread::ClearInterrupt() { PR_ClearInterrupt(); }

void RCThread::SetPriority(RCThread::Priority new_priority)
    { PR_SetThreadPriority(identity, (PRThreadPriority)new_priority); }

PRThread *RCThread::Self()
    { return PR_GetCurrentThread(); }

RCThread::Scope RCThread::GetScope() const
    { return (RCThread::Scope)PR_GetThreadScope(identity); }

RCThread::State RCThread::GetState() const
    { return (RCThread::State)PR_GetThreadState(identity); }

RCThread::Priority RCThread::GetPriority() const
    { return (RCThread::Priority)PR_GetThreadPriority(identity); }
    
static void _rc_PDDestructor(RCThreadPrivateData* privateData)
{
    PR_ASSERT(NULL != privateData);
    privateData->Release();
}

static PRThreadPrivateDTOR _tpd_dtor = (PRThreadPrivateDTOR)_rc_PDDestructor;

PRStatus RCThread::NewPrivateIndex(PRUintn* index)
    { return PR_NewThreadPrivateIndex(index, _tpd_dtor); }

PRStatus RCThread::SetPrivateData(PRUintn index)
    { return PR_SetThreadPrivate(index, NULL); }

PRStatus RCThread::SetPrivateData(PRUintn index, RCThreadPrivateData* data)
{
    return PR_SetThreadPrivate(index, data);
}

RCThreadPrivateData* RCThread::GetPrivateData(PRUintn index)
    { return (RCThreadPrivateData*)PR_GetThreadPrivate(index); }

PRStatus RCThread::Sleep(const RCInterval& ticks)
    { PRIntervalTime tmo = ticks; return PR_Sleep(tmo); }

RCPrimordialThread *RCThread::WrapPrimordialThread()
{
    





    if (NULL == primordial)
    {
        
        RCPrimordialThread *me = new RCPrimordialThread();
        PR_ASSERT(NULL != me);
        if (NULL == primordial)
        {
            primordial = me;
            me->execution = RCThread::ex_started;
            me->identity = PR_GetCurrentThread();
        }
        else delete me;  
    }
    return primordial;
}  

RCPrimordialThread::RCPrimordialThread(): RCThread() { }

RCPrimordialThread::~RCPrimordialThread() { }

void RCPrimordialThread::RootFunction()
{
    PR_NOT_REACHED("Primordial thread calling root function"); 
}  
 
PRStatus RCPrimordialThread::Cleanup() { return PR_Cleanup(); }

PRStatus RCPrimordialThread::SetVirtualProcessors(PRIntn count)
{
    PR_SetConcurrency(count);
    return PR_SUCCESS;
}  

RCThreadPrivateData::RCThreadPrivateData() { }

RCThreadPrivateData::RCThreadPrivateData(
    const RCThreadPrivateData& him) { }

RCThreadPrivateData::~RCThreadPrivateData() { }




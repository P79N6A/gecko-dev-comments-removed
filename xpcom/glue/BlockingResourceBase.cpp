






































#include "mozilla/BlockingResourceBase.h"

#ifdef DEBUG
#include "nsAutoPtr.h"

#include "mozilla/CondVar.h"
#include "mozilla/ReentrantMonitor.h"
#include "mozilla/Mutex.h"
#endif 

namespace mozilla {





const char* const BlockingResourceBase::kResourceTypeName[] =
{
    
    "Mutex", "ReentrantMonitor", "CondVar"
};

#ifdef DEBUG

PRCallOnceType BlockingResourceBase::sCallOnce;
PRUintn BlockingResourceBase::sResourceAcqnChainFrontTPI = (PRUintn)-1;
BlockingResourceBase::DDT* BlockingResourceBase::sDeadlockDetector;

bool
BlockingResourceBase::DeadlockDetectorEntry::Print(
    const DDT::ResourceAcquisition& aFirstSeen,
    nsACString& out,
    bool aPrintFirstSeenCx) const
{
    CallStack lastAcquisition = mAcquisitionContext; 
    bool maybeCurrentlyAcquired = (CallStack::kNone != lastAcquisition);
    CallStack printAcquisition =
        (aPrintFirstSeenCx || !maybeCurrentlyAcquired) ?
            aFirstSeen.mCallContext : lastAcquisition;

    fprintf(stderr, "--- %s : %s",
            kResourceTypeName[mType], mName);
    out += BlockingResourceBase::kResourceTypeName[mType];
    out += " : ";
    out += mName;

    if (maybeCurrentlyAcquired) {
        fputs(" (currently acquired)\n", stderr);
        out += " (currently acquired)\n";
    }

    fputs(" calling context\n", stderr);
    printAcquisition.Print(stderr);

    return maybeCurrentlyAcquired;
}


BlockingResourceBase::BlockingResourceBase(
    const char* aName,
    BlockingResourceBase::BlockingResourceType aType)
{
    
    
    if (PR_SUCCESS != PR_CallOnce(&sCallOnce, InitStatics))
        NS_RUNTIMEABORT("can't initialize blocking resource static members");

    mDDEntry = new BlockingResourceBase::DeadlockDetectorEntry(aName, aType);
    if (!mDDEntry)
        NS_RUNTIMEABORT("can't allocated deadlock detector entry");

    mChainPrev = 0;
    sDeadlockDetector->Add(mDDEntry);
}


BlockingResourceBase::~BlockingResourceBase()
{
    
    
    
    
    mChainPrev = 0;             
    mDDEntry = 0;               
}


void
BlockingResourceBase::CheckAcquire(const CallStack& aCallContext)
{
    if (eCondVar == mDDEntry->mType) {
        NS_NOTYETIMPLEMENTED(
            "FIXME bug 456272: annots. to allow CheckAcquire()ing condvars");
        return;
    }

    BlockingResourceBase* chainFront = ResourceChainFront();
    nsAutoPtr<DDT::ResourceAcquisitionArray> cycle(
        sDeadlockDetector->CheckAcquisition(
            chainFront ? chainFront->mDDEntry : 0, mDDEntry,
            aCallContext));
    if (!cycle)
        return;

    fputs("###!!! ERROR: Potential deadlock detected:\n", stderr);
    nsCAutoString out("Potential deadlock detected:\n");
    bool maybeImminent = PrintCycle(cycle, out);

    if (maybeImminent) {
        fputs("\n###!!! Deadlock may happen NOW!\n\n", stderr);
        out.Append("\n###!!! Deadlock may happen NOW!\n\n");
    } else {
        fputs("\nDeadlock may happen for some other execution\n\n",
              stderr);
        out.Append("\nDeadlock may happen for some other execution\n\n");
    }

    
    
    
    
    NS_ERROR(out.get());
}


void
BlockingResourceBase::Acquire(const CallStack& aCallContext)
{
    if (eCondVar == mDDEntry->mType) {
        NS_NOTYETIMPLEMENTED(
            "FIXME bug 456272: annots. to allow Acquire()ing condvars");
        return;
    }
    NS_ASSERTION(mDDEntry->mAcquisitionContext == CallStack::kNone,
                 "reacquiring already acquired resource");

    ResourceChainAppend(ResourceChainFront());
    mDDEntry->mAcquisitionContext = aCallContext;
}


void
BlockingResourceBase::Release()
{
    if (eCondVar == mDDEntry->mType) {
        NS_NOTYETIMPLEMENTED(
            "FIXME bug 456272: annots. to allow Release()ing condvars");
        return;
    }
      
    BlockingResourceBase* chainFront = ResourceChainFront();
    NS_ASSERTION(chainFront
                 && CallStack::kNone != mDDEntry->mAcquisitionContext,
                 "Release()ing something that hasn't been Acquire()ed");

    if (chainFront == this) {
        ResourceChainRemove();
    }
    else {
        
        NS_WARNING("Resource acquired at calling context\n");
        mDDEntry->mAcquisitionContext.Print(stderr);
        NS_WARNING("\nis being released in non-LIFO order; why?");
        
        
        
        
        
        
        BlockingResourceBase* curr = chainFront;
        BlockingResourceBase* prev = nsnull;
        while (curr && (prev = curr->mChainPrev) && (prev != this))
            curr = prev;
        if (prev == this)
            curr->mChainPrev = prev->mChainPrev;
    }

    mDDEntry->mAcquisitionContext = CallStack::kNone;
}


bool
BlockingResourceBase::PrintCycle(const DDT::ResourceAcquisitionArray* aCycle,
                                 nsACString& out)
{
    NS_ASSERTION(aCycle->Length() > 1, "need > 1 element for cycle!");

    bool maybeImminent = true;

    fputs("=== Cyclical dependency starts at\n", stderr);
    out += "Cyclical dependency starts at\n";

    const DDT::ResourceAcquisition res = aCycle->ElementAt(0);
    maybeImminent &= res.mResource->Print(res, out);

    DDT::ResourceAcquisitionArray::index_type i;
    DDT::ResourceAcquisitionArray::size_type len = aCycle->Length();
    const DDT::ResourceAcquisition* it = 1 + aCycle->Elements();
    for (i = 1; i < len - 1; ++i, ++it) {
        fputs("\n--- Next dependency:\n", stderr);
        out += "\nNext dependency:\n";

        maybeImminent &= it->mResource->Print(*it, out);
    }

    fputs("\n=== Cycle completed at\n", stderr);
    out += "Cycle completed at\n";
    it->mResource->Print(*it, out, true);

    return maybeImminent;
}




void
Mutex::Lock()
{
    CallStack callContext = CallStack();

    CheckAcquire(callContext);
    PR_Lock(mLock);
    Acquire(callContext);       
}

void
Mutex::Unlock()
{
    Release();                  
    PRStatus status = PR_Unlock(mLock);
    NS_ASSERTION(PR_SUCCESS == status, "bad Mutex::Unlock()");
}




void
ReentrantMonitor::Enter()
{
    BlockingResourceBase* chainFront = ResourceChainFront();

    

    if (this == chainFront) {
        
        PR_EnterMonitor(mReentrantMonitor);
        ++mEntryCount;
        return;
    }

    CallStack callContext = CallStack();
    
    
    
    if (chainFront) {
        for (BlockingResourceBase* br = ResourceChainPrev(chainFront);
             br;
             br = ResourceChainPrev(br)) {
            if (br == this) {
                NS_WARNING(
                    "Re-entering ReentrantMonitor after acquiring other resources.\n"
                    "At calling context\n");
                GetAcquisitionContext().Print(stderr);

                
                CheckAcquire(callContext);

                PR_EnterMonitor(mReentrantMonitor);
                ++mEntryCount;
                return;
            }
        }
    }

    CheckAcquire(callContext);
    PR_EnterMonitor(mReentrantMonitor);
    NS_ASSERTION(0 == mEntryCount, "ReentrantMonitor isn't free!");
    Acquire(callContext);       
    mEntryCount = 1;
}

void
ReentrantMonitor::Exit()
{
    if (0 == --mEntryCount)
        Release();              
    PRStatus status = PR_ExitMonitor(mReentrantMonitor);
    NS_ASSERTION(PR_SUCCESS == status, "bad ReentrantMonitor::Exit()");
}

nsresult
ReentrantMonitor::Wait(PRIntervalTime interval)
{
    AssertCurrentThreadIn();

    
    PRInt32 savedEntryCount = mEntryCount;
    CallStack savedAcquisitionContext = GetAcquisitionContext();
    BlockingResourceBase* savedChainPrev = mChainPrev;
    mEntryCount = 0;
    SetAcquisitionContext(CallStack::kNone);
    mChainPrev = 0;

    
    nsresult rv =
        PR_Wait(mReentrantMonitor, interval) == PR_SUCCESS ?
            NS_OK : NS_ERROR_FAILURE;
    
    
    mEntryCount = savedEntryCount;
    SetAcquisitionContext(savedAcquisitionContext);
    mChainPrev = savedChainPrev;

    return rv;
}




nsresult
CondVar::Wait(PRIntervalTime interval)
{
    AssertCurrentThreadOwnsMutex();

    
    CallStack savedAcquisitionContext = mLock->GetAcquisitionContext();
    BlockingResourceBase* savedChainPrev = mLock->mChainPrev;
    mLock->SetAcquisitionContext(CallStack::kNone);
    mLock->mChainPrev = 0;

    
    nsresult rv =
        PR_WaitCondVar(mCvar, interval) == PR_SUCCESS ?
            NS_OK : NS_ERROR_FAILURE;

    
    mLock->SetAcquisitionContext(savedAcquisitionContext);
    mLock->mChainPrev = savedChainPrev;

    return rv;
}

#endif 


} 

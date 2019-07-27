





#include "mozilla/BlockingResourceBase.h"

#ifdef DEBUG
#include "prthread.h"

#include "nsAutoPtr.h"

#ifndef MOZ_CALLSTACK_DISABLED
#include "CodeAddressService.h"
#include "nsHashKeys.h"
#include "mozilla/StackWalk.h"
#include "nsTHashtable.h"
#endif

#include "mozilla/CondVar.h"
#include "mozilla/DeadlockDetector.h"
#include "mozilla/ReentrantMonitor.h"
#include "mozilla/Mutex.h"

#if defined(MOZILLA_INTERNAL_API) && !defined(MOZILLA_XPCOMRT_API)
#include "GeckoProfiler.h"
#endif 

#endif 

namespace mozilla {





const char* const BlockingResourceBase::kResourceTypeName[] = {
  
  "Mutex", "ReentrantMonitor", "CondVar"
};

#ifdef DEBUG

PRCallOnceType BlockingResourceBase::sCallOnce;
unsigned BlockingResourceBase::sResourceAcqnChainFrontTPI = (unsigned)-1;
BlockingResourceBase::DDT* BlockingResourceBase::sDeadlockDetector;


void
BlockingResourceBase::StackWalkCallback(uint32_t aFrameNumber, void* aPc,
                                        void* aSp, void* aClosure)
{
#ifndef MOZ_CALLSTACK_DISABLED
  AcquisitionState* state = (AcquisitionState*)aClosure;
  state->AppendElement(aPc);
#endif
}

void
BlockingResourceBase::GetStackTrace(AcquisitionState& aState)
{
#ifndef MOZ_CALLSTACK_DISABLED
  
  const uint32_t kSkipFrames = 2;

  aState.Clear();

  
  
  MozStackWalk(StackWalkCallback, kSkipFrames, 24, &aState, 0, nullptr);
#endif
}















bool
PrintCycle(const BlockingResourceBase::DDT::ResourceAcquisitionArray* aCycle,
           nsACString& aOut)
{
  NS_ASSERTION(aCycle->Length() > 1, "need > 1 element for cycle!");

  bool maybeImminent = true;

  fputs("=== Cyclical dependency starts at\n", stderr);
  aOut += "Cyclical dependency starts at\n";

  const BlockingResourceBase::DDT::ResourceAcquisitionArray::elem_type res =
    aCycle->ElementAt(0);
  maybeImminent &= res->Print(aOut);

  BlockingResourceBase::DDT::ResourceAcquisitionArray::index_type i;
  BlockingResourceBase::DDT::ResourceAcquisitionArray::size_type len =
    aCycle->Length();
  const BlockingResourceBase::DDT::ResourceAcquisitionArray::elem_type* it =
    1 + aCycle->Elements();
  for (i = 1; i < len - 1; ++i, ++it) {
    fputs("\n--- Next dependency:\n", stderr);
    aOut += "\nNext dependency:\n";

    maybeImminent &= (*it)->Print(aOut);
  }

  fputs("\n=== Cycle completed at\n", stderr);
  aOut += "Cycle completed at\n";
  (*it)->Print(aOut);

  return maybeImminent;
}

#ifndef MOZ_CALLSTACK_DISABLED
struct CodeAddressServiceLock final
{
  static void Unlock() { }
  static void Lock() { }
  static bool IsLocked() { return true; }
};

struct CodeAddressServiceStringAlloc final
{
  static char* copy(const char* aString) { return ::strdup(aString); }
  static void free(char* aString) { ::free(aString); }
};

class CodeAddressServiceStringTable final
{
public:
  CodeAddressServiceStringTable() : mSet(32) {}

  const char* Intern(const char* aString)
  {
    nsCharPtrHashKey* e = mSet.PutEntry(aString);
    return e->GetKey();
  }

  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    return mSet.SizeOfExcludingThis(aMallocSizeOf);
  }

private:
  typedef nsTHashtable<nsCharPtrHashKey> StringSet;
  StringSet mSet;
};

typedef CodeAddressService<CodeAddressServiceStringTable,
                           CodeAddressServiceStringAlloc,
                           CodeAddressServiceLock> WalkTheStackCodeAddressService;
#endif

bool
BlockingResourceBase::Print(nsACString& aOut) const
{
  fprintf(stderr, "--- %s : %s",
          kResourceTypeName[mType], mName);
  aOut += BlockingResourceBase::kResourceTypeName[mType];
  aOut += " : ";
  aOut += mName;

  bool acquired = IsAcquired();

  if (acquired) {
    fputs(" (currently acquired)\n", stderr);
    aOut += " (currently acquired)\n";
  }

  fputs(" calling context\n", stderr);
#ifdef MOZ_CALLSTACK_DISABLED
  fputs("  [stack trace unavailable]\n", stderr);
#else
  const AcquisitionState& state = acquired ? mAcquired : mFirstSeen;

  WalkTheStackCodeAddressService addressService;

  for (uint32_t i = 0; i < state.Length(); i++) {
    const size_t kMaxLength = 1024;
    char buffer[kMaxLength];
    addressService.GetLocation(i + 1, state[i], buffer, kMaxLength);
    const char* fmt = "    %s\n";
    aOut.AppendLiteral("    ");
    aOut.Append(buffer);
    aOut.AppendLiteral("\n");
    fprintf(stderr, fmt, buffer);
  }

#endif

  return acquired;
}


BlockingResourceBase::BlockingResourceBase(
    const char* aName,
    BlockingResourceBase::BlockingResourceType aType)
  : mName(aName)
  , mType(aType)
#ifdef MOZ_CALLSTACK_DISABLED
  , mAcquired(false)
#else
  , mAcquired()
#endif
{
  MOZ_ASSERT(mName, "Name must be nonnull");
  
  
  if (PR_SUCCESS != PR_CallOnce(&sCallOnce, InitStatics)) {
    NS_RUNTIMEABORT("can't initialize blocking resource static members");
  }

  mChainPrev = 0;
  sDeadlockDetector->Add(this);
}


BlockingResourceBase::~BlockingResourceBase()
{
  
  
  
  
  mChainPrev = 0;             
  if (sDeadlockDetector) {
    sDeadlockDetector->Remove(this);
  }
}


size_t
BlockingResourceBase::SizeOfDeadlockDetector(MallocSizeOf aMallocSizeOf)
{
  return sDeadlockDetector ?
      sDeadlockDetector->SizeOfIncludingThis(aMallocSizeOf) : 0;
}


PRStatus
BlockingResourceBase::InitStatics()
{
  PR_NewThreadPrivateIndex(&sResourceAcqnChainFrontTPI, 0);
  sDeadlockDetector = new DDT();
  if (!sDeadlockDetector) {
    NS_RUNTIMEABORT("can't allocate deadlock detector");
  }
  return PR_SUCCESS;
}


void
BlockingResourceBase::Shutdown()
{
  delete sDeadlockDetector;
  sDeadlockDetector = 0;
}


void
BlockingResourceBase::CheckAcquire()
{
  if (mType == eCondVar) {
    NS_NOTYETIMPLEMENTED(
      "FIXME bug 456272: annots. to allow CheckAcquire()ing condvars");
    return;
  }

  BlockingResourceBase* chainFront = ResourceChainFront();
  nsAutoPtr<DDT::ResourceAcquisitionArray> cycle(
    sDeadlockDetector->CheckAcquisition(
      chainFront ? chainFront : 0, this));
  if (!cycle) {
    return;
  }

#ifndef MOZ_CALLSTACK_DISABLED
  
  GetStackTrace(mAcquired);
#endif

  fputs("###!!! ERROR: Potential deadlock detected:\n", stderr);
  nsAutoCString out("Potential deadlock detected:\n");
  bool maybeImminent = PrintCycle(cycle, out);

  if (maybeImminent) {
    fputs("\n###!!! Deadlock may happen NOW!\n\n", stderr);
    out.AppendLiteral("\n###!!! Deadlock may happen NOW!\n\n");
  } else {
    fputs("\nDeadlock may happen for some other execution\n\n",
          stderr);
    out.AppendLiteral("\nDeadlock may happen for some other execution\n\n");
  }

  
  
  
  
  NS_ERROR(out.get());
}


void
BlockingResourceBase::Acquire()
{
  if (mType == eCondVar) {
    NS_NOTYETIMPLEMENTED(
      "FIXME bug 456272: annots. to allow Acquire()ing condvars");
    return;
  }
  NS_ASSERTION(!IsAcquired(),
               "reacquiring already acquired resource");

  ResourceChainAppend(ResourceChainFront());

#ifdef MOZ_CALLSTACK_DISABLED
  mAcquired = true;
#else
  
  GetStackTrace(mAcquired);
  if (mFirstSeen.IsEmpty()) {
    mFirstSeen = mAcquired;
  }
#endif
}


void
BlockingResourceBase::Release()
{
  if (mType == eCondVar) {
    NS_NOTYETIMPLEMENTED(
      "FIXME bug 456272: annots. to allow Release()ing condvars");
    return;
  }

  BlockingResourceBase* chainFront = ResourceChainFront();
  NS_ASSERTION(chainFront && IsAcquired(),
               "Release()ing something that hasn't been Acquire()ed");

  if (chainFront == this) {
    ResourceChainRemove();
  } else {
    
    NS_WARNING("Resource acquired at calling context\n");
    NS_WARNING("  [stack trace unavailable]\n");
    NS_WARNING("\nis being released in non-LIFO order; why?");

    
    
    
    
    
    BlockingResourceBase* curr = chainFront;
    BlockingResourceBase* prev = nullptr;
    while (curr && (prev = curr->mChainPrev) && (prev != this)) {
      curr = prev;
    }
    if (prev == this) {
      curr->mChainPrev = prev->mChainPrev;
    }
  }

  ClearAcquisitionState();
}




void
OffTheBooksMutex::Lock()
{
  CheckAcquire();
  PR_Lock(mLock);
  Acquire();       
}

void
OffTheBooksMutex::Unlock()
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

  
  
  if (chainFront) {
    for (BlockingResourceBase* br = ResourceChainPrev(chainFront);
         br;
         br = ResourceChainPrev(br)) {
      if (br == this) {
        NS_WARNING(
          "Re-entering ReentrantMonitor after acquiring other resources.\n"
          "At calling context\n"
          "  [stack trace unavailable]\n");

        
        CheckAcquire();

        PR_EnterMonitor(mReentrantMonitor);
        ++mEntryCount;
        return;
      }
    }
  }

  CheckAcquire();
  PR_EnterMonitor(mReentrantMonitor);
  NS_ASSERTION(mEntryCount == 0, "ReentrantMonitor isn't free!");
  Acquire();       
  mEntryCount = 1;
}

void
ReentrantMonitor::Exit()
{
  if (--mEntryCount == 0) {
    Release();              
  }
  PRStatus status = PR_ExitMonitor(mReentrantMonitor);
  NS_ASSERTION(PR_SUCCESS == status, "bad ReentrantMonitor::Exit()");
}

nsresult
ReentrantMonitor::Wait(PRIntervalTime aInterval)
{
  AssertCurrentThreadIn();

  
  int32_t savedEntryCount = mEntryCount;
  AcquisitionState savedAcquisitionState = GetAcquisitionState();
  BlockingResourceBase* savedChainPrev = mChainPrev;
  mEntryCount = 0;
  ClearAcquisitionState();
  mChainPrev = 0;

  nsresult rv;
#if defined(MOZILLA_INTERNAL_API) && !defined(MOZILLA_XPCOMRT_API)
  {
    GeckoProfilerSleepRAII profiler_sleep;
#endif


    rv = PR_Wait(mReentrantMonitor, aInterval) == PR_SUCCESS ? NS_OK :
                                                               NS_ERROR_FAILURE;

#if defined(MOZILLA_INTERNAL_API) && !defined(MOZILLA_XPCOMRT_API)
  }
#endif 

  
  mEntryCount = savedEntryCount;
  SetAcquisitionState(savedAcquisitionState);
  mChainPrev = savedChainPrev;

  return rv;
}




nsresult
CondVar::Wait(PRIntervalTime aInterval)
{
  AssertCurrentThreadOwnsMutex();

  
  AcquisitionState savedAcquisitionState = mLock->GetAcquisitionState();
  BlockingResourceBase* savedChainPrev = mLock->mChainPrev;
  mLock->ClearAcquisitionState();
  mLock->mChainPrev = 0;

  
  nsresult rv =
    PR_WaitCondVar(mCvar, aInterval) == PR_SUCCESS ? NS_OK : NS_ERROR_FAILURE;

  
  mLock->SetAcquisitionState(savedAcquisitionState);
  mLock->mChainPrev = savedChainPrev;

  return rv;
}

#endif 


} 

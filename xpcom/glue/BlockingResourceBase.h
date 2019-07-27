






#ifndef mozilla_BlockingResourceBase_h
#define mozilla_BlockingResourceBase_h

#include "prlog.h"

#include "nscore.h"
#include "nsDebug.h"
#include "nsError.h"
#include "nsISupportsImpl.h"

#ifdef DEBUG


#define MOZ_CALLSTACK_DISABLED

#include "prinit.h"

#include "nsStringGlue.h"

#ifndef MOZ_CALLSTACK_DISABLED
#include "nsTArray.h"
#endif

#include "nsXPCOM.h"
#endif





namespace mozilla {

#ifdef DEBUG
template <class T> class DeadlockDetector;
#endif






class BlockingResourceBase
{
public:
  
  enum BlockingResourceType { eMutex, eReentrantMonitor, eCondVar };

  



  static const char* const kResourceTypeName[];


#ifdef DEBUG

  static size_t
  SizeOfDeadlockDetector(MallocSizeOf aMallocSizeOf);

  
















  bool Print(nsACString& aOut) const;

  size_t
  SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
  {
    
    
    
    size_t n = aMallocSizeOf(this);
    return n;
  }

  
  typedef DeadlockDetector<BlockingResourceBase> DDT;

protected:
#ifdef MOZ_CALLSTACK_DISABLED
  typedef bool AcquisitionState;
#else
  typedef nsAutoTArray<void*, 24> AcquisitionState;
#endif

  










  BlockingResourceBase(const char* aName, BlockingResourceType aType);

  ~BlockingResourceBase();

  




  void CheckAcquire();

  




  void Acquire(); 

  








  void Release();             

  







  static BlockingResourceBase* ResourceChainFront()
  {
    return
      (BlockingResourceBase*)PR_GetThreadPrivate(sResourceAcqnChainFrontTPI);
  }

  




  static BlockingResourceBase* ResourceChainPrev(
      const BlockingResourceBase* aResource)
  {
    return aResource->mChainPrev;
  } 

  






  void ResourceChainAppend(BlockingResourceBase* aPrev)
  {
    mChainPrev = aPrev;
    PR_SetThreadPrivate(sResourceAcqnChainFrontTPI, this);
  } 

  





  void ResourceChainRemove()
  {
    NS_ASSERTION(this == ResourceChainFront(), "not at chain front");
    PR_SetThreadPrivate(sResourceAcqnChainFrontTPI, mChainPrev);
  } 

  





  AcquisitionState GetAcquisitionState()
  {
    return mAcquired;
  }

  





  void SetAcquisitionState(const AcquisitionState& aAcquisitionState)
  {
    mAcquired = aAcquisitionState;
  }

  





  void ClearAcquisitionState()
  {
#ifdef MOZ_CALLSTACK_DISABLED
    mAcquired = false;
#else
    mAcquired.Clear();
#endif
  }

  





  bool IsAcquired() const
  {
#ifdef MOZ_CALLSTACK_DISABLED
    return mAcquired;
#else
    return !mAcquired.IsEmpty();
#endif
  }

  





  BlockingResourceBase* mChainPrev;

private:
  




  const char* mName;

  




  BlockingResourceType mType;

  



  AcquisitionState mAcquired;

#ifndef MOZ_CALLSTACK_DISABLED
  



  AcquisitionState mFirstSeen;
#endif

  




  static PRCallOnceType sCallOnce;

  




  static unsigned sResourceAcqnChainFrontTPI;

  



  static DDT* sDeadlockDetector;

  






  static PRStatus InitStatics();

  





  static void Shutdown();

  static void StackWalkCallback(void* aPc, void* aSp, void* aClosure);
  static void GetStackTrace(AcquisitionState& aState);

#  ifdef MOZILLA_INTERNAL_API
  
  friend void LogTerm();
#  endif  

#else  

  BlockingResourceBase(const char* aName, BlockingResourceType aType) {}

  ~BlockingResourceBase() {}

#endif
};


} 


#endif

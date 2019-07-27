






#ifndef mozilla_BlockingResourceBase_h
#define mozilla_BlockingResourceBase_h

#include "prlog.h"

#include "nscore.h"
#include "nsDebug.h"
#include "nsError.h"
#include "nsISupportsImpl.h"

#ifdef DEBUG
#include "prinit.h"
#include "prthread.h"

#include "nsStringGlue.h"

#include "mozilla/DeadlockDetector.h"
#include "nsXPCOM.h"
#endif





namespace mozilla {







class NS_COM_GLUE BlockingResourceBase
{
public:
  
  enum BlockingResourceType { eMutex, eReentrantMonitor, eCondVar };

  



  static const char* const kResourceTypeName[];


#ifdef DEBUG

  static size_t
  SizeOfDeadlockDetector(MallocSizeOf aMallocSizeOf)
  {
    return sDeadlockDetector ?
        sDeadlockDetector->SizeOfIncludingThis(aMallocSizeOf) : 0;
  }

  
















  bool Print(nsACString& aOut) const;

  size_t
  SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
  {
    
    
    
    size_t n = aMallocSizeOf(this);
    return n;
  }

  
  typedef DeadlockDetector<BlockingResourceBase> DDT;

protected:
  










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

  





  bool GetAcquisitionState()
  {
    return mAcquired;
  }

  





  void SetAcquisitionState(bool aAcquisitionState)
  {
    mAcquired = aAcquisitionState;
  }

  





  BlockingResourceBase* mChainPrev;

private:
  




  const char* mName;

  




  BlockingResourceType mType;

  



  bool mAcquired;

  




  static PRCallOnceType sCallOnce;

  




  static unsigned sResourceAcqnChainFrontTPI;

  



  static DDT* sDeadlockDetector;

  






  static PRStatus InitStatics()
  {
    PR_NewThreadPrivateIndex(&sResourceAcqnChainFrontTPI, 0);
    sDeadlockDetector = new DDT();
    if (!sDeadlockDetector) {
      NS_RUNTIMEABORT("can't allocate deadlock detector");
    }
    return PR_SUCCESS;
  }

  





  static void Shutdown()
  {
    delete sDeadlockDetector;
    sDeadlockDetector = 0;
  }

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

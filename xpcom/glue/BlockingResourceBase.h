






#ifndef mozilla_BlockingResourceBase_h
#define mozilla_BlockingResourceBase_h

#include "prlog.h"

#include "nscore.h"
#include "nsDebug.h"
#include "nsError.h"
#include "nsISupportsImpl.h"

#ifdef DEBUG
#include "prinit.h"

#include "nsStringGlue.h"
#include "nsXPCOM.h"
#endif





namespace mozilla {

#ifdef DEBUG
template <class T> class DeadlockDetector;
#endif






class NS_COM_GLUE BlockingResourceBase
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
  typedef bool AcquisitionState;

  










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

  





  BlockingResourceBase* mChainPrev;

private:
  




  const char* mName;

  




  BlockingResourceType mType;

  



  AcquisitionState mAcquired;

  




  static PRCallOnceType sCallOnce;

  




  static unsigned sResourceAcqnChainFrontTPI;

  



  static DDT* sDeadlockDetector;

  






  static PRStatus InitStatics();

  





  static void Shutdown();

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

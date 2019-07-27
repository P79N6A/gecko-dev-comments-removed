



#ifndef mozilla_IOInterposer_h
#define mozilla_IOInterposer_h

#include "mozilla/Attributes.h"
#include "mozilla/TimeStamp.h"

namespace mozilla {





class IOInterposeObserver
{
public:
  enum Operation
  {
    OpNone = 0,
    OpCreateOrOpen = (1 << 0),
    OpRead = (1 << 1),
    OpWrite = (1 << 2),
    OpFSync = (1 << 3),
    OpStat = (1 << 4),
    OpClose = (1 << 5),
    OpNextStage = (1 << 6), 
    OpWriteFSync = (OpWrite | OpFSync),
    OpAll = (OpCreateOrOpen | OpRead | OpWrite | OpFSync | OpStat | OpClose),
    OpAllWithStaging = (OpAll | OpNextStage)
  };

  
  class Observation
  {
  protected:
    








    Observation(Operation aOperation, const char* aReference,
                bool aShouldReport = true);

  public:
    



    Observation(Operation aOperation, const TimeStamp& aStart,
                const TimeStamp& aEnd, const char* aReference);

    



    Operation ObservedOperation() const { return mOperation; }

    


    const char* ObservedOperationString() const;

    
    TimeStamp Start() const { return mStart; }

    



    TimeStamp End() const { return mEnd; }

    



    TimeDuration Duration() const { return mEnd - mStart; }

    






    const char* Reference() const { return mReference; }

    
    virtual const char16_t* Filename() { return nullptr; }

    virtual ~Observation() {}

  protected:
    void
    Report();

    Operation   mOperation;
    TimeStamp   mStart;
    TimeStamp   mEnd;
    const char* mReference;     
    bool        mShouldReport;  
  };

  









  virtual void Observe(Observation& aObservation) = 0;

  virtual ~IOInterposeObserver() {}

protected:
  




  static bool IsMainThread();
};





namespace IOInterposer {













bool Init();












void Clear();





void Disable();





















void Report(IOInterposeObserver::Observation& aObservation);






bool IsObservedOperation(IOInterposeObserver::Operation aOp);







void Register(IOInterposeObserver::Operation aOp,
              IOInterposeObserver* aObserver);









void Unregister(IOInterposeObserver::Operation aOp,
                IOInterposeObserver* aObserver);












void RegisterCurrentThread(bool aIsMainThread = false);






void UnregisterCurrentThread();





void EnteringNextStage();

} 

class IOInterposerInit
{
public:
  IOInterposerInit()
  {
#if defined(MOZ_ENABLE_PROFILER_SPS)
    IOInterposer::Init();
#endif
  }

  ~IOInterposerInit()
  {
#if defined(MOZ_ENABLE_PROFILER_SPS)
    IOInterposer::Clear();
#endif
  }
};

} 

#endif

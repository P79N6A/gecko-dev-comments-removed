



#ifndef mozilla_IOInterposer_h
#define mozilla_IOInterposer_h

#include "mozilla/Attributes.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/XPCOM.h"

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
    OpWriteFSync = (OpWrite | OpFSync),
    OpAll = (OpCreateOrOpen | OpRead | OpWrite | OpFSync | OpStat | OpClose)
  };

  
  class Observation
  {
  protected:
    








    Observation(Operation aOperation, const char* aReference,
                bool aShouldReport = true);

  public:
    



    Observation(Operation aOperation, const TimeStamp& aStart,
                const TimeStamp& aEnd, const char* aReference);

    



    Operation ObservedOperation() const
    {
      return mOperation;
    }

    
    TimeStamp Start() const
    {
      return mStart;
    }

    



    TimeStamp End() const
    {
      return mEnd;
    }

    



    TimeDuration Duration() const
    {
      return mEnd - mStart;
    }

    






    const char* Reference() const
    {
      return mReference;
    }

    
    virtual const char16_t* Filename()
    {
      return nullptr;
    }

    virtual ~Observation()
    {
    }

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

  virtual ~IOInterposeObserver()
  {
  }

protected:
  




  static bool IsMainThread();
};

#ifdef MOZ_ENABLE_PROFILER_SPS














class IOInterposer MOZ_FINAL
{
  
  static IOInterposeObserver::Operation sObservedOperations;

  
  IOInterposer();
public:

  









  static void Init();

  










  static void Clear();

  



  static void Disable();

  



















  static void Report(IOInterposeObserver::Observation& aObservation);

  




  static bool IsObservedOperation(IOInterposeObserver::Operation aOp);

  





  static void Register(IOInterposeObserver::Operation aOp,
                       IOInterposeObserver* aObserver);

  







  static void Unregister(IOInterposeObserver::Operation aOp,
                         IOInterposeObserver* aObserver);

  





  static void
  RegisterCurrentThread(bool aIsMainThread = false);
};

#else 

class IOInterposer MOZ_FINAL
{
  IOInterposer();
public:
  static inline void Init()                                               {}
  static inline void Clear()                                              {}
  static inline void Disable()                                            {}
  static inline void Report(IOInterposeObserver::Observation& aOb)        {}
  static inline void Register(IOInterposeObserver::Operation aOp,
                              IOInterposeObserver* aObserver)             {}
  static inline void Unregister(IOInterposeObserver::Operation aOp,
                                IOInterposeObserver* aObserver)           {}
  static inline bool IsObservedOperation(IOInterposeObserver::Operation aOp) {
    return false;
  }
  static inline void RegisterCurrentThread(bool)                          {}
};

#endif 

class IOInterposerInit
{
public:
  IOInterposerInit()
  {
    IOInterposer::Init();
  }

  
  
};

} 

#endif

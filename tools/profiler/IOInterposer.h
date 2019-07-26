



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
    OpRead = (1 << 0),
    OpWrite = (1 << 1),
    OpFSync = (1 << 2),
    OpWriteFSync = (OpWrite | OpFSync),
    OpAll = (OpRead | OpWrite | OpFSync)
  };

  
  class Observation
  {
  protected:
    Observation()
    {
    }
  public:
    Observation(Operation aOperation, const TimeStamp& aStart,
                const TimeStamp& aEnd, const char* aReference)
     : mOperation(aOperation), mStart(aStart), mEnd(aEnd),
       mReference(aReference)
    {
    }

    



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

    
    virtual const char* Filename()
    {
      return nullptr;
    }

    virtual ~Observation()
    {
    }
  protected:
    Operation   mOperation;
    TimeStamp   mStart;
    TimeStamp   mEnd;
    const char* mReference;
  };

  









  virtual void Observe(Observation& aObservation) = 0;

  virtual ~IOInterposeObserver()
  {
  }
};

#ifdef MOZ_ENABLE_PROFILER_SPS














class IOInterposer MOZ_FINAL
{
  
  static IOInterposeObserver::Operation sObservedOperations;

  
  IOInterposer();
public:

  









  static void Init();

  










  static void Clear();

  



















  static void Report(IOInterposeObserver::Observation& aObservation);

  




  static inline bool IsObservedOperation(IOInterposeObserver::Operation aOp) {
    
    
    
    
    
    return (sObservedOperations & aOp);
  }

  





  static void Register(IOInterposeObserver::Operation aOp,
                       IOInterposeObserver* aObserver);

  







  static void Unregister(IOInterposeObserver::Operation aOp,
                         IOInterposeObserver* aObserver);
};

#else 

class IOInterposer MOZ_FINAL
{
  IOInterposer();
public:
  static inline void Init()                                               {}
  static inline void Clear()                                              {}
  static inline void Report(IOInterposeObserver::Observation& aOb)        {}
  static inline void Register(IOInterposeObserver::Operation aOp,
                              IOInterposeObserver* aObserver)             {}
  static inline void Unregister(IOInterposeObserver::Operation aOp,
                                IOInterposeObserver* aObserver)           {}
  static inline bool IsObservedOperation(IOInterposeObserver::Operation aOp) {
    return false;
  }
};

#endif 

} 

#endif

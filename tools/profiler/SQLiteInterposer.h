



#ifndef SQLITEINTERPOSER_H_
#define SQLITEINTERPOSER_H_

#ifdef MOZ_ENABLE_PROFILER_SPS

#include "IOInterposer.h"
#include "mozilla/Atomics.h"

namespace mozilla {









class SQLiteInterposer MOZ_FINAL : public IOInterposerModule
{
public:
  static IOInterposerModule* GetInstance(IOInterposeObserver* aObserver, 
      IOInterposeObserver::Operation aOpsToInterpose);

  





  static void ClearInstance();

  ~SQLiteInterposer();
  void Enable(bool aEnable);

  typedef void (*EventHandlerFn)(double&);
  static void OnRead(double& aDuration);
  static void OnWrite(double& aDuration);
  static void OnFSync(double& aDuration);

private:
  SQLiteInterposer();
  bool Init(IOInterposeObserver* aObserver,
            IOInterposeObserver::Operation aOpsToInterpose);

  IOInterposeObserver*            mObserver;
  IOInterposeObserver::Operation  mOps;
  Atomic<uint32_t,ReleaseAcquire> mEnabled;
};

} 

#else 

namespace mozilla {

class SQLiteInterposer MOZ_FINAL
{
public:
  typedef void (*EventHandlerFn)(double&);
  static inline void OnRead(double& aDuration) {}
  static inline void OnWrite(double& aDuration) {}
  static inline void OnFSync(double& aDuration) {}
};

} 

#endif 

#endif 

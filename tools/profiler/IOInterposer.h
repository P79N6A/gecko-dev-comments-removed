



#ifndef IOINTERPOSER_H_
#define IOINTERPOSER_H_

#include "mozilla/Attributes.h"
#include "mozilla/Mutex.h"
#include "mozilla/StaticPtr.h"
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
  virtual void Observe(Operation aOp, double& aDuration,
                       const char* aModuleInfo) = 0;
};





class IOInterposerModule
{
public:
  virtual ~IOInterposerModule() {}
  virtual void Enable(bool aEnable) = 0;

protected:
  IOInterposerModule() {}

private:
  IOInterposerModule(const IOInterposerModule&);
  IOInterposerModule& operator=(const IOInterposerModule&);
};







class IOInterposer MOZ_FINAL : public IOInterposeObserver
{
public:
  ~IOInterposer();

  




  void Enable(bool aEnable);

  void Register(IOInterposeObserver::Operation aOp,
                IOInterposeObserver* aObserver);
  void Deregister(IOInterposeObserver::Operation aOp,
                  IOInterposeObserver* aObserver);
  void Observe(IOInterposeObserver::Operation aOp, double& aDuration,
               const char* aModuleInfo);

  static IOInterposer* GetInstance();

  




  static void ClearInstance();

private:
  IOInterposer();
  bool Init();
  IOInterposer(const IOInterposer&);
  IOInterposer& operator=(const IOInterposer&);

  
  
  mozilla::Mutex  mMutex;
  nsTArray<IOInterposerModule*>   mModules;
  nsTArray<IOInterposeObserver*>  mReadObservers;
  nsTArray<IOInterposeObserver*>  mWriteObservers;
  nsTArray<IOInterposeObserver*>  mFSyncObservers;
};

} 

#endif


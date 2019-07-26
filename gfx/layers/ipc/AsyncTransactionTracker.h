






#ifndef mozilla_layers_AsyncTransactionTracker_h
#define mozilla_layers_AsyncTransactionTracker_h

#include <map>

#include "mozilla/Atomics.h"
#include "mozilla/Monitor.h"      
#include "mozilla/RefPtr.h"       

namespace mozilla {
namespace layers {

class TextureClient;





class AsyncTransactionTracker
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AsyncTransactionTracker)

  AsyncTransactionTracker();

  static void Initialize()
  {
    if (!sLock) {
      sLock = new Mutex("AsyncTransactionTracker::sLock");
    }
  }

  static void Finalize()
  {
    if (sLock) {
      delete sLock;
      sLock = nullptr;
    }
  }

  Monitor& GetReentrantMonitor()
  {
    return mCompletedMonitor;
  }

  


  void WaitComplete();

  


  void NotifyComplete();

  


  void NotifyCancel();

  uint64_t GetId()
  {
    return mSerial;
  }

  


  virtual void Complete()= 0;

  



  virtual void Cancel()= 0;

  virtual void SetTextureClient(TextureClient* aTextureClient) {}

protected:
  virtual ~AsyncTransactionTracker();

  static uint64_t GetNextSerial()
  {
    MOZ_ASSERT(sLock);
    if(sLock) {
      sLock->Lock();
    }
    ++sSerialCounter;
    if(sLock) {
      sLock->Unlock();
    }
    return sSerialCounter;
  }

  uint64_t mSerial;
  Monitor mCompletedMonitor;
  bool    mCompleted;

  



  static uint64_t sSerialCounter;
  static Mutex* sLock;
};

class AsyncTransactionTrackersHolder
{
public:
  AsyncTransactionTrackersHolder();
  virtual ~AsyncTransactionTrackersHolder();

  void HoldUntilComplete(AsyncTransactionTracker* aTransactionTracker);

  void TransactionCompleteted(uint64_t aTransactionId);

protected:
  void ClearAllAsyncTransactionTrackers();

  void DestroyAsyncTransactionTrackersHolder();

  bool mIsTrackersHolderDestroyed;
  std::map<uint64_t, RefPtr<AsyncTransactionTracker> > mAsyncTransactionTrackeres;

};

} 
} 

#endif  

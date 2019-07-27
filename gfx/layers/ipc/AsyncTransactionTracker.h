






#ifndef mozilla_layers_AsyncTransactionTracker_h
#define mozilla_layers_AsyncTransactionTracker_h

#include <map>

#include "mozilla/Atomics.h"
#include "mozilla/layers/FenceUtils.h"  
#include "mozilla/Monitor.h"      
#include "mozilla/RefPtr.h"       

namespace mozilla {
namespace layers {

class TextureClient;
class AsyncTransactionTrackersHolder;





class AsyncTransactionTracker
{
  friend class AsyncTransactionTrackersHolder;
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AsyncTransactionTracker)

  AsyncTransactionTracker();

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

  virtual void SetReleaseFenceHandle(FenceHandle& aReleaseFenceHandle) {}

protected:
  virtual ~AsyncTransactionTracker();

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

  static uint64_t GetNextSerial()
  {
    MOZ_ASSERT(sLock);
    MutexAutoLock lock(*sLock);
    ++sSerialCounter;
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

  static void Initialize()
  {
    if (!sHolderLock) {
      sHolderLock = new Mutex("AsyncTransactionTrackersHolder::sHolderLock");
    }
    AsyncTransactionTracker::Initialize();
  }

  static void Finalize()
  {
    if (sHolderLock) {
      delete sHolderLock;
      sHolderLock = nullptr;
    }
    AsyncTransactionTracker::Finalize();
  }

  void HoldUntilComplete(AsyncTransactionTracker* aTransactionTracker);

  void TransactionCompleteted(uint64_t aTransactionId);

  static void TransactionCompleteted(uint64_t aHolderId, uint64_t aTransactionId);

  static void SetReleaseFenceHandle(FenceHandle& aReleaseFenceHandle,
                                    uint64_t aHolderId,
                                    uint64_t aTransactionId);

  uint64_t GetId()
  {
    return mSerial;
  }

protected:

  static uint64_t GetNextSerial()
  {
    MOZ_ASSERT(sHolderLock);
    MutexAutoLock lock(*sHolderLock);
    ++sSerialCounter;
    return sSerialCounter;
  }

  void TransactionCompletetedInternal(uint64_t aTransactionId);

  void SetReleaseFenceHandle(FenceHandle& aReleaseFenceHandle, uint64_t aTransactionId);

  void ClearAllAsyncTransactionTrackers();

  void DestroyAsyncTransactionTrackersHolder();

  uint64_t mSerial;

  bool mIsTrackersHolderDestroyed;
  std::map<uint64_t, RefPtr<AsyncTransactionTracker> > mAsyncTransactionTrackeres;

  



  static uint64_t sSerialCounter;
  static Mutex* sHolderLock;

  


  static std::map<uint64_t, AsyncTransactionTrackersHolder*> sTrackersHolders;
};

} 
} 

#endif  

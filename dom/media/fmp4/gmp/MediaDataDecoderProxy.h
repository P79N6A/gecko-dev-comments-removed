





#if !defined(MediaDataDecoderProxy_h_)
#define MediaDataDecoderProxy_h_

#include "PlatformDecoderModule.h"
#include "nsRefPtr.h"
#include "nsThreadUtils.h"
#include "nscore.h"

namespace mozilla {

class InputTask : public nsRunnable {
public:
  InputTask(MediaDataDecoder* aDecoder,
            MediaRawData* aSample)
   : mDecoder(aDecoder)
   , mSample(aSample)
  {}

  NS_IMETHOD Run() {
    mDecoder->Input(mSample);
    return NS_OK;
  }

private:
  nsRefPtr<MediaDataDecoder> mDecoder;
  nsRefPtr<MediaRawData> mSample;
};

class InitTask : public nsRunnable {
public:
  explicit InitTask(MediaDataDecoder* aDecoder)
   : mDecoder(aDecoder)
   , mResultValid(false)
  {}

  NS_IMETHOD Run() {
    mResult = mDecoder->Init();
    mResultValid = true;
    return NS_OK;
  }

  nsresult Result() {
    MOZ_ASSERT(mResultValid);
    return mResult;
  }

private:
  MediaDataDecoder* mDecoder;
  nsresult mResult;
  bool mResultValid;
};

template<typename T>
class Condition {
public:
  explicit Condition(T aValue)
   : mMonitor("Condition")
   , mCondition(aValue)
  {}

  void Set(T aValue) {
    MonitorAutoLock mon(mMonitor);
    mCondition = aValue;
    mon.NotifyAll();
  }

  void WaitUntil(T aValue) {
    MonitorAutoLock mon(mMonitor);
    while (mCondition != aValue) {
      mon.Wait();
    }
  }

private:
  Monitor mMonitor;
  T mCondition;
};

class MediaDataDecoderProxy;

class MediaDataDecoderCallbackProxy : public MediaDataDecoderCallback {
public:
  MediaDataDecoderCallbackProxy(MediaDataDecoderProxy* aProxyDecoder, MediaDataDecoderCallback* aCallback)
   : mProxyDecoder(aProxyDecoder)
   , mProxyCallback(aCallback)
  {
  }

  virtual void Output(MediaData* aData) override {
    mProxyCallback->Output(aData);
  }

  virtual void Error() override;

  virtual void InputExhausted() override {
    mProxyCallback->InputExhausted();
  }

  virtual void DrainComplete() override {
    mProxyCallback->DrainComplete();
  }

  virtual void NotifyResourcesStatusChanged() override {
    mProxyCallback->NotifyResourcesStatusChanged();
  }

  virtual void ReleaseMediaResources() override {
    mProxyCallback->ReleaseMediaResources();
  }

  virtual void FlushComplete();

private:
  MediaDataDecoderProxy* mProxyDecoder;
  MediaDataDecoderCallback* mProxyCallback;
};

class MediaDataDecoderProxy : public MediaDataDecoder {
public:
  MediaDataDecoderProxy(nsIThread* aProxyThread, MediaDataDecoderCallback* aCallback)
   : mProxyThread(aProxyThread)
   , mProxyCallback(this, aCallback)
   , mFlushComplete(false)
#if defined(DEBUG)
   , mIsShutdown(false)
#endif
  {
  }

  
  
  
  
  
  
  MediaDataDecoderCallbackProxy* Callback()
  {
    return &mProxyCallback;
  }

  void SetProxyTarget(MediaDataDecoder* aProxyDecoder)
  {
    MOZ_ASSERT(aProxyDecoder);
    mProxyDecoder = aProxyDecoder;
  }

  
  
  
  
  virtual nsresult Init() override;
  virtual nsresult Input(MediaRawData* aSample) override;
  virtual nsresult Flush() override;
  virtual nsresult Drain() override;
  virtual nsresult Shutdown() override;

  
  void FlushComplete();

private:
#ifdef DEBUG
  bool IsOnProxyThread() {
    return NS_GetCurrentThread() == mProxyThread;
  }
#endif

  friend class InputTask;
  friend class InitTask;

  nsRefPtr<MediaDataDecoder> mProxyDecoder;
  nsCOMPtr<nsIThread> mProxyThread;

  MediaDataDecoderCallbackProxy mProxyCallback;

  Condition<bool> mFlushComplete;
#if defined(DEBUG)
  bool mIsShutdown;
#endif
};

} 

#endif 

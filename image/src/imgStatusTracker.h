





#ifndef imgStatusTracker_h__
#define imgStatusTracker_h__

class imgIContainer;
class imgRequestProxy;
class imgStatusNotifyRunnable;
class imgRequestNotifyRunnable;
class imgStatusTrackerObserver;
struct nsIntRect;
namespace mozilla {
namespace image {
class Image;
} 
} 


#include "mozilla/RefPtr.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsTObserverArray.h"
#include "nsIRunnable.h"
#include "nscore.h"
#include "imgDecoderObserver.h"

enum {
  stateRequestStarted    = 1u << 0,
  stateHasSize           = 1u << 1,
  stateDecodeStarted     = 1u << 2,
  stateDecodeStopped     = 1u << 3,
  stateFrameStopped      = 1u << 4,
  stateRequestStopped    = 1u << 5,
  stateBlockingOnload    = 1u << 6
};












class imgStatusTracker : public mozilla::RefCounted<imgStatusTracker>
{
public:
  
  
  
  imgStatusTracker(mozilla::image::Image* aImage);
  imgStatusTracker(const imgStatusTracker& aOther);
  ~imgStatusTracker();

  
  
  
  
  void SetImage(mozilla::image::Image* aImage);

  
  void SetIsMultipart() { mIsMultipart = true; }

  
  
  
  
  
  void Notify(imgRequestProxy* proxy);

  
  
  
  
  
  void NotifyCurrentState(imgRequestProxy* proxy);

  
  
  
  
  void SyncNotify(imgRequestProxy* proxy);

  
  
  
  void EmulateRequestFinished(imgRequestProxy* proxy, nsresult aStatus);

  
  
  void AddConsumer(imgRequestProxy* aConsumer);
  bool RemoveConsumer(imgRequestProxy* aConsumer, nsresult aStatus);
  size_t ConsumerCount() const { return mConsumers.Length(); }

  
  
  
  bool FirstConsumerIs(imgRequestProxy* aConsumer) {
    return mConsumers.SafeElementAt(0, nullptr) == aConsumer;
  }

  void AdoptConsumers(imgStatusTracker* aTracker) { mConsumers = aTracker->mConsumers; }

  
  
  bool IsLoading() const;

  
  uint32_t GetImageStatus() const;

  
  
  

  
  void RecordCancel();

  
  
  void RecordLoaded();

  
  
  void RecordDecoded();

  
  void RecordStartDecode();
  void SendStartDecode(imgRequestProxy* aProxy);
  void RecordStartContainer(imgIContainer* aContainer);
  void SendStartContainer(imgRequestProxy* aProxy);
  void RecordFrameChanged(const nsIntRect* aDirtyRect);
  void SendFrameChanged(imgRequestProxy* aProxy, const nsIntRect* aDirtyRect);
  void RecordStopFrame();
  void SendStopFrame(imgRequestProxy* aProxy);
  void RecordStopDecode(nsresult statusg);
  void SendStopDecode(imgRequestProxy* aProxy, nsresult aStatus);
  void RecordDiscard();
  void SendDiscard(imgRequestProxy* aProxy);
  void RecordUnlockedDraw();
  void SendUnlockedDraw(imgRequestProxy* aProxy);
  void RecordImageIsAnimated();
  void SendImageIsAnimated(imgRequestProxy *aProxy);

  
  void RecordStartRequest();
  void SendStartRequest(imgRequestProxy* aProxy);
  void RecordStopRequest(bool aLastPart, nsresult aStatus);
  void SendStopRequest(imgRequestProxy* aProxy, bool aLastPart, nsresult aStatus);

  void OnStartRequest();
  void OnDataAvailable();
  void OnStopRequest(bool aLastPart, nsresult aStatus);

  
  
  
  
  void RecordBlockOnload();
  void SendBlockOnload(imgRequestProxy* aProxy);
  void RecordUnblockOnload();
  void SendUnblockOnload(imgRequestProxy* aProxy);

  void MaybeUnblockOnload();

  bool IsMultipart() const { return mIsMultipart; }

  
  inline mozilla::image::Image* GetImage() const { return mImage; }

  inline imgDecoderObserver* GetDecoderObserver() { return mTrackerObserver.get(); }

private:
  friend class imgStatusNotifyRunnable;
  friend class imgRequestNotifyRunnable;
  friend class imgStatusTrackerObserver;

  void FireFailureNotification();

  nsCOMPtr<nsIRunnable> mRequestRunnable;

  
  mozilla::image::Image* mImage;

  
  
  nsTObserverArray<imgRequestProxy*> mConsumers;

  mozilla::RefPtr<imgDecoderObserver> mTrackerObserver;

  uint32_t mState;
  uint32_t mImageStatus;
  bool mIsMultipart    : 1;
  bool mHadLastPart    : 1;
};

#endif







#ifndef imgStatusTracker_h__
#define imgStatusTracker_h__

class imgIContainer;
class imgRequest;
class imgRequestProxy;
class imgStatusNotifyRunnable;
class imgRequestNotifyRunnable;
class imgStatusTracker;
struct nsIntRect;
namespace mozilla {
namespace image {
class Image;
} 
} 


#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsTObserverArray.h"
#include "nsIRunnable.h"
#include "nscore.h"
#include "nsWeakReference.h"
#include "imgIDecoderObserver.h"

enum {
  stateRequestStarted    = PR_BIT(0),
  stateHasSize           = PR_BIT(1),
  stateDecodeStopped     = PR_BIT(3),
  stateFrameStopped      = PR_BIT(4),
  stateRequestStopped    = PR_BIT(5),
  stateBlockingOnload    = PR_BIT(6)
};

class imgStatusTrackerObserver : public imgIDecoderObserver,
                                 public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGIDECODEROBSERVER
  NS_DECL_IMGICONTAINEROBSERVER

  imgStatusTrackerObserver(imgStatusTracker* aTracker)
  : mTracker(aTracker) {}

  virtual ~imgStatusTrackerObserver() {}

  void SetTracker(imgStatusTracker* aTracker) {
    mTracker = aTracker;
  }

private:
  imgStatusTracker* mTracker;
};












class imgStatusTracker
{
public:
  
  
  
  imgStatusTracker(mozilla::image::Image* aImage, imgRequest* aRequest);
  imgStatusTracker(const imgStatusTracker& aOther);

  
  
  
  
  void SetImage(mozilla::image::Image* aImage);

  
  
  
  
  
  void Notify(imgRequest* request, imgRequestProxy* proxy);

  
  
  
  
  
  void NotifyCurrentState(imgRequestProxy* proxy);

  
  
  
  
  void SyncNotify(imgRequestProxy* proxy);

  
  
  
  void EmulateRequestFinished(imgRequestProxy* proxy, nsresult aStatus);

  
  
  void AddConsumer(imgRequestProxy* aConsumer);
  bool RemoveConsumer(imgRequestProxy* aConsumer, nsresult aStatus);
  size_t ConsumerCount() const { return mConsumers.Length(); };

  
  
  
  bool FirstConsumerIs(imgRequestProxy* aConsumer) {
    return mConsumers.SafeElementAt(0, nullptr) == aConsumer;
  }

  void AdoptConsumers(imgStatusTracker* aTracker) { mConsumers = aTracker->mConsumers; }

  
  
  bool IsLoading() const;

  
  uint32_t GetImageStatus() const;

  
  
  

  
  void RecordCancel();

  
  
  void RecordLoaded();

  
  
  void RecordDecoded();

  
  void RecordStartContainer(imgIContainer* aContainer);
  void SendStartContainer(imgRequestProxy* aProxy);
  void RecordDataAvailable();
  void SendDataAvailable(imgRequestProxy* aProxy, const nsIntRect* aRect);
  void RecordStopFrame();
  void SendStopFrame(imgRequestProxy* aProxy);
  void RecordStopDecode(nsresult statusg);
  void SendStopDecode(imgRequestProxy* aProxy, nsresult aStatus);
  void RecordDiscard();
  void SendDiscard(imgRequestProxy* aProxy);
  void RecordImageIsAnimated();
  void SendImageIsAnimated(imgRequestProxy *aProxy);

  
  void RecordFrameChanged(const nsIntRect* aDirtyRect);
  void SendFrameChanged(imgRequestProxy* aProxy, const nsIntRect* aDirtyRect);

  
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

  
  inline mozilla::image::Image* GetImage() const { return mImage; };
  inline imgRequest* GetRequest() const { return mRequest; };

  inline imgIDecoderObserver* GetDecoderObserver() { return mTrackerObserver.get(); }

private:
  friend class imgStatusNotifyRunnable;
  friend class imgRequestNotifyRunnable;
  friend class imgStatusTrackerObserver;

  nsCOMPtr<nsIRunnable> mRequestRunnable;

  
  
  mozilla::image::Image* mImage;
  imgRequest* mRequest;
  uint32_t mState;
  uint32_t mImageStatus;
  bool mHadLastPart;
  bool mBlockingOnload;

  
  
  nsTObserverArray<imgRequestProxy*> mConsumers;

  nsRefPtr<imgStatusTrackerObserver> mTrackerObserver;
};

#endif

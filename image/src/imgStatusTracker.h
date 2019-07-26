





#ifndef imgStatusTracker_h__
#define imgStatusTracker_h__

class imgIContainer;
class imgRequest;
class imgRequestProxy;
class imgStatusNotifyRunnable;
class imgRequestNotifyRunnable;
struct nsIntRect;
namespace mozilla {
namespace image {
class Image;
} 
} 


#include "nsCOMPtr.h"
#include "nsTObserverArray.h"
#include "nsIRunnable.h"
#include "nscore.h"

enum {
  stateRequestStarted    = PR_BIT(0),
  stateHasSize           = PR_BIT(1),
  stateDecodeStarted     = PR_BIT(2),
  stateDecodeStopped     = PR_BIT(3),
  stateFrameStopped      = PR_BIT(4),
  stateRequestStopped    = PR_BIT(5),
  stateBlockingOnload    = PR_BIT(6)
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

  
  const nsTObserverArray<imgRequestProxy*>& GetConsumers() { return mConsumers; };

  void AdoptConsumers(imgStatusTracker* aTracker) { mConsumers = aTracker->mConsumers; }

  
  
  bool IsLoading() const;

  
  uint32_t GetImageStatus() const;

  
  
  

  
  void RecordCancel();

  
  
  void RecordLoaded();

  
  
  void RecordDecoded();

  
  void RecordStartDecode();
  void SendStartDecode(imgRequestProxy* aProxy);
  void RecordStartContainer(imgIContainer* aContainer);
  void SendStartContainer(imgRequestProxy* aProxy, imgIContainer* aContainer);
  void RecordStartFrame(uint32_t aFrame);
  void SendStartFrame(imgRequestProxy* aProxy, uint32_t aFrame);
  void RecordDataAvailable(bool aCurrentFrame, const nsIntRect* aRect);
  void SendDataAvailable(imgRequestProxy* aProxy, bool aCurrentFrame, const nsIntRect* aRect);
  void RecordStopFrame(uint32_t aFrame);
  void SendStopFrame(imgRequestProxy* aProxy, uint32_t aFrame);
  void RecordStopContainer(imgIContainer* aContainer);
  void SendStopContainer(imgRequestProxy* aProxy, imgIContainer* aContainer);
  void RecordStopDecode(nsresult status, const PRUnichar* statusArg);
  void SendStopDecode(imgRequestProxy* aProxy, nsresult aStatus, const PRUnichar* statusArg);
  void RecordDiscard();
  void SendDiscard(imgRequestProxy* aProxy);
  void RecordImageIsAnimated();
  void SendImageIsAnimated(imgRequestProxy *aProxy);

  
  void RecordFrameChanged(imgIContainer* aContainer,
                          const nsIntRect* aDirtyRect);
  void SendFrameChanged(imgRequestProxy* aProxy, imgIContainer* aContainer,
                        const nsIntRect* aDirtyRect);

  
  void RecordStartRequest();
  void SendStartRequest(imgRequestProxy* aProxy);
  void RecordStopRequest(bool aLastPart, nsresult aStatus);
  void SendStopRequest(imgRequestProxy* aProxy, bool aLastPart, nsresult aStatus);

  
  
  
  
  void RecordBlockOnload();
  void SendBlockOnload(imgRequestProxy* aProxy);
  void RecordUnblockOnload();
  void SendUnblockOnload(imgRequestProxy* aProxy);

  
  inline mozilla::image::Image* GetImage() const { return mImage; };
  inline imgRequest* GetRequest() const { return mRequest; };

private:
  friend class imgStatusNotifyRunnable;
  friend class imgRequestNotifyRunnable;

  nsCOMPtr<nsIRunnable> mRequestRunnable;

  
  
  mozilla::image::Image* mImage;
  imgRequest* mRequest;
  uint32_t mState;
  uint32_t mImageStatus;
  bool mHadLastPart;

  
  
  nsTObserverArray<imgRequestProxy*> mConsumers;
};

#endif

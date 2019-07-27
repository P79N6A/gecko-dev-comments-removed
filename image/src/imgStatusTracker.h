





#ifndef imgStatusTracker_h__
#define imgStatusTracker_h__

class imgDecoderObserver;
class imgIContainer;
class imgStatusNotifyRunnable;
class imgRequestNotifyRunnable;
class imgStatusTrackerObserver;
class nsIRunnable;

#include "mozilla/RefPtr.h"
#include "mozilla/WeakPtr.h"
#include "nsCOMPtr.h"
#include "nsTObserverArray.h"
#include "nsThreadUtils.h"
#include "nsRect.h"
#include "imgRequestProxy.h"

namespace mozilla {
namespace image {

class Image;

struct ImageStatusDiff
{
  ImageStatusDiff()
    : invalidRect()
    , diffState(0)
    , diffImageStatus(0)
    , unblockedOnload(false)
    , unsetDecodeStarted(false)
    , foundError(false)
    , foundIsMultipart(false)
    , foundLastPart(false)
    , gotDecoded(false)
  { }

  static ImageStatusDiff NoChange() { return ImageStatusDiff(); }
  bool IsNoChange() const { return *this == NoChange(); }

  bool operator!=(const ImageStatusDiff& aOther) const { return !(*this == aOther); }
  bool operator==(const ImageStatusDiff& aOther) const {
    return aOther.invalidRect == invalidRect
        && aOther.diffState == diffState
        && aOther.diffImageStatus == diffImageStatus
        && aOther.unblockedOnload == unblockedOnload
        && aOther.unsetDecodeStarted == unsetDecodeStarted
        && aOther.foundError == foundError
        && aOther.foundIsMultipart == foundIsMultipart
        && aOther.foundLastPart == foundLastPart
        && aOther.gotDecoded == gotDecoded;
  }

  void Combine(const ImageStatusDiff& aOther) {
    invalidRect = invalidRect.Union(aOther.invalidRect);
    diffState |= aOther.diffState;
    diffImageStatus |= aOther.diffImageStatus;
    unblockedOnload = unblockedOnload || aOther.unblockedOnload;
    unsetDecodeStarted = unsetDecodeStarted || aOther.unsetDecodeStarted;
    foundError = foundError || aOther.foundError;
    foundIsMultipart = foundIsMultipart || aOther.foundIsMultipart;
    foundLastPart = foundLastPart || aOther.foundLastPart;
    gotDecoded = gotDecoded || aOther.gotDecoded;
  }

  nsIntRect invalidRect;
  uint32_t  diffState;
  uint32_t  diffImageStatus;
  bool      unblockedOnload    : 1;
  bool      unsetDecodeStarted : 1;
  bool      foundError         : 1;
  bool      foundIsMultipart   : 1;
  bool      foundLastPart      : 1;
  bool      gotDecoded         : 1;
};

enum {
  stateRequestStarted    = 1u << 0,
  stateHasSize           = 1u << 1,
  stateDecodeStarted     = 1u << 2,
  stateDecodeStopped     = 1u << 3,
  stateFrameStopped      = 1u << 4,
  stateRequestStopped    = 1u << 5,
  stateBlockingOnload    = 1u << 6,
  stateImageIsAnimated   = 1u << 7
};

} 
} 













class imgStatusTracker : public mozilla::SupportsWeakPtr<imgStatusTracker>
{
  virtual ~imgStatusTracker();

public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(imgStatusTracker)
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(imgStatusTracker)

  
  
  
  explicit imgStatusTracker(mozilla::image::Image* aImage);

  
  
  
  
  void SetImage(mozilla::image::Image* aImage);

  
  
  void ResetImage();

  
  void SetIsMultipart() { mIsMultipart = true; }

  
  
  
  
  
  
  
  void Notify(imgRequestProxy* proxy);

  
  
  
  
  
  
  
  void NotifyCurrentState(imgRequestProxy* proxy);

  
  
  
  
  
  
  void SyncNotify(imgRequestProxy* proxy);

  
  
  
  void EmulateRequestFinished(imgRequestProxy* proxy, nsresult aStatus);

  
  
  void AddConsumer(imgRequestProxy* aConsumer);
  bool RemoveConsumer(imgRequestProxy* aConsumer, nsresult aStatus);
  size_t ConsumerCount() const {
    MOZ_ASSERT(NS_IsMainThread(), "Use mConsumers on main thread only");
    return mConsumers.Length();
  }

  
  
  
  bool FirstConsumerIs(imgRequestProxy* aConsumer);

  void AdoptConsumers(imgStatusTracker* aTracker) {
    MOZ_ASSERT(NS_IsMainThread(), "Use mConsumers on main thread only");
    MOZ_ASSERT(aTracker);
    mConsumers = aTracker->mConsumers;
  }

  
  
  bool IsLoading() const;

  
  uint32_t GetImageStatus() const;

  
  
  

  
  void RecordCancel();

  
  
  void RecordLoaded();

  
  
  void RecordDecoded();

  
  
  
  void RecordStartDecode();
  void SendStartDecode(imgRequestProxy* aProxy);
  void RecordStartContainer(imgIContainer* aContainer);
  void SendStartContainer(imgRequestProxy* aProxy);
  void RecordStartFrame();
  
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
  void OnDiscard();
  void FrameChanged(const nsIntRect* aDirtyRect);
  void OnUnlockedDraw();
  
  
  void OnStopFrame();

  
  
  
  
  void RecordBlockOnload();
  void SendBlockOnload(imgRequestProxy* aProxy);
  void RecordUnblockOnload();
  void SendUnblockOnload(imgRequestProxy* aProxy);

  
  void MaybeUnblockOnload();

  void RecordError();

  bool IsMultipart() const { return mIsMultipart; }

  
  inline already_AddRefed<mozilla::image::Image> GetImage() const {
    nsRefPtr<mozilla::image::Image> image = mImage;
    return image.forget();
  }
  inline bool HasImage() { return mImage; }

  inline imgDecoderObserver* GetDecoderObserver() { return mTrackerObserver.get(); }

  already_AddRefed<imgStatusTracker> CloneForRecording();

  
  mozilla::image::ImageStatusDiff Difference(imgStatusTracker* aOther) const;

  
  
  mozilla::image::ImageStatusDiff DecodeStateAsDifference() const;

  
  void ApplyDifference(const mozilla::image::ImageStatusDiff& aDiff);

  
  
  
  void SyncNotifyDifference(const mozilla::image::ImageStatusDiff& aDiff);

  nsIntRect GetInvalidRect() const { return mInvalidRect; }

private:
  typedef nsTObserverArray<mozilla::WeakPtr<imgRequestProxy>> ProxyArray;
  friend class imgStatusNotifyRunnable;
  friend class imgRequestNotifyRunnable;
  friend class imgStatusTrackerObserver;
  friend class imgStatusTrackerInit;
  imgStatusTracker(const imgStatusTracker& aOther);

  
  void FireFailureNotification();

  
  
  static void SyncNotifyState(ProxyArray& proxies,
                              bool hasImage, uint32_t state,
                              nsIntRect& dirtyRect, bool hadLastPart);

  nsCOMPtr<nsIRunnable> mRequestRunnable;

  
  
  nsIntRect mInvalidRect;

  
  mozilla::image::Image* mImage;

  
  
  
  ProxyArray mConsumers;

  mozilla::RefPtr<imgDecoderObserver> mTrackerObserver;

  uint32_t mState;
  uint32_t mImageStatus;
  bool mIsMultipart    : 1;
  bool mHadLastPart    : 1;
  bool mHasBeenDecoded : 1;
};

class imgStatusTrackerInit
{
public:
  imgStatusTrackerInit(mozilla::image::Image* aImage,
                       imgStatusTracker* aTracker);
  ~imgStatusTrackerInit();
private:
  imgStatusTracker* mTracker;
};

#endif

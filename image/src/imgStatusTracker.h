





#ifndef imgStatusTracker_h__
#define imgStatusTracker_h__

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


enum {
  FLAG_REQUEST_STARTED    = 1u << 0,
  FLAG_HAS_SIZE           = 1u << 1,  
  FLAG_DECODE_STARTED     = 1u << 2,  
  FLAG_DECODE_STOPPED     = 1u << 3,  
  FLAG_FRAME_STOPPED      = 1u << 4,  
  FLAG_REQUEST_STOPPED    = 1u << 5,  
  FLAG_ONLOAD_BLOCKED     = 1u << 6,
  FLAG_ONLOAD_UNBLOCKED   = 1u << 7,
  FLAG_IS_ANIMATED        = 1u << 8,
  FLAG_IS_MULTIPART       = 1u << 9,
  FLAG_MULTIPART_STOPPED  = 1u << 10,
  FLAG_HAS_ERROR          = 1u << 11  
};

struct ImageStatusDiff
{
  ImageStatusDiff()
    : diffState(0)
  { }

  static ImageStatusDiff NoChange() { return ImageStatusDiff(); }
  bool IsNoChange() const { return *this == NoChange(); }

  static ImageStatusDiff ForOnStopRequest(bool aLastPart,
                                          bool aError,
                                          nsresult aStatus)
  {
    ImageStatusDiff diff;
    diff.diffState |= FLAG_REQUEST_STOPPED;
    if (aLastPart) {
      diff.diffState |= FLAG_MULTIPART_STOPPED;
    }
    if (NS_FAILED(aStatus) || aError) {
      diff.diffState |= FLAG_HAS_ERROR;
    }
    return diff;
  }

  bool operator!=(const ImageStatusDiff& aOther) const { return !(*this == aOther); }
  bool operator==(const ImageStatusDiff& aOther) const {
    return aOther.diffState == diffState;
  }

  void Combine(const ImageStatusDiff& aOther) {
    diffState |= aOther.diffState;
  }

  uint32_t diffState;
};

} 
} 













class imgStatusTracker : public mozilla::SupportsWeakPtr<imgStatusTracker>
{
  virtual ~imgStatusTracker() { }

public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(imgStatusTracker)
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(imgStatusTracker)

  
  
  
  explicit imgStatusTracker(mozilla::image::Image* aImage)
    : mImage(aImage)
    , mState(0)
  { }

  
  
  
  
  void SetImage(mozilla::image::Image* aImage);

  
  
  void ResetImage();

  
  void SetIsMultipart();

  
  
  
  
  
  
  
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

  
  
  void SendStartDecode(imgRequestProxy* aProxy);
  void SendStartContainer(imgRequestProxy* aProxy);
  void SendStopFrame(imgRequestProxy* aProxy);
  void SendStopDecode(imgRequestProxy* aProxy, nsresult aStatus);
  void SendDiscard(imgRequestProxy* aProxy);
  void SendUnlockedDraw(imgRequestProxy* aProxy);
  void SendImageIsAnimated(imgRequestProxy *aProxy);

  
  
  
  void SendStartRequest(imgRequestProxy* aProxy);
  void SendStopRequest(imgRequestProxy* aProxy, bool aLastPart, nsresult aStatus);

  
  
  void OnStartRequest();
  
  
  void OnDataAvailable();
  void OnDiscard();
  void OnUnlockedDraw();

  
  
  
  
  void SendBlockOnload(imgRequestProxy* aProxy);
  void SendUnblockOnload(imgRequestProxy* aProxy);

  
  void MaybeUnblockOnload();

  void RecordError();

  bool IsMultipart() const { return mState & mozilla::image::FLAG_IS_MULTIPART; }

  
  inline already_AddRefed<mozilla::image::Image> GetImage() const {
    nsRefPtr<mozilla::image::Image> image = mImage;
    return image.forget();
  }
  inline bool HasImage() { return mImage; }

  
  mozilla::image::ImageStatusDiff Difference(const mozilla::image::ImageStatusDiff& aOther) const;

  
  void ApplyDifference(const mozilla::image::ImageStatusDiff& aDiff);

  
  
  
  void SyncNotifyDifference(const mozilla::image::ImageStatusDiff& aDiff,
                            const nsIntRect& aInvalidRect = nsIntRect());

private:
  typedef nsTObserverArray<mozilla::WeakPtr<imgRequestProxy>> ProxyArray;
  friend class imgStatusNotifyRunnable;
  friend class imgRequestNotifyRunnable;
  friend class imgStatusTrackerObserver;
  friend class imgStatusTrackerInit;

  imgStatusTracker(const imgStatusTracker& aOther) MOZ_DELETE;

  
  void FireFailureNotification();

  
  
  static void SyncNotifyState(ProxyArray& aProxies,
                              bool aHasImage, uint32_t aState,
                              const nsIntRect& aInvalidRect);

  nsCOMPtr<nsIRunnable> mRequestRunnable;

  
  mozilla::image::Image* mImage;

  
  
  
  ProxyArray mConsumers;

  uint32_t mState;
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

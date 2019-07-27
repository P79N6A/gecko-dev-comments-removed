





#ifndef ProgressTracker_h__
#define ProgressTracker_h__

#include "mozilla/RefPtr.h"
#include "mozilla/WeakPtr.h"
#include "nsCOMPtr.h"
#include "nsTObserverArray.h"
#include "nsThreadUtils.h"
#include "nsRect.h"
#include "imgRequestProxy.h"

class imgIContainer;
class nsIRunnable;

namespace mozilla {
namespace image {

class AsyncNotifyRunnable;
class AsyncNotifyCurrentStateRunnable;
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

typedef uint32_t Progress;

const uint32_t NoProgress = 0;

inline Progress OnStopRequestProgress(bool aLastPart,
                                      bool aError,
                                      nsresult aStatus)
{
  Progress progress = FLAG_REQUEST_STOPPED;
  if (aLastPart) {
    progress |= FLAG_MULTIPART_STOPPED;
  }
  if (NS_FAILED(aStatus) || aError) {
    progress |= FLAG_HAS_ERROR;
  }
  return progress;
}










class ProgressTracker : public mozilla::SupportsWeakPtr<ProgressTracker>
{
  virtual ~ProgressTracker() { }

public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(ProgressTracker)
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ProgressTracker)

  
  
  
  explicit ProgressTracker(Image* aImage)
    : mImage(aImage)
    , mProgress(NoProgress)
  { }

  bool HasImage() const { return mImage; }
  already_AddRefed<Image> GetImage() const
  {
    nsRefPtr<Image> image = mImage;
    return image.forget();
  }

  
  void SetIsMultipart();

  
  
  bool IsLoading() const;

  
  uint32_t GetImageStatus() const;

  
  
  
  
  
  
  
  void Notify(imgRequestProxy* proxy);

  
  
  
  
  
  
  
  void NotifyCurrentState(imgRequestProxy* proxy);

  
  
  
  
  
  
  void SyncNotify(imgRequestProxy* proxy);

  
  
  void ResetForNewRequest();

  
  
  void OnDiscard();
  void OnUnlockedDraw();
  void OnImageAvailable();

  
  
  Progress Difference(Progress aProgress) const
  {
    return ~mProgress & aProgress;
  }

  
  
  
  
  
  void SyncNotifyProgress(Progress aProgress,
                          const nsIntRect& aInvalidRect = nsIntRect());

  
  
  void AddConsumer(imgRequestProxy* aConsumer);
  bool RemoveConsumer(imgRequestProxy* aConsumer, nsresult aStatus);
  size_t ConsumerCount() const {
    MOZ_ASSERT(NS_IsMainThread(), "Use mConsumers on main thread only");
    return mConsumers.Length();
  }

  
  
  
  bool FirstConsumerIs(imgRequestProxy* aConsumer);

  void AdoptConsumers(ProgressTracker* aTracker) {
    MOZ_ASSERT(NS_IsMainThread(), "Use mConsumers on main thread only");
    MOZ_ASSERT(aTracker);
    mConsumers = aTracker->mConsumers;
  }

private:
  typedef nsTObserverArray<mozilla::WeakPtr<imgRequestProxy>> ProxyArray;
  friend class AsyncNotifyRunnable;
  friend class AsyncNotifyCurrentStateRunnable;
  friend class ProgressTrackerInit;

  ProgressTracker(const ProgressTracker& aOther) MOZ_DELETE;

  
  
  void SetImage(Image* aImage);

  
  
  void ResetImage();

  
  
  
  void EmulateRequestFinished(imgRequestProxy* aProxy, nsresult aStatus);

  
  void FireFailureNotification();

  
  
  static void SyncNotifyInternal(ProxyArray& aProxies,
                                 bool aHasImage, Progress aProgress,
                                 const nsIntRect& aInvalidRect);

  nsCOMPtr<nsIRunnable> mRunnable;

  
  Image* mImage;

  
  
  
  ProxyArray mConsumers;

  Progress mProgress;
};

class ProgressTrackerInit
{
public:
  ProgressTrackerInit(Image* aImage, ProgressTracker* aTracker);
  ~ProgressTrackerInit();
private:
  ProgressTracker* mTracker;
};

} 
} 

#endif

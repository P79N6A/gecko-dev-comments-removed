





#ifndef mozilla_image_src_ProgressTracker_h
#define mozilla_image_src_ProgressTracker_h

#include "mozilla/Mutex.h"
#include "mozilla/RefPtr.h"
#include "mozilla/WeakPtr.h"
#include "nsCOMPtr.h"
#include "nsTObserverArray.h"
#include "nsThreadUtils.h"
#include "nsRect.h"
#include "IProgressObserver.h"

class nsIRunnable;

namespace mozilla {
namespace image {

class AsyncNotifyRunnable;
class AsyncNotifyCurrentStateRunnable;
class Image;


enum {
  FLAG_SIZE_AVAILABLE     = 1u << 0,  
  FLAG_DECODE_STARTED     = 1u << 1,  
  FLAG_DECODE_COMPLETE    = 1u << 2,  
  FLAG_FRAME_COMPLETE     = 1u << 3,  
  FLAG_LOAD_COMPLETE      = 1u << 4,  
  FLAG_ONLOAD_BLOCKED     = 1u << 5,
  FLAG_ONLOAD_UNBLOCKED   = 1u << 6,
  FLAG_IS_ANIMATED        = 1u << 7,  
  FLAG_HAS_TRANSPARENCY   = 1u << 8,  
  FLAG_LAST_PART_COMPLETE = 1u << 9,
  FLAG_HAS_ERROR          = 1u << 10  
};

typedef uint32_t Progress;

const uint32_t NoProgress = 0;

inline Progress LoadCompleteProgress(bool aLastPart,
                                     bool aError,
                                     nsresult aStatus)
{
  Progress progress = FLAG_LOAD_COMPLETE;
  if (aLastPart) {
    progress |= FLAG_LAST_PART_COMPLETE;
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

  ProgressTracker()
    : mImageMutex("ProgressTracker::mImage")
    , mImage(nullptr)
    , mProgress(NoProgress)
  { }

  bool HasImage() const { MutexAutoLock lock(mImageMutex); return mImage; }
  already_AddRefed<Image> GetImage() const
  {
    MutexAutoLock lock(mImageMutex);
    nsRefPtr<Image> image = mImage;
    return image.forget();
  }

  
  uint32_t GetImageStatus() const;

  
  Progress GetProgress() const { return mProgress; }

  
  
  
  
  
  
  
  void Notify(IProgressObserver* aObserver);

  
  
  
  
  
  
  
  void NotifyCurrentState(IProgressObserver* aObserver);

  
  
  
  
  
  
  void SyncNotify(IProgressObserver* aObserver);

  
  
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

  
  
  void AddObserver(IProgressObserver* aObserver);
  bool RemoveObserver(IProgressObserver* aObserver);
  size_t ObserverCount() const {
    MOZ_ASSERT(NS_IsMainThread(), "Use mObservers on main thread only");
    return mObservers.Length();
  }

  
  
  
  bool FirstObserverIs(IProgressObserver* aObserver);

private:
  typedef nsTObserverArray<mozilla::WeakPtr<IProgressObserver>> ObserverArray;
  friend class AsyncNotifyRunnable;
  friend class AsyncNotifyCurrentStateRunnable;
  friend class ProgressTrackerInit;

  ProgressTracker(const ProgressTracker& aOther) = delete;

  
  
  void SetImage(Image* aImage);

  
  
  void ResetImage();

  
  
  
  void EmulateRequestFinished(IProgressObserver* aObserver);

  
  void FireFailureNotification();

  
  
  static void SyncNotifyInternal(ObserverArray& aObservers,
                                 bool aHasImage, Progress aProgress,
                                 const nsIntRect& aInvalidRect);

  nsCOMPtr<nsIRunnable> mRunnable;

  
  
  mutable Mutex mImageMutex;
  Image* mImage;

  
  
  
  ObserverArray mObservers;

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

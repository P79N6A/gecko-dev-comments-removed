





#ifndef mozilla_dom_SourceBuffer_h_
#define mozilla_dom_SourceBuffer_h_

#include "AsyncEventRunner.h"
#include "MediaSource.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/SourceBufferBinding.h"
#include "mozilla/dom/TimeRanges.h"
#include "mozilla/dom/TypedArray.h"
#include "mozilla/ErrorResult.h"
#include "nsCOMPtr.h"
#include "nscore.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDOMEventTargetHelper.h"
#include "nsWrapperCache.h"

namespace mozilla {

template <typename T> class AsyncEventRunner;

namespace dom {

class SourceBuffer MOZ_FINAL : public nsDOMEventTargetHelper
{
public:
  
  SourceBufferAppendMode Mode() const
  {
    return mAppendMode;
  }

  void SetMode(SourceBufferAppendMode aMode, ErrorResult& aRv);

  bool Updating() const
  {
    return mUpdating;
  }

  already_AddRefed<TimeRanges> GetBuffered(ErrorResult& aRv);

  double TimestampOffset() const
  {
    return mTimestampOffset;
  }

  void SetTimestampOffset(double aTimestampOffset, ErrorResult& aRv);

  double AppendWindowStart() const
  {
    return mAppendWindowStart;
  }

  void SetAppendWindowStart(double aAppendWindowStart, ErrorResult& aRv);

  double AppendWindowEnd() const
  {
    return mAppendWindowEnd;
  }

  void SetAppendWindowEnd(double aAppendWindowEnd, ErrorResult& aRv);

  void AppendBuffer(const ArrayBuffer& aData, ErrorResult& aRv);
  void AppendBuffer(const ArrayBufferView& aData, ErrorResult& aRv);

  void Abort(ErrorResult& aRv);

  void Remove(double aStart, double aEnd, ErrorResult& aRv);
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(SourceBuffer, nsDOMEventTargetHelper)

  explicit SourceBuffer(MediaSource* aMediaSource);

  MediaSource* GetParentObject() const;

  JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  
  
  void Attach();
  void Detach();

private:
  friend class AsyncEventRunner<SourceBuffer>;
  void DispatchSimpleEvent(const char* aName);
  void QueueAsyncSimpleEvent(const char* aName);

  
  void StartUpdating();
  void StopUpdating();
  void AbortUpdating();

  
  void AppendData(const uint8_t* aData, uint32_t aLength, ErrorResult& aRv);

  nsRefPtr<MediaSource> mMediaSource;

  double mAppendWindowStart;
  double mAppendWindowEnd;

  double mTimestampOffset;

  SourceBufferAppendMode mAppendMode;
  bool mUpdating;

  bool mAttached;
};

} 
} 
#endif 

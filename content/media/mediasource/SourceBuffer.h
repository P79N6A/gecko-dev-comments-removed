





#ifndef mozilla_dom_SourceBuffer_h_
#define mozilla_dom_SourceBuffer_h_

#include "MediaSource.h"
#include "js/RootingAPI.h"
#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/SourceBufferBinding.h"
#include "mozilla/dom/TypedArray.h"
#include "mozilla/mozalloc.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionNoteChild.h"
#include "nsCycleCollectionParticipant.h"
#include "nsISupports.h"
#include "nsString.h"
#include "nscore.h"

class JSObject;
struct JSContext;

namespace mozilla {

class ContainerParser;
class ErrorResult;
class TrackBuffer;
template <typename T> class AsyncEventRunner;

namespace dom {

class TimeRanges;

class SourceBuffer MOZ_FINAL : public DOMEventTargetHelper
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
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(SourceBuffer, DOMEventTargetHelper)

  SourceBuffer(MediaSource* aMediaSource, const nsACString& aType);

  MediaSource* GetParentObject() const;

  JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  
  void Detach();
  bool IsAttached() const
  {
    return mMediaSource != nullptr;
  }

  void Ended();

  
  void Evict(double aStart, double aEnd);

  double GetBufferedStart();
  double GetBufferedEnd();

#if defined(DEBUG)
  void Dump(const char* aPath);
#endif

private:
  ~SourceBuffer();

  friend class AsyncEventRunner<SourceBuffer>;
  void DispatchSimpleEvent(const char* aName);
  void QueueAsyncSimpleEvent(const char* aName);

  
  
  bool InitNewDecoder();

  
  void DiscardDecoder();

  
  void StartUpdating();
  void StopUpdating();
  void AbortUpdating();

  
  void AppendData(const uint8_t* aData, uint32_t aLength, ErrorResult& aRv);

  nsRefPtr<MediaSource> mMediaSource;

  const nsCString mType;

  nsAutoPtr<ContainerParser> mParser;

  nsRefPtr<TrackBuffer> mTrackBuffer;

  double mAppendWindowStart;
  double mAppendWindowEnd;

  double mTimestampOffset;

  SourceBufferAppendMode mAppendMode;
  bool mUpdating;
};

} 

} 
#endif 

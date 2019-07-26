





#ifndef mozilla_dom_MediaSource_h_
#define mozilla_dom_MediaSource_h_

#include "AsyncEventRunner.h"
#include "mozilla/Attributes.h"
#include "mozilla/Monitor.h"
#include "mozilla/dom/MediaSourceBinding.h"
#include "mozilla/dom/TypedArray.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDOMEventTargetHelper.h"
#include "nsWrapperCache.h"
#include "nscore.h"

namespace mozilla {
namespace dom {

class HTMLMediaElement;
class MediaSourceInputAdapter;
class SourceBufferList;
class SourceBuffer;
class TimeRanges;

#define MOZILLA_DOM_MEDIASOURCE_IMPLEMENTATION_IID \
  { 0x3839d699, 0x22c5, 0x439f, \
  { 0x94, 0xca, 0x0e, 0x0b, 0x26, 0xf9, 0xca, 0xbf } }

class MediaSource MOZ_FINAL : public nsDOMEventTargetHelper
{
public:
  
  static already_AddRefed<MediaSource> Constructor(const GlobalObject& aGlobal, ErrorResult& aRv);

  SourceBufferList* SourceBuffers();
  SourceBufferList* ActiveSourceBuffers();
  MediaSourceReadyState ReadyState();

  double Duration();
  void SetDuration(double aDuration, ErrorResult& aRv);

  already_AddRefed<SourceBuffer> AddSourceBuffer(const nsAString& aType, ErrorResult& aRv);
  void RemoveSourceBuffer(SourceBuffer& aSourceBuffer, ErrorResult& aRv);

  void EndOfStream(const Optional<MediaSourceEndOfStreamError>& aError, ErrorResult& aRv);
  static bool IsTypeSupported(const GlobalObject& aGlobal, const nsAString& aType);
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(MediaSource, nsDOMEventTargetHelper)
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_DOM_MEDIASOURCE_IMPLEMENTATION_IID)

  nsPIDOMWindow* GetParentObject() const;

  JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  const nsString& GetType()
  {
    return mContentType;
  }

  already_AddRefed<nsIInputStream> CreateInternalStream();


  void AppendData(const uint8_t* aData, uint32_t aLength, ErrorResult& aRv);

  
  nsTArray<uint8_t> const& GetData()
  {
    return mData;
  }

  Monitor& GetMonitor()
  {
    return mMonitor;
  }

  bool AppendDone() const
  {
    return mReadyState == MediaSourceReadyState::Closed;
  }

  
  bool AttachElement(HTMLMediaElement* aElement);
  void DetachElement();

  
  void SetReadyState(MediaSourceReadyState aState);

  void GetBuffered(TimeRanges* aRanges);

private:
  explicit MediaSource(nsPIDOMWindow* aWindow);

  friend class AsyncEventRunnner<MediaSource>;
  void DispatchSimpleEvent(const char* aName);
  void QueueAsyncSimpleEvent(const char* aName);

  void NotifyListeners();

  void DurationChange(double aNewDuration, ErrorResult& aRv);
  void EndOfStreamInternal(const Optional<MediaSourceEndOfStreamError>& aError, ErrorResult& aRv);

  static bool IsTypeSupportedInternal(const nsAString& aType, ErrorResult& aRv);

  double mDuration;

  nsTArray<nsRefPtr<MediaSourceInputAdapter> > mAdapters;

  
  nsTArray<uint8_t> mData;

  
  Monitor mMonitor;

  nsRefPtr<SourceBufferList> mSourceBuffers;
  nsRefPtr<SourceBufferList> mActiveSourceBuffers;

  nsRefPtr<HTMLMediaElement> mElement;

  nsString mContentType;
  MediaSourceReadyState mReadyState;
};

NS_DEFINE_STATIC_IID_ACCESSOR(MediaSource, MOZILLA_DOM_MEDIASOURCE_IMPLEMENTATION_IID)

} 
} 
#endif 

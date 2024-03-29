





#ifndef mozilla_dom_MediaSource_h_
#define mozilla_dom_MediaSource_h_

#include "MediaSourceDecoder.h"
#include "js/RootingAPI.h"
#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/MediaSourceBinding.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionNoteChild.h"
#include "nsCycleCollectionParticipant.h"
#include "nsID.h"
#include "nsISupports.h"
#include "nscore.h"

struct JSContext;
class JSObject;
class nsPIDOMWindow;

namespace mozilla {

class ErrorResult;
template <typename T> class AsyncEventRunner;

enum MSRangeRemovalAction: uint8_t {
  RUN = 0,
  SKIP = 1
};

namespace dom {

class GlobalObject;
class SourceBuffer;
class SourceBufferList;
template <typename T> class Optional;

#define MOZILLA_DOM_MEDIASOURCE_IMPLEMENTATION_IID \
  { 0x3839d699, 0x22c5, 0x439f, \
  { 0x94, 0xca, 0x0e, 0x0b, 0x26, 0xf9, 0xca, 0xbf } }

class MediaSource final : public DOMEventTargetHelper
{
public:
  
  static already_AddRefed<MediaSource>
  Constructor(const GlobalObject& aGlobal,
              ErrorResult& aRv);

  SourceBufferList* SourceBuffers();
  SourceBufferList* ActiveSourceBuffers();
  MediaSourceReadyState ReadyState();

  double Duration();
  void SetDuration(double aDuration, ErrorResult& aRv);

  already_AddRefed<SourceBuffer> AddSourceBuffer(const nsAString& aType, ErrorResult& aRv);
  void RemoveSourceBuffer(SourceBuffer& aSourceBuffer, ErrorResult& aRv);

  void EndOfStream(const Optional<MediaSourceEndOfStreamError>& aError, ErrorResult& aRv);
  static bool IsTypeSupported(const GlobalObject&, const nsAString& aType);

  static bool Enabled(JSContext* cx, JSObject* aGlobal);
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(MediaSource, DOMEventTargetHelper)
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_DOM_MEDIASOURCE_IMPLEMENTATION_IID)

  nsPIDOMWindow* GetParentObject() const;

  JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  
  bool Attach(MediaSourceDecoder* aDecoder);
  void Detach();

  
  void SetReadyState(MediaSourceReadyState aState);

 
  MediaSourceDecoder* GetDecoder()
  {
    return mDecoder;
  }

  nsIPrincipal* GetPrincipal()
  {
    return mPrincipal;
  }

  
  
  
  void NotifyEvicted(double aStart, double aEnd);

  
  
  
  
  
  
  void QueueInitializationEvent();

#if defined(DEBUG)
  
  
  void Dump(const char* aPath);
#endif

  
  
  void GetMozDebugReaderData(nsAString& aString);

private:
  
  
  friend class mozilla::MediaSourceDecoder;
  
  friend class mozilla::dom::SourceBuffer;

  ~MediaSource();

  explicit MediaSource(nsPIDOMWindow* aWindow);

  friend class AsyncEventRunner<MediaSource>;
  void DispatchSimpleEvent(const char* aName);
  void QueueAsyncSimpleEvent(const char* aName);

  void DurationChange(double aOldDuration, double aNewDuration);

  void InitializationEvent();

  
  void SetDuration(double aDuration, MSRangeRemovalAction aAction);

  
  void SourceBufferIsActive(SourceBuffer* aSourceBuffer);

  nsRefPtr<SourceBufferList> mSourceBuffers;
  nsRefPtr<SourceBufferList> mActiveSourceBuffers;

  nsRefPtr<MediaSourceDecoder> mDecoder;
  
  
  nsRefPtr<HTMLMediaElement> mMediaElement;

  nsRefPtr<nsIPrincipal> mPrincipal;

  MediaSourceReadyState mReadyState;

  bool mFirstSourceBufferInitialized;
};

NS_DEFINE_STATIC_IID_ACCESSOR(MediaSource, MOZILLA_DOM_MEDIASOURCE_IMPLEMENTATION_IID)

} 

} 

#endif 







#ifndef MediaRecorder_h
#define MediaRecorder_h

#include "DOMMediaStream.h"
#include "MediaEncoder.h"
#include "mozilla/dom/MediaRecorderBinding.h"
#include "nsDOMEventTargetHelper.h"
#include "EncodedBufferCache.h"
#include "TrackUnionStream.h"


#define MAX_ALLOW_MEMORY_BUFFER 1024000
namespace mozilla {

class ErrorResult;

namespace dom {












class MediaRecorder : public nsDOMEventTargetHelper
{
  class ExtractEncodedDataTask;
  class PushBlobTask;
  class PushErrorMessageTask;
public:
  MediaRecorder(DOMMediaStream&);
  virtual ~MediaRecorder();

  
  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  nsPIDOMWindow* GetParentObject() { return GetOwner(); }

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(MediaRecorder,
                                           nsDOMEventTargetHelper)

  
  
  
  
  void Start(const Optional<int32_t>& timeSlice, ErrorResult & aResult);
  
  void Stop(ErrorResult& aResult);
  
  void Pause(ErrorResult& aResult);

  void Resume(ErrorResult& aResult);
  
  void RequestData(ErrorResult& aResult);
  
  DOMMediaStream* Stream() const { return mStream; }
  
  RecordingState State() const { return mState; }
  
  void GetMimeType(nsString &aMimeType) { aMimeType = mMimeType; }

  static already_AddRefed<MediaRecorder>
  Constructor(const GlobalObject& aGlobal, JSContext* aCx, DOMMediaStream& aStream,
              ErrorResult& aRv);

  
  IMPL_EVENT_HANDLER(dataavailable)
  IMPL_EVENT_HANDLER(error)
  IMPL_EVENT_HANDLER(stop)
  IMPL_EVENT_HANDLER(warning)

  friend class ExtractEncodedData;

protected:
  void Init(JSContext* aCx, nsPIDOMWindow* aOwnerWindow);
  
  void ExtractEncodedData();

  MediaRecorder& operator = (const MediaRecorder& x) MOZ_DELETE;
  
  nsresult CreateAndDispatchBlobEvent();
  
  void DispatchSimpleEvent(const nsAString & aStr);
  
  void NotifyError(nsresult aRv);
  
  bool CheckPrincipal();

  MediaRecorder(const MediaRecorder& x) MOZ_DELETE; 


  
  nsCOMPtr<nsIThread> mReadThread;
  
  nsRefPtr<MediaEncoder> mEncoder;
  
  nsRefPtr<DOMMediaStream> mStream;
  
  nsRefPtr<ProcessedMediaStream> mTrackUnionStream;
  
  nsAutoPtr<EncodedBufferCache> mEncodedBufferCache;
  
  nsString mMimeType;
  
  
  int32_t mTimeSlice;
  
  RecordingState mState;
};

}
}

#endif







#ifndef MediaRecorder_h
#define MediaRecorder_h

#include "mozilla/dom/MediaRecorderBinding.h"
#include "nsDOMEventTargetHelper.h"


#define MAX_ALLOW_MEMORY_BUFFER 1024000
namespace mozilla {

class ErrorResult;
class DOMMediaStream;
class EncodedBufferCache;
class MediaEncoder;
class ProcessedMediaStream;
class MediaInputPort;

namespace dom {












class MediaRecorder : public nsDOMEventTargetHelper
{
  class Session;

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
  
  void GetMimeType(nsString &aMimeType);

  static already_AddRefed<MediaRecorder>
  Constructor(const GlobalObject& aGlobal,
              DOMMediaStream& aStream, ErrorResult& aRv);

  
  IMPL_EVENT_HANDLER(dataavailable)
  IMPL_EVENT_HANDLER(error)
  IMPL_EVENT_HANDLER(stop)
  IMPL_EVENT_HANDLER(warning)

protected:
  void Init(nsPIDOMWindow* aOwnerWindow);

  MediaRecorder& operator = (const MediaRecorder& x) MOZ_DELETE;
  
  nsresult CreateAndDispatchBlobEvent(Session *session);
  
  void DispatchSimpleEvent(const nsAString & aStr);
  
  void NotifyError(nsresult aRv);
  
  bool CheckPrincipal();
  
  void SetMimeType(const nsString &aMimeType);

  MediaRecorder(const MediaRecorder& x) MOZ_DELETE; 

  
  nsRefPtr<DOMMediaStream> mStream;
  
  RecordingState mState;
  
  Session *mSession;
  
  Mutex mMutex;
  
  nsString mMimeType;
};

}
}

#endif







#ifndef IMAGECAPTURE_H
#define IMAGECAPTURE_H

#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/ImageCaptureBinding.h"
#include "prlog.h"

class nsIDOMBlob;

namespace mozilla {

#ifdef PR_LOGGING

#ifndef IC_LOG
PRLogModuleInfo* GetICLog();
#define IC_LOG(...) PR_LOG(GetICLog(), PR_LOG_DEBUG, (__VA_ARGS__))
#endif

#else

#ifndef IC_LOG
#define IC_LOG(...)
#endif

#endif 

namespace dom {

class VideoStreamTrack;










class ImageCapture MOZ_FINAL : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ImageCapture, DOMEventTargetHelper)

  IMPL_EVENT_HANDLER(photo)
  IMPL_EVENT_HANDLER(error)

  
  void TakePhoto(ErrorResult& aResult);

  
  VideoStreamTrack* GetVideoStreamTrack() const;

  
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE
  {
    return ImageCaptureBinding::Wrap(aCx, this);
  }

  
  nsPIDOMWindow* GetParentObject() { return GetOwner(); }

  static already_AddRefed<ImageCapture> Constructor(const GlobalObject& aGlobal,
                                                    VideoStreamTrack& aTrack,
                                                    ErrorResult& aRv);

  ImageCapture(VideoStreamTrack* aVideoStreamTrack, nsPIDOMWindow* aOwnerWindow);

  
  nsresult PostBlobEvent(nsIDOMBlob* aBlob);

  
  
  
  nsresult PostErrorEvent(uint16_t aErrorCode, nsresult aReason = NS_OK);

  bool CheckPrincipal();

protected:
  virtual ~ImageCapture();

  nsRefPtr<VideoStreamTrack> mVideoStreamTrack;
};

} 
} 

#endif 

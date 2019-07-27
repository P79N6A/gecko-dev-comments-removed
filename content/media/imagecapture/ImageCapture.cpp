





#include "ImageCapture.h"
#include "mozilla/dom/BlobEvent.h"
#include "mozilla/dom/DOMException.h"
#include "mozilla/dom/ImageCaptureError.h"
#include "mozilla/dom/ImageCaptureErrorEvent.h"
#include "mozilla/dom/ImageCaptureErrorEventBinding.h"
#include "mozilla/dom/VideoStreamTrack.h"
#include "nsDOMFile.h"
#include "nsIDocument.h"
#include "CaptureTask.h"
#include "MediaEngine.h"

namespace mozilla {

#ifdef PR_LOGGING

PRLogModuleInfo* GetICLog()
{
  static PRLogModuleInfo* log = nullptr;
  if (!log) {
    log = PR_NewLogModule("ImageCapture");
  }
  return log;
}

#endif

namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED(ImageCapture, DOMEventTargetHelper,
                                   mVideoStreamTrack)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(ImageCapture)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(ImageCapture, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(ImageCapture, DOMEventTargetHelper)

ImageCapture::ImageCapture(VideoStreamTrack* aVideoStreamTrack,
                           nsPIDOMWindow* aOwnerWindow)
  : DOMEventTargetHelper(aOwnerWindow)
{
  MOZ_ASSERT(aOwnerWindow);
  MOZ_ASSERT(aVideoStreamTrack);

  mVideoStreamTrack = aVideoStreamTrack;
}

ImageCapture::~ImageCapture()
{
  MOZ_ASSERT(NS_IsMainThread());
}

already_AddRefed<ImageCapture>
ImageCapture::Constructor(const GlobalObject& aGlobal,
                          VideoStreamTrack& aTrack,
                          ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(aGlobal.GetAsSupports());
  if (!win) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsRefPtr<ImageCapture> object = new ImageCapture(&aTrack, win);

  return object.forget();
}

VideoStreamTrack*
ImageCapture::GetVideoStreamTrack() const
{
  return mVideoStreamTrack;
}

nsresult
ImageCapture::TakePhotoByMediaEngine()
{
  
  
  class TakePhotoCallback : public MediaEngineSource::PhotoCallback,
                            public DOMMediaStream::PrincipalChangeObserver
  {
  public:
    TakePhotoCallback(DOMMediaStream* aStream, ImageCapture* aImageCapture)
      : mStream(aStream)
      , mImageCapture(aImageCapture)
      , mPrincipalChanged(false)
    {
      MOZ_ASSERT(NS_IsMainThread());
      mStream->AddPrincipalChangeObserver(this);
    }

    void PrincipalChanged(DOMMediaStream* aMediaStream) MOZ_OVERRIDE
    {
      mPrincipalChanged = true;
    }

    nsresult PhotoComplete(already_AddRefed<DOMFile> aBlob) MOZ_OVERRIDE
    {
      nsRefPtr<DOMFile> blob = aBlob;

      if (mPrincipalChanged) {
        return PhotoError(NS_ERROR_DOM_SECURITY_ERR);
      }
      return mImageCapture->PostBlobEvent(blob);
    }

    nsresult PhotoError(nsresult aRv) MOZ_OVERRIDE
    {
      return mImageCapture->PostErrorEvent(ImageCaptureError::PHOTO_ERROR, aRv);
    }

  protected:
    ~TakePhotoCallback()
    {
      MOZ_ASSERT(NS_IsMainThread());
      mStream->RemovePrincipalChangeObserver(this);
    }

    nsRefPtr<DOMMediaStream> mStream;
    nsRefPtr<ImageCapture> mImageCapture;
    bool mPrincipalChanged;
  };

  nsRefPtr<DOMMediaStream> domStream = mVideoStreamTrack->GetStream();
  DOMLocalMediaStream* domLocalStream = domStream->AsDOMLocalMediaStream();
  if (domLocalStream) {
    nsRefPtr<MediaEngineSource> mediaEngine =
      domLocalStream->GetMediaEngine(mVideoStreamTrack->GetTrackID());
    nsRefPtr<MediaEngineSource::PhotoCallback> callback =
      new TakePhotoCallback(domStream, this);
    return mediaEngine->TakePhoto(callback);
  }

  return NS_ERROR_NOT_IMPLEMENTED;
}

void
ImageCapture::TakePhoto(ErrorResult& aResult)
{
  
  
  
  
  
  if (!mVideoStreamTrack->Enabled()) {
    PostErrorEvent(ImageCaptureError::PHOTO_ERROR, NS_ERROR_FAILURE);
    return;
  }

  
  nsresult rv = TakePhotoByMediaEngine();

  
  
  if (rv == NS_ERROR_NOT_IMPLEMENTED) {
    IC_LOG("MediaEngine doesn't support TakePhoto(), it falls back to MediaStreamGraph.");
    nsRefPtr<CaptureTask> task =
      new CaptureTask(this, mVideoStreamTrack->GetTrackID());

    
    
    task->AttachStream();
  }
}

nsresult
ImageCapture::PostBlobEvent(nsIDOMBlob* aBlob)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!CheckPrincipal()) {
    
    return PostErrorEvent(ImageCaptureError::PHOTO_ERROR, NS_ERROR_DOM_SECURITY_ERR);
  }

  BlobEventInit init;
  init.mBubbles = false;
  init.mCancelable = false;
  init.mData = aBlob;

  nsRefPtr<BlobEvent> blob_event =
    BlobEvent::Constructor(this, NS_LITERAL_STRING("photo"), init);

  return DispatchTrustedEvent(blob_event);
}

nsresult
ImageCapture::PostErrorEvent(uint16_t aErrorCode, nsresult aReason)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsresult rv = CheckInnerWindowCorrectness();
  NS_ENSURE_SUCCESS(rv, rv);

  nsString errorMsg;
  if (NS_FAILED(aReason)) {
    nsCString name, message;
    rv = NS_GetNameAndMessageForDOMNSResult(aReason, name, message);
    if (NS_SUCCEEDED(rv)) {
      CopyASCIItoUTF16(message, errorMsg);
    }
  }

  nsRefPtr<ImageCaptureError> error =
    new ImageCaptureError(this, aErrorCode, errorMsg);

  ImageCaptureErrorEventInit init;
  init.mBubbles = false;
  init.mCancelable = false;
  init.mImageCaptureError = error;

  nsCOMPtr<nsIDOMEvent> event =
    ImageCaptureErrorEvent::Constructor(this, NS_LITERAL_STRING("error"), init);

  return DispatchTrustedEvent(event);
}

bool
ImageCapture::CheckPrincipal()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsRefPtr<DOMMediaStream> ms = mVideoStreamTrack->GetStream();
  if (!ms) {
    return false;
  }
  nsCOMPtr<nsIPrincipal> principal = ms->GetPrincipal();

  if (!GetOwner()) {
    return false;
  }
  nsCOMPtr<nsIDocument> doc = GetOwner()->GetExtantDoc();
  if (!doc || !principal) {
    return false;
  }

  bool subsumes;
  if (NS_FAILED(doc->NodePrincipal()->Subsumes(principal, &subsumes))) {
    return false;
  }

  return subsumes;
}

} 
} 

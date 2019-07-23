





































#include "imgContainerRequest.h"
#include "ImageErrors.h"
#include "imgRequest.h"

NS_IMPL_ISUPPORTS2(imgContainerRequest, imgIRequest, nsIRequest)

imgContainerRequest::imgContainerRequest(imgIContainer* aImage,
                                         nsIURI* aURI,
                                         PRUint32 aImageStatus,
                                         PRUint32 aState,
                                         nsIPrincipal* aPrincipal)
: mImage(aImage), mURI(aURI), mPrincipal(aPrincipal), mImageStatus(aImageStatus),
  mState(aState), mLocksHeld(0)
{
#ifdef DEBUG
  PRUint32 numFrames = 0;
  if (aImage) {
    aImage->GetNumFrames(&numFrames);
  }
  NS_ABORT_IF_FALSE(!aImage || numFrames == 1,
                    "Shouldn't have image with more than one frame!");
#endif
}

imgContainerRequest::~imgContainerRequest()
{
  if (mImage) {
    while (mLocksHeld) {
      UnlockImage();
    }
  }
}

NS_IMETHODIMP
imgContainerRequest::GetName(nsACString& aName)
{
  aName.Truncate();
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
imgContainerRequest::IsPending(PRBool* _retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
imgContainerRequest::GetStatus(nsresult* aStatus)
{
  *aStatus = NS_OK;
  return NS_OK;
}

NS_IMETHODIMP
imgContainerRequest::Cancel(nsresult aStatus)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
imgContainerRequest::Suspend()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
imgContainerRequest::Resume()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
imgContainerRequest::GetLoadGroup(nsILoadGroup** aLoadGroup)
{
  *aLoadGroup = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
imgContainerRequest::SetLoadGroup(nsILoadGroup* aLoadGroup)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
imgContainerRequest::GetLoadFlags(nsLoadFlags* aLoadFlags)
{
  *aLoadFlags = LOAD_NORMAL;
  return NS_OK;
}

NS_IMETHODIMP
imgContainerRequest::SetLoadFlags(nsLoadFlags aLoadFlags)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
imgContainerRequest::GetImage(imgIContainer** aImage)
{
  NS_IF_ADDREF(*aImage = mImage);
  return NS_OK;
}

NS_IMETHODIMP
imgContainerRequest::GetImageStatus(PRUint32* aImageStatus)
{
  *aImageStatus = mImageStatus;
  return NS_OK;
}

NS_IMETHODIMP
imgContainerRequest::GetURI(nsIURI** aURI)
{
  NS_IF_ADDREF(*aURI = mURI);
  return NS_OK;
}

NS_IMETHODIMP
imgContainerRequest::GetDecoderObserver(imgIDecoderObserver** aDecoderObserver)
{
  NS_IF_ADDREF(*aDecoderObserver = mObserver);
  return NS_OK;
}

NS_IMETHODIMP
imgContainerRequest::GetMimeType(char** aMimeType)
{
  *aMimeType = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
imgContainerRequest::Clone(imgIDecoderObserver* aObserver, imgIRequest** _retval)
{
  imgContainerRequest* req =
    new imgContainerRequest(mImage, mURI, mImageStatus, mState, mPrincipal);
  if (!req) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*_retval = req);
  req->mObserver = aObserver;

  nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(aObserver);

  

  
  if (req->mState & stateRequestStarted)
    aObserver->OnStartRequest(req);

  
  if (req->mState & stateHasSize)
    aObserver->OnStartContainer(req, req->mImage);

  
  if (req->mState & stateDecodeStarted)
    aObserver->OnStartDecode(req);

  
  PRUint32 nframes = 0;
  if (req->mImage)
    req->mImage->GetNumFrames(&nframes);

  if (nframes > 0) {
    PRUint32 frame;
    req->mImage->GetCurrentFrameIndex(&frame);
    aObserver->OnStartFrame(req, frame);

    
    
    
    nsIntRect r;
    req->mImage->GetCurrentFrameRect(r);
    aObserver->OnDataAvailable(req, frame, &r);

    if (req->mState & stateRequestStopped)
      aObserver->OnStopFrame(req, frame);
  }

  

  if (req->mState & stateRequestStopped) {
    aObserver->OnStopContainer(req, req->mImage);
    aObserver->OnStopDecode(req,
                            imgRequest::GetResultFromImageStatus(req->mImageStatus),
                            nsnull);
    aObserver->OnStopRequest(req, PR_TRUE);
  }

  return NS_OK;
}

NS_IMETHODIMP
imgContainerRequest::GetImagePrincipal(nsIPrincipal** aImagePrincipal)
{
  NS_IF_ADDREF(*aImagePrincipal = mPrincipal);
  return NS_OK;
}

NS_IMETHODIMP
imgContainerRequest::CancelAndForgetObserver(nsresult aStatus)
{
  return NS_OK;
}

NS_IMETHODIMP
imgContainerRequest::RequestDecode()
{
  return mImage ? mImage->RequestDecode() : NS_OK;
}

NS_IMETHODIMP
imgContainerRequest::LockImage()
{
  if (mImage) {
    ++mLocksHeld;
    return mImage->LockImage();
  }
  return NS_OK;
}

NS_IMETHODIMP
imgContainerRequest::UnlockImage()
{
  if (mImage) {
    NS_ABORT_IF_FALSE(mLocksHeld > 0, "calling unlock but no locks!");
    --mLocksHeld;
    return mImage->UnlockImage();
  }
  return NS_OK;
}

NS_IMETHODIMP
imgContainerRequest::GetStaticRequest(imgIRequest** aReturn)
{
  NS_ADDREF(*aReturn = this);
  return NS_OK;
}









































#include "imgRequestProxy.h"

#include "nsIInputStream.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIMultiPartChannel.h"

#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"

#include "ImageErrors.h"
#include "ImageLogging.h"

#include "nspr.h"


NS_IMPL_ISUPPORTS4(imgRequestProxy, imgIRequest, nsIRequest,
                   nsISupportsPriority, nsISecurityInfoProvider)

imgRequestProxy::imgRequestProxy() :
  mOwner(nsnull),
  mListener(nsnull),
  mLoadFlags(nsIRequest::LOAD_NORMAL),
  mCanceled(PR_FALSE),
  mIsInLoadGroup(PR_FALSE),
  mListenerIsStrongRef(PR_FALSE),
  mShouldRequestDecode(PR_FALSE),
  mLockHeld(PR_FALSE)
{
  

}

imgRequestProxy::~imgRequestProxy()
{
  
  NS_PRECONDITION(!mListener, "Someone forgot to properly cancel this request!");

  
  if (mLockHeld && mOwner)
    UnlockImage();

  
  
  
  
  NullOutListener();

  if (mOwner) {
    if (!mCanceled) {
      mCanceled = PR_TRUE;

      







      mOwner->RemoveProxy(this, NS_OK, PR_FALSE);
    }
  }
}

nsresult imgRequestProxy::Init(imgRequest *request, nsILoadGroup *aLoadGroup, imgIDecoderObserver *aObserver)
{
  NS_PRECONDITION(!mOwner && !mListener, "imgRequestProxy is already initialized");
  NS_PRECONDITION(request, "no request");
  if (!request)
    return NS_ERROR_NULL_POINTER;

  LOG_SCOPE_WITH_PARAM(gImgLog, "imgRequestProxy::Init", "request", request);

  mOwner = request;
  mListener = aObserver;
  
  
  
  if (mListener) {
    mListenerIsStrongRef = PR_TRUE;
    NS_ADDREF(mListener);
  }
  mLoadGroup = aLoadGroup;

  
  request->AddProxy(this);

  return NS_OK;
}

nsresult imgRequestProxy::ChangeOwner(imgRequest *aNewOwner)
{
  if (mCanceled)
    return NS_OK;

  
  PRBool wasDecoded = PR_FALSE;
  if (mOwner->GetImageStatus() & imgIRequest::STATUS_FRAME_COMPLETE)
    wasDecoded = PR_TRUE;

  
  PRBool wasLocked = mLockHeld;
  if (mLockHeld)
    UnlockImage();

  
  
  mOwner->RemoveProxy(this, NS_IMAGELIB_CHANGING_OWNER, PR_FALSE);

  mOwner = aNewOwner;

  mOwner->AddProxy(this);

  
  if (wasDecoded)
    RequestDecode();

  
  if (wasLocked)
    LockImage();

  return NS_OK;
}

void imgRequestProxy::AddToLoadGroup()
{
  NS_ASSERTION(!mIsInLoadGroup, "Whaa, we're already in the loadgroup!");

  if (!mIsInLoadGroup && mLoadGroup) {
    mLoadGroup->AddRequest(this, nsnull);
    mIsInLoadGroup = PR_TRUE;
  }
}

void imgRequestProxy::RemoveFromLoadGroup(PRBool releaseLoadGroup)
{
  if (!mIsInLoadGroup)
    return;

  




  nsCOMPtr<imgIRequest> kungFuDeathGrip(this);

  mLoadGroup->RemoveRequest(this, NS_OK, nsnull);
  mIsInLoadGroup = PR_FALSE;

  if (releaseLoadGroup) {
    
    mLoadGroup = nsnull;
  }
}





NS_IMETHODIMP imgRequestProxy::GetName(nsACString &aName)
{
  aName.Truncate();
  if (mOwner) {
    nsCOMPtr<nsIURI> uri;
    mOwner->GetURI(getter_AddRefs(uri));
    if (uri)
      uri->GetSpec(aName);
  }
  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::IsPending(PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP imgRequestProxy::GetStatus(nsresult *aStatus)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP imgRequestProxy::Cancel(nsresult status)
{
  if (mCanceled || !mOwner)
    return NS_ERROR_FAILURE;

  LOG_SCOPE(gImgLog, "imgRequestProxy::Cancel");

  mCanceled = PR_TRUE;

  nsCOMPtr<nsIRunnable> ev = new imgCancelRunnable(this, status);
  return NS_DispatchToCurrentThread(ev);
}

void
imgRequestProxy::DoCancel(nsresult status)
{
  
  
  mOwner->RemoveProxy(this, status, PR_FALSE);

  NullOutListener();
}


NS_IMETHODIMP imgRequestProxy::CancelAndForgetObserver(nsresult aStatus)
{
  if (mCanceled || !mOwner)
    return NS_ERROR_FAILURE;

  LOG_SCOPE(gImgLog, "imgRequestProxy::CancelAndForgetObserver");

  mCanceled = PR_TRUE;

  
  
  mOwner->RemoveProxy(this, aStatus, PR_FALSE);

  NullOutListener();

  return NS_OK;
}


NS_IMETHODIMP
imgRequestProxy::RequestDecode()
{
  if (!mOwner)
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<imgIContainer> container;
  nsresult rv = mOwner->GetImage(getter_AddRefs(container));
  if (NS_FAILED(rv))
    return rv;

  
  if (container)
    return container->RequestDecode();

  
  mShouldRequestDecode = PR_TRUE;
  return NS_OK;
}


NS_IMETHODIMP
imgRequestProxy::LockImage()
{
  if (!mOwner)
    return NS_ERROR_FAILURE;

  
  if (mLockHeld)
    return NS_OK;
  NS_ABORT_IF_FALSE(!mLockHeld, "Only call lockImage once per imgIRequest!");
  mLockHeld = PR_TRUE;

  
  
  nsCOMPtr<imgIContainer> container;
  nsresult rv = mOwner->GetImage(getter_AddRefs(container));
  if (NS_FAILED(rv))
    return rv;
  if (container)
    return container->LockImage();

  return NS_OK;
}


NS_IMETHODIMP
imgRequestProxy::UnlockImage()
{
  if (!mOwner)
    return NS_ERROR_FAILURE;

  
  NS_ABORT_IF_FALSE(mLockHeld, "calling unlock but not locked!");
  mLockHeld = PR_FALSE;

  
  
  nsCOMPtr<imgIContainer> container;
  nsresult rv = mOwner->GetImage(getter_AddRefs(container));
  if (NS_FAILED(rv))
    return rv;
  if (container)
    return container->UnlockImage();

  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::Suspend()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP imgRequestProxy::Resume()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP imgRequestProxy::GetLoadGroup(nsILoadGroup **loadGroup)
{
  NS_IF_ADDREF(*loadGroup = mLoadGroup.get());
  return NS_OK;
}
NS_IMETHODIMP imgRequestProxy::SetLoadGroup(nsILoadGroup *loadGroup)
{
  mLoadGroup = loadGroup;
  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::GetLoadFlags(nsLoadFlags *flags)
{
  *flags = mLoadFlags;
  return NS_OK;
}
NS_IMETHODIMP imgRequestProxy::SetLoadFlags(nsLoadFlags flags)
{
  mLoadFlags = flags;
  return NS_OK;
}




NS_IMETHODIMP imgRequestProxy::GetImage(imgIContainer * *aImage)
{
  if (!mOwner)
    return NS_ERROR_FAILURE;

  mOwner->GetImage(aImage);
  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::GetImageStatus(PRUint32 *aStatus)
{
  if (!mOwner) {
    *aStatus = imgIRequest::STATUS_ERROR;
    return NS_ERROR_FAILURE;
  }

  *aStatus = mOwner->GetImageStatus();
  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::GetURI(nsIURI **aURI)
{
  if (!mOwner)
    return NS_ERROR_FAILURE;

  return mOwner->GetURI(aURI);
}


NS_IMETHODIMP imgRequestProxy::GetDecoderObserver(imgIDecoderObserver **aDecoderObserver)
{
  *aDecoderObserver = mListener;
  NS_IF_ADDREF(*aDecoderObserver);
  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::GetMimeType(char **aMimeType)
{
  if (!mOwner)
    return NS_ERROR_FAILURE;

  const char *type = mOwner->GetMimeType();
  if (!type)
    return NS_ERROR_FAILURE;

  *aMimeType = NS_strdup(type);

  return NS_OK;
}

NS_IMETHODIMP imgRequestProxy::Clone(imgIDecoderObserver* aObserver,
                                     imgIRequest** aClone)
{
  NS_PRECONDITION(aClone, "Null out param");
  *aClone = nsnull;
  imgRequestProxy* clone = new imgRequestProxy();
  if (!clone) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ADDREF(clone);

  
  
  
  
  
  
  clone->SetLoadFlags(mLoadFlags);
  nsresult rv = clone->Init(mOwner, mLoadGroup, aObserver);
  if (NS_FAILED(rv)) {
    NS_RELEASE(clone);
    return rv;
  }

  
  
  
  *aClone = clone;

  
  mOwner->NotifyProxyListener(clone);

  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::GetImagePrincipal(nsIPrincipal **aPrincipal)
{
  if (!mOwner)
    return NS_ERROR_FAILURE;

  return mOwner->GetPrincipal(aPrincipal);
}



NS_IMETHODIMP imgRequestProxy::GetPriority(PRInt32 *priority)
{
  NS_ENSURE_STATE(mOwner);
  *priority = mOwner->Priority();
  return NS_OK;
}

NS_IMETHODIMP imgRequestProxy::SetPriority(PRInt32 priority)
{
  NS_ENSURE_STATE(mOwner && !mCanceled);
  mOwner->AdjustPriority(this, priority - mOwner->Priority());
  return NS_OK;
}

NS_IMETHODIMP imgRequestProxy::AdjustPriority(PRInt32 priority)
{
  NS_ENSURE_STATE(mOwner && !mCanceled);
  mOwner->AdjustPriority(this, priority);
  return NS_OK;
}



NS_IMETHODIMP imgRequestProxy::GetSecurityInfo(nsISupports** _retval)
{
  if (mOwner)
    return mOwner->GetSecurityInfo(_retval);

  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP imgRequestProxy::GetHasTransferredData(PRBool* hasData)
{
  if (mOwner) {
    *hasData = mOwner->HasTransferredData();
  } else {
    
    *hasData = PR_TRUE;
  }
  return NS_OK;
}



void imgRequestProxy::FrameChanged(imgIContainer *container, nsIntRect * dirtyRect)
{
  LOG_FUNC(gImgLog, "imgRequestProxy::FrameChanged");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->FrameChanged(container, dirtyRect);
  }
}



void imgRequestProxy::OnStartDecode()
{
  LOG_FUNC(gImgLog, "imgRequestProxy::OnStartDecode");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnStartDecode(this);
  }
}

void imgRequestProxy::OnStartContainer(imgIContainer *image)
{
  LOG_FUNC(gImgLog, "imgRequestProxy::OnStartContainer");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnStartContainer(this, image);
  }

  
  if (mShouldRequestDecode) {
    image->RequestDecode();
    mShouldRequestDecode = PR_FALSE;
  }

  
  if (mLockHeld)
    image->LockImage();
}

void imgRequestProxy::OnStartFrame(PRUint32 frame)
{
  LOG_FUNC(gImgLog, "imgRequestProxy::OnStartFrame");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnStartFrame(this, frame);
  }
}

void imgRequestProxy::OnDataAvailable(PRBool aCurrentFrame, const nsIntRect * rect)
{
  LOG_FUNC(gImgLog, "imgRequestProxy::OnDataAvailable");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnDataAvailable(this, aCurrentFrame, rect);
  }
}

void imgRequestProxy::OnStopFrame(PRUint32 frame)
{
  LOG_FUNC(gImgLog, "imgRequestProxy::OnStopFrame");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnStopFrame(this, frame);
  }
}

void imgRequestProxy::OnStopContainer(imgIContainer *image)
{
  LOG_FUNC(gImgLog, "imgRequestProxy::OnStopContainer");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnStopContainer(this, image);
  }
}

void imgRequestProxy::OnStopDecode(nsresult status, const PRUnichar *statusArg)
{
  LOG_FUNC(gImgLog, "imgRequestProxy::OnStopDecode");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnStopDecode(this, status, statusArg);
  }
}

void imgRequestProxy::OnDiscard()
{
  LOG_FUNC(gImgLog, "imgRequestProxy::OnDiscard");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnDiscard(this);
  }
}




void imgRequestProxy::OnStartRequest(nsIRequest *request, nsISupports *ctxt)
{
#ifdef PR_LOGGING
  nsCAutoString name;
  GetName(name);
  LOG_FUNC_WITH_PARAM(gImgLog, "imgRequestProxy::OnStartRequest", "name", name.get());
#endif

  
  
  if (mListener) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnStartRequest(this);
  }
}

void imgRequestProxy::OnStopRequest(nsIRequest *request, nsISupports *ctxt,
                                    nsresult statusCode, PRBool lastPart)
{
#ifdef PR_LOGGING
  nsCAutoString name;
  GetName(name);
  LOG_FUNC_WITH_PARAM(gImgLog, "imgRequestProxy::OnStopRequest", "name", name.get());
#endif
  
  
  
  nsCOMPtr<imgIRequest> kungFuDeathGrip(this);

  if (mListener) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnStopRequest(this, lastPart);
  }

  
  
  
  
  if (lastPart || (mLoadFlags & nsIRequest::LOAD_BACKGROUND) == 0) {
    RemoveFromLoadGroup(lastPart);
    
    
    if (!lastPart) {
      mLoadFlags |= nsIRequest::LOAD_BACKGROUND;
      AddToLoadGroup();
    }
  }

  if (mListenerIsStrongRef) {
    NS_PRECONDITION(mListener, "How did that happen?");
    
    
    
    imgIDecoderObserver* obs = mListener;
    mListenerIsStrongRef = PR_FALSE;
    NS_RELEASE(obs);
  }
}

void imgRequestProxy::NullOutListener()
{
  if (mListenerIsStrongRef) {
    
    nsCOMPtr<imgIDecoderObserver> obs;
    obs.swap(mListener);
    mListenerIsStrongRef = PR_FALSE;
  } else {
    mListener = nsnull;
  }
}

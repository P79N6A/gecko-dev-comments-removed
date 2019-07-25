







#include "nsImageLoader.h"

#include "imgILoader.h"

#include "nsIURI.h"
#include "nsILoadGroup.h"
#include "nsNetUtil.h"

#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIFrame.h"
#include "nsIContent.h"
#include "nsIDocument.h"

#include "imgIContainer.h"

#include "nsStyleContext.h"
#include "nsGkAtoms.h"
#include "nsLayoutUtils.h"


#include "prenv.h"

NS_IMPL_ISUPPORTS2(nsImageLoader, imgIDecoderObserver, imgIContainerObserver)

nsImageLoader::nsImageLoader(nsIFrame *aFrame, PRUint32 aActions,
                             nsImageLoader *aNextLoader)
  : mFrame(aFrame),
    mActions(aActions),
    mNextLoader(aNextLoader),
    mRequestRegistered(false)
{
}

nsImageLoader::~nsImageLoader()
{
  Destroy();
}

 already_AddRefed<nsImageLoader>
nsImageLoader::Create(nsIFrame *aFrame, imgIRequest *aRequest, 
                      PRUint32 aActions, nsImageLoader *aNextLoader)
{
  nsRefPtr<nsImageLoader> loader =
    new nsImageLoader(aFrame, aActions, aNextLoader);

  loader->Load(aRequest);

  return loader.forget();
}

void
nsImageLoader::Destroy()
{
  
  nsRefPtr<nsImageLoader> list = mNextLoader;
  mNextLoader = nsnull;
  while (list) {
    nsRefPtr<nsImageLoader> todestroy = list;
    list = todestroy->mNextLoader;
    todestroy->mNextLoader = nsnull;
    todestroy->Destroy();
  }

  if (mRequest && mFrame) {
    nsLayoutUtils::DeregisterImageRequest(mFrame->PresContext(), mRequest,
                                          &mRequestRegistered);
  }

  mFrame = nsnull;

  if (mRequest) {
    mRequest->CancelAndForgetObserver(NS_ERROR_FAILURE);
  }

  mRequest = nsnull;
}

nsresult
nsImageLoader::Load(imgIRequest *aImage)
{
  NS_ASSERTION(!mRequest, "can't reuse image loaders");
  NS_ASSERTION(mFrame, "not initialized");
  NS_ASSERTION(aImage, "must have non-null image");

  if (!mFrame)
    return NS_ERROR_NOT_INITIALIZED;

  if (!aImage)
    return NS_ERROR_FAILURE;

  
  
  nsPresContext* presContext = mFrame->PresContext();

  nsLayoutUtils::DeregisterImageRequest(presContext, mRequest,
                                        &mRequestRegistered);

  
  
  
  nsCOMPtr<imgIRequest> newRequest;
  nsresult rv = aImage->Clone(this, getter_AddRefs(newRequest));
  mRequest.swap(newRequest);

  if (mRequest) {
    nsLayoutUtils::RegisterImageRequestIfAnimated(presContext, mRequest,
                                                  &mRequestRegistered);
  }

  return rv;
}

NS_IMETHODIMP nsImageLoader::OnStartContainer(imgIRequest *aRequest,
                                              imgIContainer *aImage)
{
  NS_ABORT_IF_FALSE(aImage, "Who's calling us then?");

  




  aImage->SetAnimationMode(mFrame->PresContext()->ImageAnimationMode());

  return NS_OK;
}

NS_IMETHODIMP nsImageLoader::OnStopFrame(imgIRequest *aRequest,
                                         PRUint32 aFrame)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  if (!mRequest) {
    
    return NS_OK;
  }

  
  if (mActions & ACTION_REDRAW_ON_DECODE) {
    DoRedraw(nsnull);
  }
  return NS_OK;
}

NS_IMETHODIMP nsImageLoader::OnImageIsAnimated(imgIRequest *aRequest)
{
  
  
  nsLayoutUtils::RegisterImageRequest(mFrame->PresContext(),
                                      aRequest, &mRequestRegistered);
  return NS_OK;
}

NS_IMETHODIMP nsImageLoader::OnStopRequest(imgIRequest *aRequest,
                                           bool aLastPart)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  if (!mRequest) {
    
    return NS_OK;
  }

  
  if (mActions & ACTION_REDRAW_ON_LOAD) {
    DoRedraw(nsnull);
  }
  return NS_OK;
}

NS_IMETHODIMP nsImageLoader::FrameChanged(imgIRequest *aRequest,
                                          imgIContainer *aContainer,
                                          const nsIntRect *aDirtyRect)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  if (!mRequest) {
    
    return NS_OK;
  }

  NS_ASSERTION(aRequest == mRequest, "This is a neat trick.");

  nsRect r = aDirtyRect->IsEqualInterior(nsIntRect::GetMaxSizedIntRect()) ?
    nsRect(nsPoint(0, 0), mFrame->GetSize()) :
    aDirtyRect->ToAppUnits(nsPresContext::AppUnitsPerCSSPixel());

  DoRedraw(&r);

  return NS_OK;
}

void
nsImageLoader::DoRedraw(const nsRect* aDamageRect)
{
  
  
  
  
  
  

  
  if (mFrame->GetStyleVisibility()->IsVisible()) {
    mFrame->InvalidateFrame();
  }
}

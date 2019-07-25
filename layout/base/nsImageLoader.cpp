








































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
  mFrame = nsnull;

  if (mRequest) {
    mRequest->CancelAndForgetObserver(NS_ERROR_FAILURE);
  }
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

  if (mRequest) {
    nsPresContext* presContext = mFrame->PresContext();

    nsLayoutUtils::DeregisterImageRequest(presContext, mRequest,
                                          &mRequestRegistered);
    mRequest->CancelAndForgetObserver(NS_ERROR_FAILURE);
  }

  mFrame = nsnull;
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

  
  
  
  nsCOMPtr<imgIRequest> newRequest;
  nsresult rv = aImage->Clone(this, getter_AddRefs(newRequest));
  mRequest.swap(newRequest);
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

  
  if (mActions & ACTION_REFLOW_ON_DECODE) {
    DoReflow();
  }
  if (mActions & ACTION_REDRAW_ON_DECODE) {
    DoRedraw(nsnull);
  }
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

  
  if (mActions & ACTION_REFLOW_ON_LOAD) {
    DoReflow();
  }
  if (mActions & ACTION_REDRAW_ON_LOAD) {
    DoRedraw(nsnull);
  }
  return NS_OK;
}

NS_IMETHODIMP nsImageLoader::FrameChanged(imgIContainer *aContainer,
                                          const nsIntRect *aDirtyRect)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  if (!mRequest) {
    
    return NS_OK;
  }

  nsRect r = aDirtyRect->IsEqualInterior(nsIntRect::GetMaxSizedIntRect()) ?
    nsRect(nsPoint(0, 0), mFrame->GetSize()) :
    aDirtyRect->ToAppUnits(nsPresContext::AppUnitsPerCSSPixel());

  DoRedraw(&r);

  return NS_OK;
}

void
nsImageLoader::DoReflow()
{
  nsIPresShell *shell = mFrame->PresContext()->GetPresShell();
  shell->FrameNeedsReflow(mFrame, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
}

void
nsImageLoader::DoRedraw(const nsRect* aDamageRect)
{
  
  
  
  
  
  

  
  

  nsRect bounds(nsPoint(0, 0), mFrame->GetSize());

  if (mFrame->GetType() == nsGkAtoms::canvasFrame) {
    
    bounds = mFrame->GetVisualOverflowRect();
  }

  
  
#if 0
  
  
  
  nsStyleContext* styleContext;
  mFrame->GetStyleContext(&styleContext);
  const nsStyleBackground* bg = styleContext->GetStyleBackground();

  if ((bg->mBackgroundFlags & NS_STYLE_BG_IMAGE_NONE) ||
      (bg->mBackgroundRepeat == NS_STYLE_BG_REPEAT_OFF)) {
    
    
    

    if (aDamageRect) {
      bounds.IntersectRect(*aDamageRect, bounds);
    }
  }

#endif

  if (mFrame->GetStyleVisibility()->IsVisible()) {
    mFrame->Invalidate(bounds);
  }
}

NS_IMETHODIMP
nsImageLoader::OnStartDecode(imgIRequest *aRequest)
{
  
  nsPresContext* presContext = mFrame->PresContext();
  if (!presContext) {
    return NS_OK;
  }

  nsLayoutUtils::RegisterImageRequest(presContext, aRequest,
                                      &mRequestRegistered);

  return NS_OK;
}

NS_IMETHODIMP
nsImageLoader::OnStopDecode(imgIRequest *aRequest, nsresult status,
                            const PRUnichar *statusArg)
{
  
  
  nsLayoutUtils::DeregisterImageRequestIfNotAnimated(mFrame->PresContext(),
                                                     mRequest,
                                                     &mRequestRegistered);

  return NS_OK;
}

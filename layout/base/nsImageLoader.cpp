








































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


#include "prenv.h"

NS_IMPL_ISUPPORTS2(nsImageLoader, imgIDecoderObserver, imgIContainerObserver)

nsImageLoader::nsImageLoader(nsIFrame *aFrame, PRUint32 aActions,
                             nsImageLoader *aNextLoader)
  : mFrame(aFrame),
    mActions(aActions),
    mNextLoader(aNextLoader)
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
  
  aImage->StartAnimation();

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
                                           PRBool aLastPart)
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
}

NS_IMETHODIMP nsImageLoader::FrameChanged(imgIContainer *aContainer,
                                          nsIntRect *dirtyRect)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  if (!mRequest) {
    
    return NS_OK;
  }
  
  nsRect r = dirtyRect->ToAppUnits(nsPresContext::AppUnitsPerCSSPixel());

  DoRedraw(&r);

  return NS_OK;
}

void
nsImageLoader::DoReflow()
{
  nsIPresShell *shell = mFrame->PresContext()->GetPresShell();
#ifdef DEBUG
  nsresult rv =
#endif
    shell->FrameNeedsReflow(mFrame, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Could not reflow after loading border-image");
}

void
nsImageLoader::DoRedraw(const nsRect* aDamageRect)
{
  
  
  
  
  
  

  
  

  nsRect bounds(nsPoint(0, 0), mFrame->GetSize());

  if (mFrame->GetType() == nsGkAtoms::canvasFrame) {
    
    bounds = mFrame->GetOverflowRect();
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










































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

nsImageLoader::nsImageLoader() :
  mFrame(nsnull), mPresContext(nsnull)
{
}

nsImageLoader::~nsImageLoader()
{
  mFrame = nsnull;
  mPresContext = nsnull;

  if (mRequest) {
    mRequest->Cancel(NS_ERROR_FAILURE);
  }
}


void
nsImageLoader::Init(nsIFrame *aFrame, nsPresContext *aPresContext)
{
  mFrame = aFrame;
  mPresContext = aPresContext;
}

void
nsImageLoader::Destroy()
{
  mFrame = nsnull;
  mPresContext = nsnull;

  if (mRequest) {
    mRequest->Cancel(NS_ERROR_FAILURE);
  }

  mRequest = nsnull;
}

nsresult
nsImageLoader::Load(imgIRequest *aImage)
{
  if (!mFrame)
    return NS_ERROR_NOT_INITIALIZED;

  if (!aImage)
    return NS_ERROR_FAILURE;

  if (mRequest) {
    nsCOMPtr<nsIURI> oldURI;
    mRequest->GetURI(getter_AddRefs(oldURI));
    nsCOMPtr<nsIURI> newURI;
    aImage->GetURI(getter_AddRefs(newURI));
    PRBool eq = PR_FALSE;
    nsresult rv = newURI->Equals(oldURI, &eq);
    if (NS_SUCCEEDED(rv) && eq) {
      return NS_OK;
    }

    
    mRequest->Cancel(NS_ERROR_FAILURE);
    mRequest = nsnull;
  }

  
  
  
  nsCOMPtr<imgIRequest> newRequest;
  nsresult rv = aImage->Clone(this, getter_AddRefs(newRequest));
  mRequest.swap(newRequest);
  return rv;
}

                    

NS_IMETHODIMP nsImageLoader::OnStartContainer(imgIRequest *aRequest,
                                              imgIContainer *aImage)
{
  if (aImage)
  {
    




    aImage->SetAnimationMode(mPresContext->ImageAnimationMode());
    
    aImage->StartAnimation();
  }
  return NS_OK;
}

NS_IMETHODIMP nsImageLoader::OnStopFrame(imgIRequest *aRequest,
                                         gfxIImageFrame *aFrame)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;
  
#ifdef NS_DEBUG


  if (aRequest) {
   PRUint32 status = imgIRequest::STATUS_ERROR;
   nsresult rv = aRequest->GetImageStatus(&status);
   if (NS_SUCCEEDED(rv)) {
     NS_ASSERTION((status & imgIRequest::STATUS_FRAME_COMPLETE), "imgIRequest::STATUS_FRAME_COMPLETE not set");
   }
  }
#endif

  if (!mRequest) {
    
    return NS_OK;
  }
  
  
  RedrawDirtyFrame(nsnull);
  return NS_OK;
}

NS_IMETHODIMP nsImageLoader::FrameChanged(imgIContainer *aContainer,
                                          gfxIImageFrame *newframe,
                                          nsRect * dirtyRect)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  if (!mRequest) {
    
    return NS_OK;
  }
  
  nsRect r(*dirtyRect);

  r.x = nsPresContext::CSSPixelsToAppUnits(r.x);
  r.y = nsPresContext::CSSPixelsToAppUnits(r.y);
  r.width = nsPresContext::CSSPixelsToAppUnits(r.width);
  r.height = nsPresContext::CSSPixelsToAppUnits(r.height);

  RedrawDirtyFrame(&r);

  return NS_OK;
}


void
nsImageLoader::RedrawDirtyFrame(const nsRect* aDamageRect)
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

  mFrame->Invalidate(bounds, PR_FALSE);
}

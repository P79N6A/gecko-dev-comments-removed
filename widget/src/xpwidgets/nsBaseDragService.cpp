





































#include "nsBaseDragService.h"
#include "nsITransferable.h"

#include "nsIServiceManager.h"
#include "nsITransferable.h"
#include "nsISupportsArray.h"
#include "nsSize.h"
#include "nsIRegion.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsCOMPtr.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIFrame.h"
#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsIPresShell.h"
#include "nsIViewManager.h"
#include "nsIScrollableView.h"
#include "nsIDOMNode.h"
#include "nsIDOMMouseEvent.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsPresContext.h"
#include "nsIEventStateManager.h"
#include "nsICanvasElement.h"
#include "nsIImage.h"
#include "nsIImageLoadingContent.h"
#include "gfxIImageFrame.h"
#include "imgIContainer.h"
#include "imgIRequest.h"
#include "nsIViewObserver.h"
#include "nsRegion.h"

#ifdef MOZ_CAIRO_GFX
#include "gfxContext.h"
#include "gfxImageSurface.h"

#endif

NS_IMPL_ADDREF(nsBaseDragService)
NS_IMPL_RELEASE(nsBaseDragService)
NS_IMPL_QUERY_INTERFACE2(nsBaseDragService, nsIDragService, nsIDragSession)







nsBaseDragService::nsBaseDragService()
  : mCanDrop(PR_FALSE), mDoingDrag(PR_FALSE), mHasImage(PR_FALSE),
    mDragAction(DRAGDROP_ACTION_NONE), mTargetSize(0,0),
    mImageX(0), mImageY(0), mScreenX(-1), mScreenY(-1)
{
}






nsBaseDragService::~nsBaseDragService()
{
}



NS_IMETHODIMP
nsBaseDragService::SetCanDrop(PRBool aCanDrop)
{
  mCanDrop = aCanDrop;
  return NS_OK;
}


NS_IMETHODIMP
nsBaseDragService::GetCanDrop(PRBool * aCanDrop)
{
  *aCanDrop = mCanDrop;
  return NS_OK;
}


NS_IMETHODIMP
nsBaseDragService::SetDragAction(PRUint32 anAction)
{
  mDragAction = anAction;
  return NS_OK;
}


NS_IMETHODIMP
nsBaseDragService::GetDragAction(PRUint32 * anAction)
{
  *anAction = mDragAction;
  return NS_OK;
}


NS_IMETHODIMP
nsBaseDragService::SetTargetSize(nsSize aDragTargetSize)
{
  mTargetSize = aDragTargetSize;
  return NS_OK;
}


NS_IMETHODIMP
nsBaseDragService::GetTargetSize(nsSize * aDragTargetSize)
{
  *aDragTargetSize = mTargetSize;
  return NS_OK;
}



NS_IMETHODIMP
nsBaseDragService::GetNumDropItems(PRUint32 * aNumItems)
{
  *aNumItems = 0;
  return NS_ERROR_FAILURE;
}








NS_IMETHODIMP
nsBaseDragService::GetSourceDocument(nsIDOMDocument** aSourceDocument)
{
  *aSourceDocument = mSourceDocument.get();
  NS_IF_ADDREF(*aSourceDocument);

  return NS_OK;
}







NS_IMETHODIMP
nsBaseDragService::GetSourceNode(nsIDOMNode** aSourceNode)
{
  *aSourceNode = mSourceNode.get();
  NS_IF_ADDREF(*aSourceNode);

  return NS_OK;
}




NS_IMETHODIMP
nsBaseDragService::GetData(nsITransferable * aTransferable,
                           PRUint32 aItemIndex)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsBaseDragService::IsDataFlavorSupported(const char *aDataFlavor,
                                         PRBool *_retval)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsBaseDragService::InvokeDragSession(nsIDOMNode *aDOMNode,
                                     nsISupportsArray* aTransferableArray,
                                     nsIScriptableRegion* aDragRgn,
                                     PRUint32 aActionType)
{
  NS_ENSURE_TRUE(aDOMNode, NS_ERROR_INVALID_ARG);

  
  aDOMNode->GetOwnerDocument(getter_AddRefs(mSourceDocument));
  mSourceNode = aDOMNode;

  
  
  
  
  
  nsCOMPtr<nsIContent> contentNode = do_QueryInterface(aDOMNode);
  if (contentNode) {
    nsIDocument* doc = contentNode->GetCurrentDoc();
    if (doc) {
      nsIPresShell* presShell = doc->GetShellAt(0);
      if (presShell) {
        nsIViewManager* vm = presShell->GetViewManager();
        if (vm) {
          PRBool notUsed;
          vm->GrabMouseEvents(nsnull, notUsed);
        }
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsBaseDragService::InvokeDragSessionWithImage(nsIDOMNode* aDOMNode,
                                              nsISupportsArray* aTransferableArray,
                                              nsIScriptableRegion* aRegion,
                                              PRUint32 aActionType,
                                              nsIDOMNode* aImage,
                                              PRInt32 aImageX, PRInt32 aImageY,
                                              nsIDOMMouseEvent* aDragEvent)
{
  NS_ENSURE_TRUE(aDragEvent, NS_ERROR_NULL_POINTER);

  mSelection = nsnull;
  mHasImage = PR_TRUE;
  mImage = aImage;
  mImageX = aImageX;
  mImageY = aImageY;

  aDragEvent->GetScreenX(&mScreenX);
  aDragEvent->GetScreenY(&mScreenY);

  return InvokeDragSession(aDOMNode, aTransferableArray, aRegion, aActionType);
}

NS_IMETHODIMP
nsBaseDragService::InvokeDragSessionWithSelection(nsISelection* aSelection,
                                                  nsISupportsArray* aTransferableArray,
                                                  PRUint32 aActionType,
                                                  nsIDOMMouseEvent* aDragEvent)
{
  NS_ENSURE_TRUE(aSelection, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aDragEvent, NS_ERROR_NULL_POINTER);

  mSelection = aSelection;
  mHasImage = PR_TRUE;
  mImage = nsnull;
  mImageX = 0;
  mImageY = 0;

  aDragEvent->GetScreenX(&mScreenX);
  aDragEvent->GetScreenY(&mScreenY);

  
  nsCOMPtr<nsIDOMNode> node;
  aSelection->GetFocusNode(getter_AddRefs(node));

  return InvokeDragSession(node, aTransferableArray, nsnull, aActionType);
}


NS_IMETHODIMP
nsBaseDragService::GetCurrentSession(nsIDragSession ** aSession)
{
  if (!aSession)
    return NS_ERROR_INVALID_ARG;

  
  
  if (mDoingDrag) {
    *aSession = this;
    NS_ADDREF(*aSession);      
  }
  else
    *aSession = nsnull;

  return NS_OK;
}


NS_IMETHODIMP
nsBaseDragService::StartDragSession()
{
  if (mDoingDrag) {
    return NS_ERROR_FAILURE;
  }
  mDoingDrag = PR_TRUE;
  return NS_OK;
}


NS_IMETHODIMP
nsBaseDragService::EndDragSession()
{
  if (!mDoingDrag) {
    return NS_ERROR_FAILURE;
  }

  mDoingDrag = PR_FALSE;

  
  mSourceDocument = nsnull;
  mSourceNode = nsnull;
  mSelection = nsnull;
  mHasImage = PR_FALSE;
  mImage = nsnull;
  mImageX = 0;
  mImageY = 0;
  mScreenX = -1;
  mScreenY = -1;

  return NS_OK;
}

#ifdef MOZ_CAIRO_GFX

static nsIPresShell*
GetPresShellForContent(nsIDOMNode* aDOMNode)
{
  nsIContent* content;
  CallQueryInterface(aDOMNode, &content);

  nsIDocument *document = content->GetCurrentDoc();
  if (document) {
    document->FlushPendingNotifications(Flush_Display);

    return document->GetShellAt(0);
  }

  return nsnull;
}

nsresult
nsBaseDragService::DrawDrag(nsIDOMNode* aDOMNode,
                            nsIScriptableRegion* aRegion,
                            PRInt32 aScreenX, PRInt32 aScreenY,
                            nsRect* aScreenDragRect,
                            gfxASurface** aSurface)
{
  *aSurface = nsnull;

  
  aScreenDragRect->x = aScreenX - mImageX;
  aScreenDragRect->y = aScreenY - mImageY;
  aScreenDragRect->width = 20;
  aScreenDragRect->height = 20;

  
  nsCOMPtr<nsIDOMNode> dragNode = mImage ? mImage.get() : aDOMNode;

  
  
  nsIPresShell* presShell = GetPresShellForContent(dragNode);
  if (!presShell && mImage)
    presShell = GetPresShellForContent(aDOMNode);
  if (!presShell)
    return NS_ERROR_FAILURE;

  
  if (!mHasImage) {
    
    
    if (aRegion) {
      
      nsPresContext* pc = presShell->GetPresContext();
      nsIFrame* rootFrame = presShell->GetRootFrame();
      if (rootFrame && pc) {
        nsRect dragRect;
        aRegion->GetBoundingBox(&dragRect.x, &dragRect.y, &dragRect.width, &dragRect.height);
        dragRect.ScaleRoundOut(nsPresContext::AppUnitsPerCSSPixel());
        dragRect.ScaleRoundOut(1.0 / pc->AppUnitsPerDevPixel());

        nsIntRect screenRect = rootFrame->GetScreenRectExternal();
        aScreenDragRect->SetRect(screenRect.x + dragRect.x, screenRect.y + dragRect.y,
                                 dragRect.width, dragRect.height);
      }
    }
    else {
      
      
      nsCOMPtr<nsIContent> content = do_QueryInterface(dragNode);
      nsIFrame* frame = presShell->GetPrimaryFrameFor(content);
      if (frame) {
        nsIntRect screenRect = frame->GetScreenRectExternal();
        aScreenDragRect->SetRect(screenRect.x, screenRect.y,
                                 screenRect.width, screenRect.height);
      }
    }

    return NS_OK;
  }

  
  if (mSelection) {
    nsPoint pnt(aScreenDragRect->x, aScreenDragRect->y);
    nsRefPtr<gfxASurface> surface = presShell->RenderSelection(mSelection, pnt, aScreenDragRect);
    *aSurface = surface;
    NS_IF_ADDREF(*aSurface);
    return NS_OK;
  }

  
  
  
  if (mImage) {
    nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(dragNode);
    
    if (imageLoader) {
      nsPresContext* pc = presShell->GetPresContext();
      if (!pc)
        return NS_ERROR_FAILURE;

      return DrawDragForImage(pc, imageLoader, aScreenX, aScreenY,
                              aScreenDragRect, aSurface);
    }
  }

  
  nsCOMPtr<nsIRegion> clipRegion;
  if (aRegion)
    aRegion->GetRegion(getter_AddRefs(clipRegion));

  nsPoint pnt(aScreenDragRect->x, aScreenDragRect->y);
  nsRefPtr<gfxASurface> surface = presShell->RenderNode(dragNode, clipRegion,
                                                        pnt, aScreenDragRect);

  
  
  if (mImage) {
    aScreenDragRect->x = aScreenX - mImageX;
    aScreenDragRect->y = aScreenY - mImageY;
  }

  *aSurface = surface;
  NS_IF_ADDREF(*aSurface);

  return NS_OK;
}

nsresult
nsBaseDragService::DrawDragForImage(nsPresContext* aPresContext,
                                    nsIImageLoadingContent* aImageLoader,
                                    PRInt32 aScreenX, PRInt32 aScreenY,
                                    nsRect* aScreenDragRect,
                                    gfxASurface** aSurface)
{
  nsCOMPtr<imgIRequest> imgRequest;
  nsresult rv = aImageLoader->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                                        getter_AddRefs(imgRequest));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!imgRequest)
    return NS_ERROR_NOT_AVAILABLE;

  nsCOMPtr<imgIContainer> imgContainer;
  rv = imgRequest->GetImage(getter_AddRefs(imgContainer));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!imgContainer)
    return NS_ERROR_NOT_AVAILABLE;

  
  imgContainer->GetWidth(&aScreenDragRect->width);
  imgContainer->GetHeight(&aScreenDragRect->height);

  nsRect srcRect = *aScreenDragRect;
  srcRect.MoveTo(0, 0);
  nsRect destRect = srcRect;

  if (destRect.width == 0 || destRect.height == 0)
    return NS_ERROR_FAILURE;

  
  
  nsIDeviceContext* deviceContext = aPresContext->DeviceContext();
  nsRect maxSize;
  deviceContext->GetClientRect(maxSize);
  nscoord maxWidth = aPresContext->AppUnitsToDevPixels(maxSize.width >> 1);
  nscoord maxHeight = aPresContext->AppUnitsToDevPixels(maxSize.height >> 1);
  if (destRect.width > maxWidth || destRect.height > maxHeight) {
    float scale = 1.0;
    if (destRect.width > maxWidth)
      scale = PR_MIN(scale, float(maxWidth) / destRect.width);
    if (destRect.height > maxHeight)
      scale = PR_MIN(scale, float(maxHeight) / destRect.height);

    destRect.width = NSToIntFloor(float(destRect.width) * scale);
    destRect.height = NSToIntFloor(float(destRect.height) * scale);

    aScreenDragRect->x = NSToIntFloor(aScreenX - float(mImageX) * scale);
    aScreenDragRect->y = NSToIntFloor(aScreenY - float(mImageY) * scale);
    aScreenDragRect->width = destRect.width;
    aScreenDragRect->height = destRect.height;
  }

  nsRefPtr<gfxImageSurface> surface =
    new gfxImageSurface(gfxIntSize(destRect.width, destRect.height),
                        gfxImageSurface::ImageFormatARGB32);
  if (!surface)
    return NS_ERROR_FAILURE;

  *aSurface = surface;
  NS_ADDREF(*aSurface);

  nsCOMPtr<nsIRenderingContext> rc;
  deviceContext->CreateRenderingContextInstance(*getter_AddRefs(rc));
  rc->Init(deviceContext, surface);

  
  gfxContext context(surface);
  context.SetOperator(gfxContext::OPERATOR_CLEAR);
  context.Rectangle(gfxRect(0, 0, destRect.width, destRect.height));
  context.Fill();

  PRInt32 upp = aPresContext->AppUnitsPerDevPixel();
  srcRect.ScaleRoundOut(upp);
  destRect.ScaleRoundOut(upp);
  return rc->DrawImage(imgContainer, srcRect, destRect);
}

#endif







































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
#include "nsIDOMDragEvent.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsPresContext.h"
#include "nsIDOMDataTransfer.h"
#include "nsIEventStateManager.h"
#include "nsICanvasElement.h"
#include "nsIImageLoadingContent.h"
#include "imgIContainer.h"
#include "imgIRequest.h"
#include "nsIViewObserver.h"
#include "nsRegion.h"
#include "nsGUIEvent.h"
#include "nsIPrefService.h"

#include "gfxContext.h"
#include "gfxPlatform.h"

#define DRAGIMAGES_PREF "nglayout.enable_drag_images"

nsBaseDragService::nsBaseDragService()
  : mCanDrop(PR_FALSE), mDoingDrag(PR_FALSE), mHasImage(PR_FALSE), mUserCancelled(PR_FALSE),
    mDragAction(DRAGDROP_ACTION_NONE), mTargetSize(0,0),
    mImageX(0), mImageY(0), mScreenX(-1), mScreenY(-1), mSuppressLevel(0)
{
}

nsBaseDragService::~nsBaseDragService()
{
}

NS_IMPL_ISUPPORTS2(nsBaseDragService, nsIDragService, nsIDragSession)


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
nsBaseDragService::GetDataTransfer(nsIDOMDataTransfer** aDataTransfer)
{
  *aDataTransfer = mDataTransfer;
  NS_IF_ADDREF(*aDataTransfer);
  return NS_OK;
}

NS_IMETHODIMP
nsBaseDragService::SetDataTransfer(nsIDOMDataTransfer* aDataTransfer)
{
  mDataTransfer = aDataTransfer;
  return NS_OK;
}


NS_IMETHODIMP
nsBaseDragService::InvokeDragSession(nsIDOMNode *aDOMNode,
                                     nsISupportsArray* aTransferableArray,
                                     nsIScriptableRegion* aDragRgn,
                                     PRUint32 aActionType)
{
  NS_ENSURE_TRUE(aDOMNode, NS_ERROR_INVALID_ARG);
  NS_ENSURE_TRUE(mSuppressLevel == 0, NS_ERROR_FAILURE);

  
  aDOMNode->GetOwnerDocument(getter_AddRefs(mSourceDocument));
  mSourceNode = aDOMNode;
  mEndDragPoint = nsIntPoint(0, 0);

  
  
  
  
  
  nsCOMPtr<nsIContent> contentNode = do_QueryInterface(aDOMNode);
  if (contentNode) {
    nsIDocument* doc = contentNode->GetCurrentDoc();
    if (doc) {
      nsIPresShell* presShell = doc->GetPrimaryShell();
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
                                              nsIDOMDragEvent* aDragEvent,
                                              nsIDOMDataTransfer* aDataTransfer)
{
  NS_ENSURE_TRUE(aDragEvent, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aDataTransfer, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(mSuppressLevel == 0, NS_ERROR_FAILURE);

  mDataTransfer = aDataTransfer;
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
                                                  nsIDOMDragEvent* aDragEvent,
                                                  nsIDOMDataTransfer* aDataTransfer)
{
  NS_ENSURE_TRUE(aSelection, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aDragEvent, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(mSuppressLevel == 0, NS_ERROR_FAILURE);

  mDataTransfer = aDataTransfer;
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

  
  
  if (!mSuppressLevel && mDoingDrag) {
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
nsBaseDragService::EndDragSession(PRBool aDoneDrag)
{
  if (!mDoingDrag) {
    return NS_ERROR_FAILURE;
  }

  if (aDoneDrag && !mSuppressLevel)
    FireDragEventAtSource(NS_DRAGDROP_END);

  mDoingDrag = PR_FALSE;

  
  mSourceDocument = nsnull;
  mSourceNode = nsnull;
  mSelection = nsnull;
  mDataTransfer = nsnull;
  mHasImage = PR_FALSE;
  mUserCancelled = PR_FALSE;
  mImage = nsnull;
  mImageX = 0;
  mImageY = 0;
  mScreenX = -1;
  mScreenY = -1;

  return NS_OK;
}

NS_IMETHODIMP
nsBaseDragService::FireDragEventAtSource(PRUint32 aMsg)
{
  if (mSourceNode && !mSuppressLevel) {
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(mSourceDocument);
    if (doc) {
      nsCOMPtr<nsIPresShell> presShell = doc->GetPrimaryShell();
      if (presShell) {
        nsEventStatus status = nsEventStatus_eIgnore;
        nsDragEvent event(PR_TRUE, aMsg, nsnull);
        if (aMsg == NS_DRAGDROP_END) {
          event.refPoint.x = mEndDragPoint.x;
          event.refPoint.y = mEndDragPoint.y;
          event.userCancelled = mUserCancelled;
        }

        nsCOMPtr<nsIContent> content = do_QueryInterface(mSourceNode);
        return presShell->HandleDOMEventWithTarget(content, &event, &status);
      }
    }
  }

  return NS_OK;
}

static nsIPresShell*
GetPresShellForContent(nsIDOMNode* aDOMNode)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(aDOMNode);
  nsCOMPtr<nsIDocument> document = content->GetCurrentDoc();
  if (document) {
    document->FlushPendingNotifications(Flush_Display);

    return document->GetPrimaryShell();
  }

  return nsnull;
}

nsresult
nsBaseDragService::DrawDrag(nsIDOMNode* aDOMNode,
                            nsIScriptableRegion* aRegion,
                            PRInt32 aScreenX, PRInt32 aScreenY,
                            nsIntRect* aScreenDragRect,
                            gfxASurface** aSurface,
                            nsPresContext** aPresContext)
{
  *aSurface = nsnull;
  *aPresContext = nsnull;

  
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

  *aPresContext = presShell->GetPresContext();

  
  PRBool enableDragImages = PR_TRUE;
  nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (prefs)
    prefs->GetBoolPref(DRAGIMAGES_PREF, &enableDragImages);

  
  if (!enableDragImages || !mHasImage) {
    
    
    if (aRegion) {
      
      nsIFrame* rootFrame = presShell->GetRootFrame();
      if (rootFrame && *aPresContext) {
        nsIntRect dragRect;
        aRegion->GetBoundingBox(&dragRect.x, &dragRect.y, &dragRect.width, &dragRect.height);
        dragRect = dragRect.ToAppUnits(nsPresContext::AppUnitsPerCSSPixel()).
                            ToOutsidePixels((*aPresContext)->AppUnitsPerDevPixel());

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
    nsIntPoint pnt(aScreenDragRect->x, aScreenDragRect->y);
    nsRefPtr<gfxASurface> surface = presShell->RenderSelection(mSelection, pnt, aScreenDragRect);
    *aSurface = surface;
    NS_IF_ADDREF(*aSurface);
    return NS_OK;
  }

  
  
  
  if (mImage) {
    nsCOMPtr<nsICanvasElement> canvas = do_QueryInterface(dragNode);
    if (canvas) {
      return DrawDragForImage(*aPresContext, nsnull, canvas, aScreenX,
                              aScreenY, aScreenDragRect, aSurface);
    }

    nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(dragNode);
    
    if (imageLoader) {
      return DrawDragForImage(*aPresContext, imageLoader, nsnull, aScreenX,
                              aScreenY, aScreenDragRect, aSurface);
    }
  }

  
  nsCOMPtr<nsIRegion> clipRegion;
  if (aRegion)
    aRegion->GetRegion(getter_AddRefs(clipRegion));

  nsIntPoint pnt(aScreenDragRect->x, aScreenDragRect->y);
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
                                    nsICanvasElement* aCanvas,
                                    PRInt32 aScreenX, PRInt32 aScreenY,
                                    nsIntRect* aScreenDragRect,
                                    gfxASurface** aSurface)
{
  nsCOMPtr<imgIContainer> imgContainer;
  if (aImageLoader) {
    nsCOMPtr<imgIRequest> imgRequest;
    nsresult rv = aImageLoader->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                                          getter_AddRefs(imgRequest));
    NS_ENSURE_SUCCESS(rv, rv);
    if (!imgRequest)
      return NS_ERROR_NOT_AVAILABLE;

    rv = imgRequest->GetImage(getter_AddRefs(imgContainer));
    NS_ENSURE_SUCCESS(rv, rv);
    if (!imgContainer)
      return NS_ERROR_NOT_AVAILABLE;

    
    imgContainer->GetWidth(&aScreenDragRect->width);
    imgContainer->GetHeight(&aScreenDragRect->height);
  }
  else {
    NS_ASSERTION(aCanvas, "both image and canvas are null");
    PRUint32 width, height;
    aCanvas->GetSize(&width, &height);
    aScreenDragRect->width = width;
    aScreenDragRect->height = height;
  }

  nsIntSize srcSize = aScreenDragRect->Size();
  nsIntSize destSize = srcSize;

  if (destSize.width == 0 || destSize.height == 0)
    return NS_ERROR_FAILURE;

  
  
  nsIDeviceContext* deviceContext = aPresContext->DeviceContext();
  nsRect maxSize;
  deviceContext->GetClientRect(maxSize);
  nscoord maxWidth = aPresContext->AppUnitsToDevPixels(maxSize.width >> 1);
  nscoord maxHeight = aPresContext->AppUnitsToDevPixels(maxSize.height >> 1);
  if (destSize.width > maxWidth || destSize.height > maxHeight) {
    float scale = 1.0;
    if (destSize.width > maxWidth)
      scale = PR_MIN(scale, float(maxWidth) / destSize.width);
    if (destSize.height > maxHeight)
      scale = PR_MIN(scale, float(maxHeight) / destSize.height);

    destSize.width = NSToIntFloor(float(destSize.width) * scale);
    destSize.height = NSToIntFloor(float(destSize.height) * scale);

    aScreenDragRect->x = NSToIntFloor(aScreenX - float(mImageX) * scale);
    aScreenDragRect->y = NSToIntFloor(aScreenY - float(mImageY) * scale);
    aScreenDragRect->width = destSize.width;
    aScreenDragRect->height = destSize.height;
  }

  nsRefPtr<gfxASurface> surface =
    gfxPlatform::GetPlatform()->CreateOffscreenSurface(gfxIntSize(destSize.width, destSize.height),
                                                       gfxASurface::ImageFormatARGB32);
  if (!surface)
    return NS_ERROR_FAILURE;

  nsRefPtr<gfxContext> ctx = new gfxContext(surface);
  if (!ctx)
    return NS_ERROR_FAILURE;

  *aSurface = surface;
  NS_ADDREF(*aSurface);

  if (aImageLoader) {
    gfxRect outRect(0, 0, destSize.width, destSize.height);
    gfxMatrix scale =
      gfxMatrix().Scale(srcSize.width/outRect.Width(), srcSize.height/outRect.Height());
    nsIntRect imgSize(0, 0, srcSize.width, srcSize.height);
    imgContainer->Draw(ctx, gfxPattern::FILTER_GOOD, scale, outRect, imgSize);
    return NS_OK;
  } else {
    return aCanvas->RenderContexts(ctx, gfxPattern::FILTER_GOOD);
  }
}

void
nsBaseDragService::ConvertToUnscaledDevPixels(nsPresContext* aPresContext,
                                              PRInt32* aScreenX, PRInt32* aScreenY)
{
  PRInt32 adj = aPresContext->DeviceContext()->UnscaledAppUnitsPerDevPixel();
  *aScreenX = nsPresContext::CSSPixelsToAppUnits(*aScreenX) / adj;
  *aScreenY = nsPresContext::CSSPixelsToAppUnits(*aScreenY) / adj;
}

NS_IMETHODIMP
nsBaseDragService::Suppress()
{
  EndDragSession(PR_FALSE);
  ++mSuppressLevel;
  return NS_OK;
}

NS_IMETHODIMP
nsBaseDragService::Unsuppress()
{
  --mSuppressLevel;
  return NS_OK;
}

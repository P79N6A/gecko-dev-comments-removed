










































#include "nsMenuPopupFrame.h"
#include "nsGkAtoms.h"
#include "nsIContent.h"
#include "prtypes.h"
#include "nsIAtom.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsCSSRendering.h"
#include "nsINameSpaceManager.h"
#include "nsIViewManager.h"
#include "nsWidgetsCID.h"
#include "nsMenuFrame.h"
#include "nsIPopupSetFrame.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMScreen.h"
#include "nsIPresShell.h"
#include "nsFrameManager.h"
#include "nsIDocument.h"
#include "nsIDeviceContext.h"
#include "nsRect.h"
#include "nsIDOMXULDocument.h"
#include "nsILookAndFeel.h"
#include "nsIComponentManager.h"
#include "nsBoxLayoutState.h"
#include "nsIScrollableView.h"
#include "nsIScrollableFrame.h"
#include "nsGUIEvent.h"
#include "nsIRootBox.h"
#include "nsIDocShellTreeItem.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsCSSFrameConstructor.h"
#include "nsIBoxLayout.h"
#include "nsIPopupBoxObject.h"
#include "nsIReflowCallback.h"
#ifdef XP_WIN
#include "nsISound.h"
#endif

const PRInt32 kMaxZ = 0x7fffffff; 


static nsIPopupSetFrame*
GetPopupSetFrame(nsPresContext* aPresContext)
{
  nsIRootBox* rootBox = nsIRootBox::GetRootBox(aPresContext->PresShell());
  if (!rootBox)
    return nsnull;

  nsIFrame* popupSetFrame = rootBox->GetPopupSetFrame();
  if (!popupSetFrame)
    return nsnull;

  nsIPopupSetFrame* popupSet = nsnull;
  CallQueryInterface(popupSetFrame, &popupSet);
  return popupSet;
}






nsIFrame*
NS_NewMenuPopupFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMenuPopupFrame (aPresShell, aContext);
}

NS_IMETHODIMP_(nsrefcnt) 
nsMenuPopupFrame::AddRef(void)
{
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt) 
nsMenuPopupFrame::Release(void)
{
    return NS_OK;
}





NS_INTERFACE_MAP_BEGIN(nsMenuPopupFrame)
  NS_INTERFACE_MAP_ENTRY(nsIMenuParent)
NS_INTERFACE_MAP_END_INHERITING(nsBoxFrame)





nsMenuPopupFrame::nsMenuPopupFrame(nsIPresShell* aShell, nsStyleContext* aContext)
  :nsBoxFrame(aShell, aContext),
  mCurrentMenu(nsnull),
  mTimerMenu(nsnull),
  mCloseTimer(nsnull),
  mMenuCanOverlapOSBar(PR_FALSE),
  mShouldAutoPosition(PR_TRUE),
  mShouldRollup(PR_TRUE),
  mConsumeRollupEvent(nsIPopupBoxObject::ROLLUP_DEFAULT),
  mInContentShell(PR_TRUE)
{
  SetIsContextMenu(PR_FALSE);   
} 


NS_IMETHODIMP
nsMenuPopupFrame::Init(nsIContent*      aContent,
                       nsIFrame*        aParent,
                       nsIFrame*        aPrevInFlow)
{
  nsresult rv = nsBoxFrame::Init(aContent, aParent, aPrevInFlow);

  
  mTimerMediator = new nsMenuPopupTimerMediator(this);
  if (NS_UNLIKELY(!mTimerMediator))
    return NS_ERROR_OUT_OF_MEMORY;

  nsPresContext* presContext = PresContext();

  
  
  PRBool tempBool;
  presContext->LookAndFeel()->
    GetMetric(nsILookAndFeel::eMetric_MenusCanOverlapOSBar, tempBool);
  mMenuCanOverlapOSBar = tempBool;

  CreateViewForFrame(presContext, this, GetStyleContext(), PR_TRUE);

  
  
  
  nsIView* ourView = GetView();
  nsIViewManager* viewManager = ourView->GetViewManager();

  
  viewManager->RemoveChild(ourView);

  
  nsIView* rootView;
  viewManager->GetRootView(rootView);
  viewManager->SetViewZIndex(ourView, PR_FALSE, kMaxZ);
  viewManager->InsertChild(rootView, ourView, nsnull, PR_TRUE);

  
  
  
  viewManager->SetViewFloating(ourView, PR_TRUE);

  nsCOMPtr<nsISupports> cont = PresContext()->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(cont);
  PRInt32 type = -1;
  if (dsti && NS_SUCCEEDED(dsti->GetItemType(&type)) &&
      type == nsIDocShellTreeItem::typeChrome)
    mInContentShell = PR_FALSE;

  
  viewManager->SetViewVisibility(ourView, nsViewVisibility_kHide);
  if (!ourView->HasWidget()) {
    CreateWidgetForView(ourView);
  }

  MoveToAttributePosition();

  return rv;
}

nsresult
nsMenuPopupFrame::CreateWidgetForView(nsIView* aView)
{
  
  nsWidgetInitData widgetData;
  widgetData.mWindowType = eWindowType_popup;
  widgetData.mBorderStyle = eBorderStyle_default;
  widgetData.clipSiblings = PR_TRUE;

  PRBool isCanvas;
  const nsStyleBackground* bg;
  PRBool hasBG =
    nsCSSRendering::FindBackground(PresContext(), this, &bg, &isCanvas);
  PRBool viewHasTransparentContent = hasBG &&
    (bg->mBackgroundFlags & NS_STYLE_BG_COLOR_TRANSPARENT) &&
    !GetStyleDisplay()->mAppearance && !mInContentShell;

  nsIContent* parentContent = GetContent()->GetParent();
  nsIAtom *tag = nsnull;
  if (parentContent)
    tag = parentContent->Tag();
  widgetData.mDropShadow = !(viewHasTransparentContent || tag == nsGkAtoms::menulist);
  
#if defined(XP_MACOSX) || defined(XP_BEOS)
  static NS_DEFINE_IID(kCPopupCID,  NS_POPUP_CID);
  aView->CreateWidget(kCPopupCID, &widgetData, nsnull, PR_TRUE, PR_TRUE, 
                      eContentTypeUI);
#else
  static NS_DEFINE_IID(kCChildCID,  NS_CHILD_CID);
  aView->CreateWidget(kCChildCID, &widgetData, nsnull, PR_TRUE, PR_TRUE);
#endif
  aView->GetWidget()->SetWindowTranslucency(viewHasTransparentContent);
  return NS_OK;
}

void
nsMenuPopupFrame::InvalidateInternal(const nsRect& aDamageRect,
                                     nscoord aX, nscoord aY, nsIFrame* aForChild,
                                     PRBool aImmediate)
{
  InvalidateRoot(aDamageRect, aX, aY, aImmediate);
}

void
nsMenuPopupFrame::GetLayoutFlags(PRUint32& aFlags)
{
  aFlags = NS_FRAME_NO_SIZE_VIEW | NS_FRAME_NO_MOVE_VIEW | NS_FRAME_NO_VISIBILITY;
}





void
nsMenuPopupFrame::GetViewOffset(nsIView* aView, nsPoint& aPoint)
{
  
  
  
  
  
  
  
  
  
  nsIView* rootView;
  aView->GetViewManager()->GetRootView(rootView);
  aPoint = aView->GetOffsetTo(rootView);
}








void
nsMenuPopupFrame::GetRootViewForPopup(nsIFrame* aStartFrame,
                                      PRBool    aStopAtViewManagerRoot,
                                      nsIView** aResult)
{
  *aResult = nsnull;

  nsIView* view = aStartFrame->GetClosestView();
  NS_ASSERTION(view, "frame must have a closest view!");
  if (view) {
    nsIView* rootView = nsnull;
    if (aStopAtViewManagerRoot) {
      view->GetViewManager()->GetRootView(rootView);
    }
    
    while (view) {
      
      
      
      nsIWidget* widget = view->GetWidget();
      if (widget) {
        nsWindowType wtype;
        widget->GetWindowType(wtype);
        if (wtype == eWindowType_popup) {
          *aResult = view;
          return;
        }
      }

      if (aStopAtViewManagerRoot && view == rootView) {
        *aResult = view;
        return;
      }

      nsIView* temp = view->GetParent();
      if (!temp) {
        
        
        *aResult = view;
      }
      view = temp;
    }
  }
}











void
nsMenuPopupFrame::AdjustClientXYForNestedDocuments ( nsIDOMXULDocument* inPopupDoc, nsIPresShell* inPopupShell, 
                                                         PRInt32 inClientX, PRInt32 inClientY, 
                                                         PRInt32* outAdjX, PRInt32* outAdjY )
{
  if ( !inPopupDoc || !outAdjX || !outAdjY )
    return;

  
  nsIWidget* popupDocumentWidget = nsnull;
  nsIViewManager* viewManager = inPopupShell->GetViewManager();
  if ( viewManager ) {  
    nsIView* rootView;
    viewManager->GetRootView(rootView);
    if ( rootView )
      popupDocumentWidget = rootView->GetNearestWidget(nsnull);
  }
  NS_ASSERTION(popupDocumentWidget, "ACK, BAD WIDGET");
  
  
  
  
  

  nsCOMPtr<nsIDOMNode> targetNode;
  if (mContent->Tag() == nsGkAtoms::tooltip)
    inPopupDoc->TrustedGetTooltipNode(getter_AddRefs(targetNode));
  else
    inPopupDoc->TrustedGetPopupNode(getter_AddRefs(targetNode));

  
  nsCOMPtr<nsIContent> targetAsContent ( do_QueryInterface(targetNode) );
  nsIWidget* targetDocumentWidget = nsnull;
  if ( targetAsContent ) {
    nsCOMPtr<nsIDocument> targetDocument = targetAsContent->GetDocument();
    if (targetDocument) {
      nsIPresShell *shell = targetDocument->GetPrimaryShell();
      if ( shell ) {
        
        
        nsIFrame* targetFrame = shell->GetPrimaryFrameFor(targetAsContent);
        nsIView* parentView = nsnull;
        if (targetFrame) {
          GetRootViewForPopup(targetFrame, PR_TRUE, &parentView);
          if (parentView) {
            targetDocumentWidget = parentView->GetNearestWidget(nsnull);
          }
        }
        if (!targetDocumentWidget) {
          
          
          nsIViewManager* viewManagerTarget = shell->GetViewManager();
          if ( viewManagerTarget ) {
            nsIView* rootViewTarget;
            viewManagerTarget->GetRootView(rootViewTarget);
            if ( rootViewTarget ) {
              targetDocumentWidget = rootViewTarget->GetNearestWidget(nsnull);
            }
          }
        }
      }
    }
  }
  

  
  
  nsRect popupDocTopLeft;
  if ( popupDocumentWidget ) {
    nsRect topLeftClient ( 0, 0, 10, 10 );
    popupDocumentWidget->WidgetToScreen ( topLeftClient, popupDocTopLeft );
  }
  nsRect targetDocTopLeft;
  if ( targetDocumentWidget ) {
    nsRect topLeftClient ( 0, 0, 10, 10 );
    targetDocumentWidget->WidgetToScreen ( topLeftClient, targetDocTopLeft );
  }
  nsPoint pixelOffset ( targetDocTopLeft.x - popupDocTopLeft.x, targetDocTopLeft.y - popupDocTopLeft.y );

  nsPresContext* context = PresContext();
  *outAdjX = nsPresContext::CSSPixelsToAppUnits(inClientX) +
             context->DevPixelsToAppUnits(pixelOffset.x);
  *outAdjY = nsPresContext::CSSPixelsToAppUnits(inClientY) +
             context->DevPixelsToAppUnits(pixelOffset.y);
  
} 










void
nsMenuPopupFrame::AdjustPositionForAnchorAlign ( PRInt32* ioXPos, PRInt32* ioYPos, const nsRect & inParentRect,
                                                    const nsString& aPopupAnchor, const nsString& aPopupAlign,
                                                    PRBool* outFlushWithTopBottom )
{
  nsAutoString popupAnchor(aPopupAnchor);
  nsAutoString popupAlign(aPopupAlign);

  if (GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
    if (popupAnchor.EqualsLiteral("topright"))
      popupAnchor.AssignLiteral("topleft");
    else if (popupAnchor.EqualsLiteral("topleft"))
      popupAnchor.AssignLiteral("topright");
    else if (popupAnchor.EqualsLiteral("bottomleft"))
      popupAnchor.AssignLiteral("bottomright");
    else if (popupAnchor.EqualsLiteral("bottomright"))
      popupAnchor.AssignLiteral("bottomleft");

    if (popupAlign.EqualsLiteral("topright"))
      popupAlign.AssignLiteral("topleft");
    else if (popupAlign.EqualsLiteral("topleft"))
      popupAlign.AssignLiteral("topright");
    else if (popupAlign.EqualsLiteral("bottomleft"))
      popupAlign.AssignLiteral("bottomright");
    else if (popupAnchor.EqualsLiteral("bottomright"))
      popupAlign.AssignLiteral("bottomleft");
  }

  
  nsMargin margin;
  GetStyleMargin()->GetMargin(margin);
  if (popupAlign.EqualsLiteral("topleft")) {
    *ioXPos += margin.left;
    *ioYPos += margin.top;
  } else if (popupAlign.EqualsLiteral("topright")) {
    *ioXPos += margin.right;
    *ioYPos += margin.top;
  } else if (popupAlign.EqualsLiteral("bottomleft")) {
    *ioXPos += margin.left;
    *ioYPos += margin.bottom;
  } else if (popupAlign.EqualsLiteral("bottomright")) {
    *ioXPos += margin.right;
    *ioYPos += margin.bottom;
  }
  
  if (popupAnchor.EqualsLiteral("topright") && popupAlign.EqualsLiteral("topleft")) {
    *ioXPos += inParentRect.width;
  }
  else if (popupAnchor.EqualsLiteral("topleft") && popupAlign.EqualsLiteral("topleft")) {
    *outFlushWithTopBottom = PR_TRUE;
  }
  else if (popupAnchor.EqualsLiteral("topright") && popupAlign.EqualsLiteral("bottomright")) {
    *ioXPos -= (mRect.width - inParentRect.width);
    *ioYPos -= mRect.height;
    *outFlushWithTopBottom = PR_TRUE;
  }
  else if (popupAnchor.EqualsLiteral("bottomright") && popupAlign.EqualsLiteral("bottomleft")) {
    *ioXPos += inParentRect.width;
    *ioYPos -= (mRect.height - inParentRect.height);
  }
  else if (popupAnchor.EqualsLiteral("bottomright") && popupAlign.EqualsLiteral("topright")) {
    *ioXPos -= (mRect.width - inParentRect.width);
    *ioYPos += inParentRect.height;
    *outFlushWithTopBottom = PR_TRUE;
  }
  else if (popupAnchor.EqualsLiteral("topleft") && popupAlign.EqualsLiteral("topright")) {
    *ioXPos -= mRect.width;
  }
  else if (popupAnchor.EqualsLiteral("topleft") && popupAlign.EqualsLiteral("bottomleft")) {
    *ioYPos -= mRect.height;
    *outFlushWithTopBottom = PR_TRUE;
  }
  else if (popupAnchor.EqualsLiteral("bottomleft") && popupAlign.EqualsLiteral("bottomright")) {
    *ioXPos -= mRect.width;
    *ioYPos -= (mRect.height - inParentRect.height);
  }
  else if (popupAnchor.EqualsLiteral("bottomleft") && popupAlign.EqualsLiteral("topleft")) {
    *ioYPos += inParentRect.height;
    *outFlushWithTopBottom = PR_TRUE;
  }
  else
    NS_WARNING ( "Hmmm, looks like you've hit a anchor/align case we weren't setup for." );

} 









PRBool
nsMenuPopupFrame::IsMoreRoomOnOtherSideOfParent ( PRBool inFlushAboveBelow, PRInt32 inScreenViewLocX, PRInt32 inScreenViewLocY,
                                                     const nsRect & inScreenParentFrameRect, PRInt32 inScreenTopTwips, PRInt32 inScreenLeftTwips,
                                                     PRInt32 inScreenBottomTwips, PRInt32 inScreenRightTwips )
{
  PRBool switchSides = PR_FALSE;
  if ( inFlushAboveBelow ) {
    PRInt32 availAbove = inScreenParentFrameRect.y - inScreenTopTwips;
    PRInt32 availBelow = inScreenBottomTwips - (inScreenParentFrameRect.y + inScreenParentFrameRect.height) ;
    if ( inScreenViewLocY > inScreenParentFrameRect.y )       
      switchSides = availAbove > availBelow;
    else
      switchSides = availBelow > availAbove;
  }
  else {
    PRInt32 availLeft = inScreenParentFrameRect.x - inScreenLeftTwips;
    PRInt32 availRight = inScreenRightTwips - (inScreenParentFrameRect.x + inScreenParentFrameRect.width) ;
    if ( inScreenViewLocX > inScreenParentFrameRect.x )       
      switchSides = availLeft > availRight;
    else
      switchSides = availRight > availLeft;           
  }

  return switchSides;
  
} 












void
nsMenuPopupFrame::MovePopupToOtherSideOfParent ( PRBool inFlushAboveBelow, PRInt32* ioXPos, PRInt32* ioYPos, 
                                                 PRInt32* ioScreenViewLocX, PRInt32* ioScreenViewLocY,
                                                 const nsRect & inScreenParentFrameRect, PRInt32 inScreenTopTwips, PRInt32 inScreenLeftTwips,
                                                 PRInt32 inScreenBottomTwips, PRInt32 inScreenRightTwips )
{
  if ( inFlushAboveBelow ) {
    if ( *ioScreenViewLocY > inScreenParentFrameRect.y ) {     
      
      PRInt32 shiftDistY = inScreenParentFrameRect.height + mRect.height;
      *ioYPos -= shiftDistY;
      *ioScreenViewLocY -= shiftDistY;
      
      if ( *ioScreenViewLocY < inScreenTopTwips ) {
        PRInt32 trimY = inScreenTopTwips - *ioScreenViewLocY;
        *ioYPos += trimY;
        *ioScreenViewLocY += trimY;
        mRect.height -= trimY;
      }
    }
    else {                                               
      
      PRInt32 shiftDistY = inScreenParentFrameRect.height + mRect.height;
      *ioYPos += shiftDistY;
      *ioScreenViewLocY += shiftDistY;
    }
  }
  else {
    if ( *ioScreenViewLocX > inScreenParentFrameRect.x ) {     
      
      PRInt32 shiftDistX = inScreenParentFrameRect.width + mRect.width;
      *ioXPos -= shiftDistX;
      *ioScreenViewLocX -= shiftDistX;
      
      if ( *ioScreenViewLocX < inScreenLeftTwips ) {
        PRInt32 trimX = inScreenLeftTwips - *ioScreenViewLocX;
        *ioXPos += trimX;
        *ioScreenViewLocX += trimX;
        mRect.width -= trimX;
      }
    }
    else {                                               
      
      PRInt32 shiftDistX = inScreenParentFrameRect.width + mRect.width;
      *ioXPos += shiftDistX;
      *ioScreenViewLocX += shiftDistX;
    }               
  }

} 

class nsASyncMenuActivation : public nsIReflowCallback
{
public:
  nsASyncMenuActivation(nsIContent* aContent)
    : mContent(aContent)
  {
  }

  virtual PRBool ReflowFinished() {
    PRBool shouldFlush = PR_FALSE;
    if (mContent &&
        !mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::menuactive,
                               nsGkAtoms::_true, eCaseMatters) &&
        mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::menutobedisplayed,
                              nsGkAtoms::_true, eCaseMatters)) {
      mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::menuactive,
                        NS_LITERAL_STRING("true"), PR_TRUE);
      shouldFlush = PR_TRUE;
    }

    delete this;
    return shouldFlush;
  }

  nsCOMPtr<nsIContent> mContent;
};

nsresult 
nsMenuPopupFrame::SyncViewWithFrame(nsPresContext* aPresContext,
                                    const nsString& aPopupAnchor,
                                    const nsString& aPopupAlign,
                                    nsIFrame* aFrame, 
                                    PRInt32 aXPos, PRInt32 aYPos)
{
  NS_ENSURE_ARG(aPresContext);
  NS_ENSURE_ARG(aFrame);

  if (!mShouldAutoPosition && !mInContentShell) 
    return NS_OK;

  
  
  
  
  nsIView* containingView = nsnull;
  nsPoint offset;
  nsMargin margin;
  containingView = aFrame->GetClosestView(&offset);
  if (!containingView)
    return NS_OK;

  
  
  
  nsIView* view = GetView();

  
  
  
  nsPoint parentPos;
  GetViewOffset(containingView, parentPos);

  
  
  nsRect parentRect = aFrame->GetRect();

  
  nsIPresShell *presShell = aPresContext->PresShell();
  nsIDocument *document = presShell->GetDocument();

  PRBool sizedToPopup = (mContent->Tag() != nsGkAtoms::tooltip) &&
    (nsMenuFrame::IsSizedToPopup(aFrame->GetContent(), PR_FALSE));

  
  
  if (sizedToPopup) {
    mRect.width = parentRect.width;
  }

  
  
  PRInt32 xpos = 0, ypos = 0;

  
  
  
  
  
  PRBool anchoredToParent = PR_FALSE;
  PRBool readjustAboveBelow = PR_FALSE;

  if ( aXPos != -1 || aYPos != -1 ) {
  
    
    
    
    nsCOMPtr<nsIDOMXULDocument> xulDoc ( do_QueryInterface(document) );
    AdjustClientXYForNestedDocuments ( xulDoc, presShell, aXPos, aYPos, &xpos, &ypos );

    
    GetStyleMargin()->GetMargin(margin);

    xpos += margin.left;
    ypos += margin.top;
  } 
  else {
    anchoredToParent = PR_TRUE;

    xpos = parentPos.x + offset.x;
    ypos = parentPos.y + offset.y;
    
    
    
    AdjustPositionForAnchorAlign ( &xpos, &ypos, parentRect, aPopupAnchor, aPopupAlign, &readjustAboveBelow );    
  }
  
  
  
  
  nsPIDOMWindow *window = document->GetWindow();
  if (!window)
    return NS_OK;

  nsIDeviceContext* devContext = PresContext()->DeviceContext();
  nsRect rect;
  if ( mMenuCanOverlapOSBar ) {
    devContext->GetRect(rect);
  }
  else {
    devContext->GetClientRect(rect);
  }

  
  rect.width  -= nsPresContext::CSSPixelsToAppUnits(3);
  rect.height -= nsPresContext::CSSPixelsToAppUnits(3);

  
  if (mInContentShell) {
    nsRect rootScreenRect = presShell->GetRootFrame()->GetScreenRect();
    rootScreenRect.ScaleRoundIn(aPresContext->AppUnitsPerDevPixel());
    rect.IntersectRect(rect, rootScreenRect);
  }

  PRInt32 screenLeftTwips   = rect.x;
  PRInt32 screenTopTwips    = rect.y;
  PRInt32 screenWidthTwips  = rect.width;
  PRInt32 screenHeightTwips = rect.height;
  PRInt32 screenRightTwips  = rect.XMost();
  PRInt32 screenBottomTwips = rect.YMost();
  
  
  
  
  
  
  
  

  
  
  
  
  
  nsIView* parentView = nsnull;
  GetRootViewForPopup(aFrame, PR_FALSE, &parentView);
  if (!parentView)
    return NS_OK;

  
  

  nsPoint parentViewWidgetOffset;
  nsIWidget* parentViewWidget = containingView->GetNearestWidget(&parentViewWidgetOffset);
  nsRect localParentWidgetRect(0,0,0,0), screenParentWidgetRect;
  parentViewWidget->WidgetToScreen ( localParentWidgetRect, screenParentWidgetRect );
  PRInt32 screenViewLocX = aPresContext->DevPixelsToAppUnits(screenParentWidgetRect.x) +
    (xpos - parentPos.x) + parentViewWidgetOffset.x;
  PRInt32 screenViewLocY = aPresContext->DevPixelsToAppUnits(screenParentWidgetRect.y) +
    (ypos - parentPos.y) + parentViewWidgetOffset.y;

  if ( anchoredToParent ) {
    
    
    
    
    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    



    
    
    nsRect screenParentFrameRect (aPresContext->AppUnitsToDevPixels(offset.x), aPresContext->AppUnitsToDevPixels(offset.y),
                                    parentRect.width, parentRect.height );
    parentViewWidget->WidgetToScreen ( screenParentFrameRect, screenParentFrameRect );
    screenParentFrameRect.x = aPresContext->DevPixelsToAppUnits(screenParentFrameRect.x);
    screenParentFrameRect.y = aPresContext->DevPixelsToAppUnits(screenParentFrameRect.y);

    
    if (screenViewLocY < screenTopTwips) {
      PRInt32 moveDist = screenTopTwips - screenViewLocY;
      screenViewLocY = screenTopTwips;
      ypos += moveDist;
    }
    
    
    if ( (screenViewLocX + mRect.width) > screenRightTwips ||
           screenViewLocX < screenLeftTwips ||
          (screenViewLocY + mRect.height) > screenBottomTwips ) {
      
      
      
      PRBool switchSides = IsMoreRoomOnOtherSideOfParent ( readjustAboveBelow, screenViewLocX, screenViewLocY,
                                                            screenParentFrameRect, screenTopTwips, screenLeftTwips,
                                                            screenBottomTwips, screenRightTwips );
      
      
      
      if ( switchSides )
        MovePopupToOtherSideOfParent ( readjustAboveBelow, &xpos, &ypos, &screenViewLocX, &screenViewLocY, 
                                        screenParentFrameRect, screenTopTwips, screenLeftTwips,
                                        screenBottomTwips, screenRightTwips );
                                        
      
      
      if ( readjustAboveBelow ) {
        
        if ( (screenViewLocX + mRect.width) > screenRightTwips ) {
          PRInt32 moveDistX = (screenViewLocX + mRect.width) - screenRightTwips;
          if ( screenViewLocX - moveDistX < screenLeftTwips )
            moveDistX = screenViewLocX - screenLeftTwips;          
          screenViewLocX -= moveDistX;
          xpos -= moveDistX;
        } else if (screenViewLocX < screenLeftTwips) {
          
          PRInt32 moveDistX = screenLeftTwips - screenViewLocX;
          if ( (screenViewLocX + mRect.width + moveDistX) > screenRightTwips )
            moveDistX = screenRightTwips - screenViewLocX - mRect.width;
          screenViewLocX += moveDistX;
          xpos += moveDistX;
        }
      }
      else {
        
        















        if ( (screenViewLocY + mRect.height) > screenBottomTwips ) {
          
          PRInt32 moveDistY = (screenViewLocY + mRect.height) - screenHeightTwips;
          if ( screenViewLocY - moveDistY < screenTopTwips )
            moveDistY = screenViewLocY - screenTopTwips;          
          screenViewLocY -= moveDistY;
          ypos -= moveDistY; 
        } 
      }
      
      
      
      
      
      

      PRInt32 xSpillage = (screenViewLocX + mRect.width) - screenRightTwips;
      if ( xSpillage > 0 )
        mRect.width -= xSpillage;
      PRInt32 ySpillage = (screenViewLocY + mRect.height) - screenBottomTwips;
      if ( ySpillage > 0 )
        mRect.height -= ySpillage;

      
      if(mRect.width > screenWidthTwips) 
          mRect.width = screenWidthTwips;    
      if(mRect.height > screenHeightTwips)
          mRect.height = screenHeightTwips;   

    } 
  } 
  else {
  
    
    
    
    

    
    
    

    
    
    
    
    
    

    
    if(mRect.width > screenWidthTwips) 
        mRect.width = screenWidthTwips;    
    if(mRect.height > screenHeightTwips)
        mRect.height = screenHeightTwips;   

    
    
    if ( screenViewLocX < screenLeftTwips ) {
      PRInt32 moveDistX = screenLeftTwips - screenViewLocX;
      xpos += moveDistX;
      screenViewLocX += moveDistX;
    }
    if ( (screenViewLocX + mRect.width) > screenRightTwips )
      xpos -= (screenViewLocX + mRect.width) - screenRightTwips;

    
    
    if ( screenViewLocY < screenTopTwips ) {
      PRInt32 moveDistY = screenTopTwips - screenViewLocY;
      ypos += moveDistY;
      screenViewLocY += moveDistY;
    }

    
    
    
    if ( (screenViewLocY + mRect.height) > screenBottomTwips ) {
      
      
      
      if (screenBottomTwips - screenViewLocY >
          screenViewLocY - screenTopTwips) {
        
        
        
        mRect.height = screenBottomTwips - screenViewLocY;
      } else {
        
        
        if (mRect.height > screenViewLocY - screenTopTwips) {
          
          mRect.height = screenViewLocY - screenTopTwips;
        }
        ypos -= (mRect.height + margin.top + margin.bottom);
      }
    }
  }  

  aPresContext->GetViewManager()->MoveViewTo(view, xpos, ypos); 

  
  nsPoint frameOrigin = GetPosition();
  nsPoint offsetToView;
  GetOriginToViewOffset(offsetToView, nsnull);
  frameOrigin -= offsetToView;
  nsBoxFrame::SetPosition(frameOrigin);

  if (sizedToPopup) {
      nsBoxLayoutState state(PresContext());
      SetBounds(state, nsRect(mRect.x, mRect.y, parentRect.width, mRect.height));
  }
    
  if (!mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::menuactive,
                             nsGkAtoms::_true, eCaseMatters) &&
      mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::menutobedisplayed,
                            nsGkAtoms::_true, eCaseMatters)) {
    nsIReflowCallback* cb = new nsASyncMenuActivation(mContent);
    NS_ENSURE_TRUE(cb, NS_ERROR_OUT_OF_MEMORY);
    PresContext()->PresShell()->PostReflowCallback(cb);
  }

  return NS_OK;
}

static void GetInsertionPoint(nsIPresShell* aShell, nsIFrame* aFrame, nsIFrame* aChild,
                              nsIFrame** aResult)
{
  nsIContent* child = nsnull;
  if (aChild)
    child = aChild->GetContent();
  aShell->FrameConstructor()->GetInsertionPoint(aFrame, child, aResult);
}

 nsIMenuFrame*
nsMenuPopupFrame::GetNextMenuItem(nsIMenuFrame* aStart)
{
  nsIFrame* immediateParent = nsnull;
  GetInsertionPoint(PresContext()->PresShell(), this, nsnull,
                    &immediateParent);
  if (!immediateParent)
    immediateParent = this;

  nsIFrame* currFrame = nsnull;
  nsIFrame* startFrame = nsnull;
  if (aStart) {
    aStart->QueryInterface(NS_GET_IID(nsIFrame), (void**)&currFrame); 
    if (currFrame) {
      startFrame = currFrame;
      currFrame = currFrame->GetNextSibling();
    }
  }
  else 
    currFrame = immediateParent->GetFirstChild(nsnull);
  
  while (currFrame) {
    
    if (IsValidItem(currFrame->GetContent())) {
      nsIMenuFrame *menuFrame;
      if (NS_FAILED(CallQueryInterface(currFrame, &menuFrame)))
        menuFrame = nsnull;
      return menuFrame;
    }
    currFrame = currFrame->GetNextSibling();
  }

  currFrame = immediateParent->GetFirstChild(nsnull);

  
  while (currFrame && currFrame != startFrame) {
    
    if (IsValidItem(currFrame->GetContent())) {
      nsIMenuFrame *menuFrame;
      if (NS_FAILED(CallQueryInterface(currFrame, &menuFrame)))
        menuFrame = nsnull;
      return menuFrame;
    }

    currFrame = currFrame->GetNextSibling();
  }

  
  return aStart;
}

 nsIMenuFrame*
nsMenuPopupFrame::GetPreviousMenuItem(nsIMenuFrame* aStart)
{
  nsIFrame* immediateParent = nsnull;
  GetInsertionPoint(PresContext()->PresShell(), this, nsnull,
                    &immediateParent);
  if (!immediateParent)
    immediateParent = this;

  nsFrameList frames(immediateParent->GetFirstChild(nsnull));
                              
  nsIFrame* currFrame = nsnull;
  nsIFrame* startFrame = nsnull;
  if (aStart) {
    aStart->QueryInterface(NS_GET_IID(nsIFrame), (void**)&currFrame);
    if (currFrame) {
      startFrame = currFrame;
      currFrame = frames.GetPrevSiblingFor(currFrame);
    }
  }
  else currFrame = frames.LastChild();

  while (currFrame) {
    
    if (IsValidItem(currFrame->GetContent())) {
      nsIMenuFrame *menuFrame;
      if (NS_FAILED(CallQueryInterface(currFrame, &menuFrame)))
        menuFrame = nsnull;
      return menuFrame;
    }
    currFrame = frames.GetPrevSiblingFor(currFrame);
  }

  currFrame = frames.LastChild();

  
  while (currFrame && currFrame != startFrame) {
    
    if (IsValidItem(currFrame->GetContent())) {
      nsIMenuFrame *menuFrame;
      if (NS_FAILED(CallQueryInterface(currFrame, &menuFrame)))
        menuFrame = nsnull;
      return menuFrame;
    }

    currFrame = frames.GetPrevSiblingFor(currFrame);
  }

  
  return aStart;
}

 nsIMenuFrame*
nsMenuPopupFrame::GetCurrentMenuItem()
{
  return mCurrentMenu;
}

NS_IMETHODIMP nsMenuPopupFrame::ConsumeOutsideClicks(PRBool& aConsumeOutsideClicks)
{
  
















  
  if (mConsumeRollupEvent != nsIPopupBoxObject::ROLLUP_DEFAULT) {
    aConsumeOutsideClicks = mConsumeRollupEvent == nsIPopupBoxObject::ROLLUP_CONSUME;
    return NS_OK;
  }

  aConsumeOutsideClicks = PR_TRUE;

  nsCOMPtr<nsIContent> parentContent = mContent->GetParent();

  if (parentContent) {
    nsIAtom *parentTag = parentContent->Tag();
    if (parentTag == nsGkAtoms::menulist)
      return NS_OK;  
    if (parentTag == nsGkAtoms::menu || parentTag == nsGkAtoms::popupset) {
#if defined(XP_WIN) || defined(XP_OS2)
      
      aConsumeOutsideClicks = PR_FALSE;
#endif
      return NS_OK;
    }
    if (parentTag == nsGkAtoms::textbox) {
      
      if (parentContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                                     nsGkAtoms::autocomplete, eCaseMatters))
        aConsumeOutsideClicks = PR_FALSE;
    }
  }

  return NS_OK;
}

static nsIScrollableView* GetScrollableViewForFrame(nsIFrame* aFrame)
{
  nsIScrollableFrame* sf;
  nsresult rv = CallQueryInterface(aFrame, &sf);
  if (NS_FAILED(rv))
    return nsnull;
  return sf->GetScrollableView();
}



nsIScrollableView* nsMenuPopupFrame::GetScrollableView(nsIFrame* aStart)
{
  if ( ! aStart )
    return nsnull;  

  nsIFrame* currFrame;
  nsIScrollableView* scrollableView=nsnull;

  
  currFrame=aStart;
  do {
    scrollableView = GetScrollableViewForFrame(currFrame);
    if ( scrollableView )
      return scrollableView;
    currFrame = currFrame->GetNextSibling();
  } while ( currFrame );

  
  nsIFrame* childFrame;
  currFrame=aStart;
  do {
    childFrame = currFrame->GetFirstChild(nsnull);
    scrollableView=GetScrollableView(childFrame);
    if ( scrollableView )
      return scrollableView;
    currFrame = currFrame->GetNextSibling();
  } while ( currFrame );

  return nsnull;
}

void nsMenuPopupFrame::EnsureMenuItemIsVisible(nsIMenuFrame* aMenuItem)
{
  nsIFrame* frame=nsnull;
  aMenuItem->QueryInterface(NS_GET_IID(nsIFrame), (void**)&frame);
  if ( frame ) {
    nsIFrame* childFrame=nsnull;
    childFrame = GetFirstChild(nsnull);
    nsIScrollableView *scrollableView;
    scrollableView=GetScrollableView(childFrame);
    if ( scrollableView ) {
      nscoord scrollX, scrollY;

      nsRect viewRect = scrollableView->View()->GetBounds();
      nsRect itemRect = frame->GetRect();
      scrollableView->GetScrollPosition(scrollX, scrollY);
  
      
      if ( itemRect.y + itemRect.height > scrollY + viewRect.height )
        scrollableView->ScrollTo(scrollX, itemRect.y + itemRect.height - viewRect.height, NS_SCROLL_PROPERTY_ALWAYS_BLIT);
      
      
      else if ( itemRect.y < scrollY )
        scrollableView->ScrollTo(scrollX, itemRect.y, NS_SCROLL_PROPERTY_ALWAYS_BLIT);
    }
  }
}

NS_IMETHODIMP nsMenuPopupFrame::SetCurrentMenuItem(nsIMenuFrame* aMenuItem)
{
  
  
  nsIMenuParent *contextMenu = GetContextMenu();
  if (contextMenu)
    return NS_OK;

  if (mCurrentMenu == aMenuItem)
    return NS_OK;
  
  
  if (mCurrentMenu) {
    PRBool isOpen = PR_FALSE;
    mCurrentMenu->MenuIsOpen(isOpen);
    mCurrentMenu->SelectMenu(PR_FALSE);
    
    if (mCurrentMenu && isOpen) {
      
      
      KillCloseTimer(); 
      PRInt32 menuDelay = 300;   

      PresContext()->LookAndFeel()->
        GetMetric(nsILookAndFeel::eMetric_SubmenuDelay, menuDelay);

      
      mCloseTimer = do_CreateInstance("@mozilla.org/timer;1");
      mCloseTimer->InitWithCallback(mTimerMediator, menuDelay, nsITimer::TYPE_ONE_SHOT);
      mTimerMenu = mCurrentMenu;
    }
  }

  
  if (aMenuItem) {
    EnsureMenuItemIsVisible(aMenuItem);
    aMenuItem->SelectMenu(PR_TRUE);
  }

  mCurrentMenu = aMenuItem;

  return NS_OK;
}


NS_IMETHODIMP
nsMenuPopupFrame::Escape(PRBool& aHandledFlag)
{
  mIncrementalString.Truncate();

  
  nsIMenuParent* contextMenu = GetContextMenu();
  if (contextMenu) {
    
    nsIFrame* childFrame;
    CallQueryInterface(contextMenu, &childFrame);
    nsIPopupSetFrame* popupSetFrame = GetPopupSetFrame(PresContext());
    if (popupSetFrame)
      
      popupSetFrame->DestroyPopup(childFrame, PR_FALSE);
    aHandledFlag = PR_TRUE;
    return NS_OK;
  }

  if (!mCurrentMenu)
    return NS_OK;

  
  PRBool isOpen = PR_FALSE;
  mCurrentMenu->MenuIsOpen(isOpen);
  if (isOpen) {
    
    mCurrentMenu->Escape(aHandledFlag);
    if (!aHandledFlag) {
      
      mCurrentMenu->OpenMenu(PR_FALSE);
      
      mCurrentMenu->SelectMenu(PR_TRUE);
      aHandledFlag = PR_TRUE;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::Enter()
{
  mIncrementalString.Truncate();

  
  nsIMenuParent *contextMenu = GetContextMenu();
  if (contextMenu)
    return contextMenu->Enter();

  
  if (mCurrentMenu)
    mCurrentMenu->Enter();

  return NS_OK;
}

nsIMenuParent*
nsMenuPopupFrame::GetContextMenu()
{
  if (mIsContextMenu)
    return nsnull;

  return nsMenuFrame::GetContextMenu();
}

nsIMenuFrame*
nsMenuPopupFrame::FindMenuWithShortcut(nsIDOMKeyEvent* aKeyEvent, PRBool& doAction)
{
  PRUint32 charCode, keyCode;
  aKeyEvent->GetCharCode(&charCode);
  aKeyEvent->GetKeyCode(&keyCode);

  doAction = PR_FALSE;

  
  nsIFrame* immediateParent = nsnull;
  GetInsertionPoint(PresContext()->PresShell(), this, nsnull,
                    &immediateParent);
  if (!immediateParent)
    immediateParent = this;

  PRUint32 matchCount = 0, matchShortcutCount = 0;
  PRBool foundActive = PR_FALSE;
  PRBool isShortcut;
  nsIMenuFrame* frameBefore = nsnull;
  nsIMenuFrame* frameAfter = nsnull;
  nsIMenuFrame* frameShortcut = nsnull;

  nsIContent* parentContent = mContent->GetParent();

  PRBool isMenu =
    parentContent && parentContent->Tag() != nsGkAtoms::menulist;

  static DOMTimeStamp lastKeyTime = 0;
  DOMTimeStamp keyTime;
  aKeyEvent->GetTimeStamp(&keyTime);

  if (charCode == 0) {
    if (keyCode == NS_VK_BACK) {
      if (!isMenu && !mIncrementalString.IsEmpty()) {
        mIncrementalString.SetLength(mIncrementalString.Length() - 1);
        return nsnull;
      }
      else {
#ifdef XP_WIN
        nsCOMPtr<nsISound> soundInterface = do_CreateInstance("@mozilla.org/sound;1");
        if (soundInterface)
          soundInterface->Beep();
#endif  
      }
    }
    return nsnull;
  }
  else {
    PRUnichar uniChar = ToLowerCase(NS_STATIC_CAST(PRUnichar, charCode));
    if (isMenu || 
        keyTime - lastKeyTime > INC_TYP_INTERVAL) 
      mIncrementalString = uniChar;
    else {
      mIncrementalString.Append(uniChar);
    }
  }

  
  nsAutoString incrementalString(mIncrementalString);
  PRUint32 charIndex = 1, stringLength = incrementalString.Length();
  while (charIndex < stringLength && incrementalString[charIndex] == incrementalString[charIndex - 1]) {
    charIndex++;
  }
  if (charIndex == stringLength) {
    incrementalString.Truncate(1);
    stringLength = 1;
  }

  lastKeyTime = keyTime;

  nsIFrame* currFrame;
  
  
  
  
  
  currFrame = immediateParent->GetFirstChild(nsnull);

  
  
  while (currFrame) {
    nsIContent* current = currFrame->GetContent();
    
    
    if (IsValidItem(current)) {
      nsAutoString textKey;
      
      current->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, textKey);
      if (textKey.IsEmpty()) { 
        isShortcut = PR_FALSE;
        current->GetAttr(kNameSpaceID_None, nsGkAtoms::label, textKey);
        if (textKey.IsEmpty()) 
          current->GetAttr(kNameSpaceID_None, nsGkAtoms::value, textKey);
      }
      else
        isShortcut = PR_TRUE;

      if (StringBeginsWith(textKey, incrementalString,
                           nsCaseInsensitiveStringComparator())) {
        
        nsIMenuFrame* menuFrame;
        if (NS_SUCCEEDED(CallQueryInterface(currFrame, &menuFrame))) {
          
          matchCount++;
          if (isShortcut) {
            
            matchShortcutCount++;
            
            frameShortcut = menuFrame;
          }
          if (!foundActive) {
            
            if (!frameBefore)
              frameBefore = menuFrame;
          }
          else {
            
            if (!frameAfter)
              frameAfter = menuFrame;
          }
        }
        else
          return nsnull;
      }

      
      if (current->AttrValueIs(kNameSpaceID_None, nsGkAtoms::menuactive,
                               nsGkAtoms::_true, eCaseMatters)) {
        foundActive = PR_TRUE;
        if (stringLength > 1) {
          
          
          nsIMenuFrame* menuFrame;
          if (NS_SUCCEEDED(CallQueryInterface(currFrame, &menuFrame)) &&
              menuFrame == frameBefore) {
            return frameBefore;
          }
        }
      }
    }
    currFrame = currFrame->GetNextSibling();
  }

  doAction = (isMenu && (matchCount == 1 || matchShortcutCount == 1));

  if (matchShortcutCount == 1) 
    return frameShortcut;
  if (frameAfter) 
    return frameAfter;
  else if (frameBefore) 
    return frameBefore;

  
  mIncrementalString.SetLength(mIncrementalString.Length() - 1);

  
#ifdef XP_WIN
  
  
  if (isMenu) {
    nsCOMPtr<nsISound> soundInterface = do_CreateInstance("@mozilla.org/sound;1");
    if (soundInterface)
      soundInterface->Beep();
  }
#endif  

  return nsnull;
}

NS_IMETHODIMP 
nsMenuPopupFrame::ShortcutNavigation(nsIDOMKeyEvent* aKeyEvent, PRBool& aHandledFlag)
{
  
  nsIMenuParent *contextMenu = GetContextMenu();
  if (contextMenu)
    return contextMenu->ShortcutNavigation(aKeyEvent, aHandledFlag);

  if (mCurrentMenu) {
    PRBool isOpen = PR_FALSE;
    mCurrentMenu->MenuIsOpen(isOpen);
    if (isOpen) {
      
      mCurrentMenu->ShortcutNavigation(aKeyEvent, aHandledFlag);
      return NS_OK;
    }
  }

  
  PRBool action;
  nsIMenuFrame* result = FindMenuWithShortcut(aKeyEvent, action);
  if (result) {
    
    nsIFrame* frame = nsnull;
    CallQueryInterface(result, &frame);
    nsWeakFrame weakResult(frame);
    aHandledFlag = PR_TRUE;
    SetCurrentMenuItem(result);
    if (action && weakResult.IsAlive()) {
      result->Enter();
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::KeyboardNavigation(PRUint32 aKeyCode, PRBool& aHandledFlag)
{
  
  nsIMenuParent *contextMenu = GetContextMenu();
  if (contextMenu)
    return contextMenu->KeyboardNavigation(aKeyCode, aHandledFlag);

  nsNavigationDirection theDirection;
  NS_DIRECTION_FROM_KEY_CODE(theDirection, aKeyCode);

  mIncrementalString.Truncate();

  
  if (!mCurrentMenu && NS_DIRECTION_IS_INLINE(theDirection)) {
    
    
    if (theDirection == eNavigationDirection_End) {
      nsIMenuFrame* nextItem = GetNextMenuItem(nsnull);
      if (nextItem) {
        aHandledFlag = PR_TRUE;
        SetCurrentMenuItem(nextItem);
      }
    }
    return NS_OK;
  }

  PRBool isContainer = PR_FALSE;
  PRBool isOpen = PR_FALSE;
  PRBool isDisabled = PR_FALSE;
  nsWeakFrame weakFrame(this);
  if (mCurrentMenu) {
    mCurrentMenu->MenuIsContainer(isContainer);
    mCurrentMenu->MenuIsOpen(isOpen);
    mCurrentMenu->MenuIsDisabled(isDisabled);

    if (isOpen) {
      
      mCurrentMenu->KeyboardNavigation(aKeyCode, aHandledFlag);
      NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);
    }
    else if (theDirection == eNavigationDirection_End &&
             isContainer && !isDisabled) {
      
      aHandledFlag = PR_TRUE;
      nsIFrame* frame = nsnull;
      CallQueryInterface(mCurrentMenu, &frame);
      nsWeakFrame weakCurrentFrame(frame);
      mCurrentMenu->OpenMenu(PR_TRUE);
      NS_ENSURE_TRUE(weakCurrentFrame.IsAlive(), NS_OK);
      mCurrentMenu->SelectFirstItem();
      NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);
    }
  }

  if (aHandledFlag)
    return NS_OK; 

  
  if (NS_DIRECTION_IS_BLOCK(theDirection) ||
      NS_DIRECTION_IS_BLOCK_TO_EDGE(theDirection)) {

    nsIMenuFrame* nextItem;
    
    if (theDirection == eNavigationDirection_Before)
      nextItem = GetPreviousMenuItem(mCurrentMenu);
    else if (theDirection == eNavigationDirection_After)
      nextItem = GetNextMenuItem(mCurrentMenu);
    else if (theDirection == eNavigationDirection_First)
      nextItem = GetNextMenuItem(nsnull);
    else
      nextItem = GetPreviousMenuItem(nsnull);

    if (nextItem) {
      aHandledFlag = PR_TRUE;
      SetCurrentMenuItem(nextItem);
    }
  }
  else if (mCurrentMenu && isContainer && isOpen) {
    if (theDirection == eNavigationDirection_Start) {
      
      mCurrentMenu->OpenMenu(PR_FALSE);
      NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);
      
      mCurrentMenu->SelectMenu(PR_TRUE);
      aHandledFlag = PR_TRUE;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::GetParentPopup(nsIMenuParent** aMenuParent)
{
  *aMenuParent = nsnull;
  nsIFrame* parent = GetParent();
  while (parent) {
    nsCOMPtr<nsIMenuParent> menuParent = do_QueryInterface(parent);
    if (menuParent) {
      *aMenuParent = menuParent.get();
      NS_ADDREF(*aMenuParent);
      return NS_OK;
    }
    parent = parent->GetParent();
  }
  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::HideChain()
{
  if (!mShouldRollup)
    return NS_OK;

  
  
  
  nsMenuDismissalListener::Shutdown();
  
  nsIFrame* frame = GetParent();
  if (frame) {
    nsWeakFrame weakMenu(frame);
    nsIMenuFrame* menuFrame;
    if (NS_FAILED(CallQueryInterface(frame, &menuFrame))) {
      nsIPopupSetFrame* popupSetFrame = GetPopupSetFrame(PresContext());
      if (popupSetFrame)
        
        popupSetFrame->HidePopup(this);
      return NS_OK;
    }
   
    menuFrame->ActivateMenu(PR_FALSE);
    NS_ENSURE_TRUE(weakMenu.IsAlive(), NS_OK);
    menuFrame->SelectMenu(PR_FALSE);
    NS_ENSURE_TRUE(weakMenu.IsAlive(), NS_OK);

    
    nsIMenuParent *menuParent = menuFrame->GetMenuParent();
    if (menuParent)
      menuParent->HideChain();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::DismissChain()
{
  if (!mShouldRollup)
    return NS_OK;

  
  nsMenuDismissalListener::Shutdown();
  
  
  nsIFrame* frame = GetParent();
  if (frame) {
    nsIMenuFrame *menuFrame = nsnull;
    CallQueryInterface(frame, &menuFrame);
    if (!menuFrame) {
      nsIPopupSetFrame* popupSetFrame = GetPopupSetFrame(PresContext());
      if (popupSetFrame) {
        
        if (mCurrentMenu) {
          PRBool wasOpen;
          mCurrentMenu->MenuIsOpen(wasOpen);
          if (wasOpen)
            mCurrentMenu->OpenMenu(PR_FALSE);
          mCurrentMenu->SelectMenu(PR_FALSE);
        }
        
        popupSetFrame->DestroyPopup(this, PR_TRUE);
      }
      return NS_OK;
    }
  
    menuFrame->OpenMenu(PR_FALSE);

    
    nsIMenuParent* menuParent = menuFrame->GetMenuParent();
    if (menuParent)
      menuParent->DismissChain();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::GetWidget(nsIWidget **aWidget)
{
  
  nsIView * view = nsnull;
  
  nsMenuPopupFrame::GetRootViewForPopup(this, PR_FALSE, &view);
  if (!view)
    return NS_OK;

  *aWidget = view->GetWidget();
  NS_IF_ADDREF(*aWidget);
  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::AttachedDismissalListener()
{
  mConsumeRollupEvent = nsIPopupBoxObject::ROLLUP_DEFAULT;
  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::InstallKeyboardNavigator()
{
  if (mKeyboardNavigator)
    return NS_OK;

  nsCOMPtr<nsIDOMEventReceiver> target = do_QueryInterface(mContent->GetDocument());
  
  mTarget = target;
  mKeyboardNavigator = new nsMenuListener(this);
  NS_IF_ADDREF(mKeyboardNavigator);

  target->AddEventListener(NS_LITERAL_STRING("keypress"), (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE); 
  target->AddEventListener(NS_LITERAL_STRING("keydown"), (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE);  
  target->AddEventListener(NS_LITERAL_STRING("keyup"), (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE);   
  
  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::RemoveKeyboardNavigator()
{
  if (!mKeyboardNavigator)
    return NS_OK;

  mTarget->RemoveEventListener(NS_LITERAL_STRING("keypress"), (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE);
  mTarget->RemoveEventListener(NS_LITERAL_STRING("keydown"), (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE);
  mTarget->RemoveEventListener(NS_LITERAL_STRING("keyup"), (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE);
  
  NS_IF_RELEASE(mKeyboardNavigator);

  return NS_OK;
}



PRBool 
nsMenuPopupFrame::IsValidItem(nsIContent* aContent)
{
  nsIAtom *tag = aContent->Tag();
  
  PRBool skipNavigatingDisabledMenuItem;
  PresContext()->LookAndFeel()->
    GetMetric(nsILookAndFeel::eMetric_SkipNavigatingDisabledMenuItem,
              skipNavigatingDisabledMenuItem);

  PRBool result = (tag == nsGkAtoms::menu ||
                   tag == nsGkAtoms::menuitem ||
                   tag == nsGkAtoms::option);
  if (skipNavigatingDisabledMenuItem)
    result = result && !IsDisabled(aContent);

  return result;
}

PRBool 
nsMenuPopupFrame::IsDisabled(nsIContent* aContent)
{
  return aContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::disabled,
                               nsGkAtoms::_true, eCaseMatters);
}

NS_IMETHODIMP 
nsMenuPopupFrame::AttributeChanged(PRInt32 aNameSpaceID,
                                   nsIAtom* aAttribute,
                                   PRInt32 aModType)

{
  nsresult rv = nsBoxFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                             aModType);
  
  if (aAttribute == nsGkAtoms::left || aAttribute == nsGkAtoms::top)
    MoveToAttributePosition();
  
  return rv;
}

void 
nsMenuPopupFrame::MoveToAttributePosition()
{
  
  
  
  
  nsAutoString left, top;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::left, left);
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::top, top);
  PRInt32 err1, err2, xPos, yPos;
  xPos = left.ToInteger(&err1);
  yPos = top.ToInteger(&err2);

  if (NS_SUCCEEDED(err1) && NS_SUCCEEDED(err2)) {
    MoveToInternal(xPos, yPos);
  }
}


NS_IMETHODIMP 
nsMenuPopupFrame::HandleEvent(nsPresContext* aPresContext, 
                              nsGUIEvent*     aEvent,
                              nsEventStatus*  aEventStatus)
{
  return nsBoxFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
}

void
nsMenuPopupFrame::Destroy()
{
  
  
  mTimerMediator->ClearFrame();

  if (mCloseTimer)
    mCloseTimer->Cancel();

  nsPresContext* rootPresContext = PresContext()->RootPresContext();
  if (rootPresContext->ContainsActivePopup(this)) {
    rootPresContext->NotifyRemovedActivePopup(this);
  }

  RemoveKeyboardNavigator();
  nsBoxFrame::Destroy();
}













































nsresult
nsMenuPopupFrame::Notify(nsITimer* aTimer)
{
  
  if (aTimer == mCloseTimer.get()) {
    PRBool menuOpen = PR_FALSE;
    mTimerMenu->MenuIsOpen(menuOpen);
    if (menuOpen)
      mTimerMenu->OpenMenu(PR_FALSE);

    if (mCloseTimer)
      mCloseTimer->Cancel();
  }
  
  mCloseTimer = nsnull;
  mTimerMenu = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::KillCloseTimer()
{
  if (mCloseTimer && mTimerMenu) {
    PRBool menuOpen = PR_FALSE;
    mTimerMenu->MenuIsOpen(menuOpen);
    if (menuOpen) {
      mTimerMenu->OpenMenu(PR_FALSE);
    }
    mCloseTimer->Cancel();
    mCloseTimer = nsnull;
    mTimerMenu = nsnull;
  }
  return NS_OK;
}



NS_IMETHODIMP
nsMenuPopupFrame::KillPendingTimers ( )
{
  return KillCloseTimer();

} 

NS_IMETHODIMP
nsMenuPopupFrame::CancelPendingTimers()
{
  if (mCloseTimer && mTimerMenu) {
    if (mTimerMenu != mCurrentMenu) {
      SetCurrentMenuItem(mTimerMenu);
    }
    mCloseTimer->Cancel();
    mCloseTimer = nsnull;
    mTimerMenu = nsnull;
  }
  return NS_OK;
}

void
nsMenuPopupFrame::MoveTo(PRInt32 aLeft, PRInt32 aTop)
{
  
  nsAutoString left, top;
  left.AppendInt(aLeft);
  top.AppendInt(aTop);

  nsWeakFrame weakFrame(this);
  mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::left, left, PR_FALSE);
  if (!weakFrame.IsAlive()) {
    return;
  }
  mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::top, top, PR_FALSE);
  if (!weakFrame.IsAlive()) {
    return;
  }

  MoveToInternal(aLeft, aTop);
}

void
nsMenuPopupFrame::MoveToInternal(PRInt32 aLeft, PRInt32 aTop)
{
  
  if (mInContentShell)
    return;

  nsIView* view = GetView();
  NS_ASSERTION(view->GetParent(), "Must have parent!");
  
  
  nsIntPoint screenPos = view->GetParent()->GetScreenPosition();

  
  
  view->GetWidget()->Move(aLeft - screenPos.x, aTop - screenPos.y);
}

void 
nsMenuPopupFrame::GetAutoPosition(PRBool* aShouldAutoPosition)
{
  *aShouldAutoPosition = mShouldAutoPosition;
}

void
nsMenuPopupFrame::SetAutoPosition(PRBool aShouldAutoPosition)
{
  mShouldAutoPosition = aShouldAutoPosition;
}

void
nsMenuPopupFrame::EnableRollup(PRBool aShouldRollup)
{
  if (!nsMenuDismissalListener::sInstance ||
       nsMenuDismissalListener::sInstance->GetCurrentMenuParent() != this)
    return;

  if (aShouldRollup)
    nsMenuDismissalListener::sInstance->Register();
  else
    nsMenuDismissalListener::sInstance->Unregister();
}

void
nsMenuPopupFrame::SetConsumeRollupEvent(PRUint32 aConsumeMode)
{
  mConsumeRollupEvent = aConsumeMode;
}


NS_IMPL_ISUPPORTS1(nsMenuPopupTimerMediator, nsITimerCallback)





nsMenuPopupTimerMediator::nsMenuPopupTimerMediator(nsMenuPopupFrame *aFrame) :
  mFrame(aFrame)
{
  NS_ASSERTION(mFrame, "Must have frame");
}

nsMenuPopupTimerMediator::~nsMenuPopupTimerMediator()
{
}






NS_IMETHODIMP nsMenuPopupTimerMediator::Notify(nsITimer* aTimer)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  return mFrame->Notify(aTimer);
}





void nsMenuPopupTimerMediator::ClearFrame()
{
  mFrame = nsnull;
}

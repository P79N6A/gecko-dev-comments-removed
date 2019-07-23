





































#include "nsNativeScrollbarFrame.h"
#include "nsGkAtoms.h"
#include "nsBoxLayoutState.h"
#include "nsComponentManagerUtils.h"
#include "nsGUIEvent.h"
#include "nsIDeviceContext.h"
#include "nsIView.h"
#include "nsINativeScrollbar.h"
#include "nsIScrollbarFrame.h"
#include "nsIScrollbarMediator.h"
#include "nsWidgetsCID.h"
#include "nsINameSpaceManager.h"






nsIFrame*
NS_NewNativeScrollbarFrame (nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsNativeScrollbarFrame (aPresShell, aContext);
} 





NS_IMETHODIMP
nsNativeScrollbarFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  if (!aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(NS_GET_IID(nsIScrollbarMediator))) {
    *aInstancePtr = (void*) ((nsIScrollbarMediator*) this);
    return NS_OK;
  }
  return nsBoxFrame::QueryInterface(aIID, aInstancePtr);
}






NS_IMETHODIMP
nsNativeScrollbarFrame::Init(nsIContent*     aContent,
                             nsIFrame*       aParent,
                             nsIFrame*       aPrevInFlow)
{
  nsresult  rv = nsBoxFrame::Init(aContent, aParent, aPrevInFlow);

  
  
  
  
  static NS_DEFINE_IID(kScrollbarCID,  NS_NATIVESCROLLBAR_CID);
  if ( NS_SUCCEEDED(CreateViewForFrame(GetPresContext(), this, GetStyleContext(), PR_TRUE)) ) {
    nsIView* myView = GetView();
    if ( myView ) {
      nsWidgetInitData widgetData;
      if ( NS_SUCCEEDED(myView->CreateWidget(kScrollbarCID, &widgetData, nsnull)) ) {
        mScrollbar = myView->GetWidget();
        if (mScrollbar) {
          mScrollbar->Show(PR_TRUE);
          mScrollbar->Enable(PR_TRUE);

          
          
          
          mScrollbarNeedsContent = PR_TRUE;
        } else {
          NS_WARNING("Couldn't create native scrollbar!");
          return NS_ERROR_FAILURE;
        }
      }
    }
  }
  
  return rv;
}

void
nsNativeScrollbarFrame::Destroy()
{
  nsCOMPtr<nsINativeScrollbar> scrollbar(do_QueryInterface(mScrollbar));
  if (scrollbar) {
    
    
    scrollbar->SetContent(nsnull, nsnull, nsnull);
  }

  nsBoxFrame::Destroy();
}









nsNativeScrollbarFrame::Parts
nsNativeScrollbarFrame::FindParts()
{
  nsIFrame* f;
  for (f = GetParent(); f; f = f->GetParent()) {
    nsIContent* currContent = f->GetContent();

    if (currContent && currContent->Tag() == nsGkAtoms::scrollbar) {
      nsIScrollbarFrame* sb;
      CallQueryInterface(f, &sb);
      if (sb)
        return Parts(f, sb, sb->GetScrollbarMediator());
    }
  }

  return Parts(nsnull, nsnull, nsnull);
}

NS_IMETHODIMP
nsNativeScrollbarFrame::Reflow(nsPresContext*          aPresContext,
                               nsHTMLReflowMetrics&     aDesiredSize,
                               const nsHTMLReflowState& aReflowState,
                               nsReflowStatus&          aStatus)
{
  nsresult rv = nsBoxFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (aReflowState.availableWidth == 0) {
    aDesiredSize.width = 0;
  }
  if (aReflowState.availableHeight == 0) {
    aDesiredSize.height = 0;
  }

  return NS_OK;
}









NS_IMETHODIMP
nsNativeScrollbarFrame::AttributeChanged(PRInt32 aNameSpaceID,
                                         nsIAtom* aAttribute,
                                         PRInt32 aModType)
{
  nsresult rv = nsBoxFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                             aModType);
  
  if (  aAttribute == nsGkAtoms::curpos ||
        aAttribute == nsGkAtoms::maxpos || 
        aAttribute == nsGkAtoms::pageincrement ||
        aAttribute == nsGkAtoms::increment ) {
    nsAutoString valueStr;
    mContent->GetAttr(aNameSpaceID, aAttribute, valueStr);
    
    PRInt32 error;
    PRInt32 value = valueStr.ToInteger(&error);
    if (value < 0)
      value = 1;          

    nsCOMPtr<nsINativeScrollbar> scrollbar(do_QueryInterface(mScrollbar));
    if (scrollbar) {
      if (aAttribute == nsGkAtoms::maxpos) {
        
        PRUint32 maxValue = (PRUint32)value;
        PRUint32 current;
        scrollbar->GetPosition(&current);
        if (current > maxValue)
        {
          PRInt32 oldPosition = (PRInt32)current;
          PRInt32 curPosition = maxValue;
        
          Parts parts = FindParts();
          if (parts.mMediator) {
            parts.mMediator->PositionChanged(parts.mIScrollbarFrame, oldPosition,  curPosition);
          }

          nsAutoString currentStr;
          currentStr.AppendInt(curPosition);
          parts.mScrollbarFrame->GetContent()->
            SetAttr(kNameSpaceID_None, nsGkAtoms::curpos, currentStr, PR_TRUE);
        }
      }
      
      if ( aAttribute == nsGkAtoms::curpos )
        scrollbar->SetPosition(value);
      else if ( aAttribute == nsGkAtoms::maxpos )
        scrollbar->SetMaxRange(value);
      else if ( aAttribute == nsGkAtoms::pageincrement )   
        scrollbar->SetViewSize(value);
     else if ( aAttribute == nsGkAtoms::increment )
        scrollbar->SetLineIncrement(value);
    }
  }

  return rv;
}








nsSize
nsNativeScrollbarFrame::GetPrefSize(nsBoxLayoutState& aState)
{
  nsSize size(0,0);
  DISPLAY_PREF_SIZE(this, size);

  PRInt32 narrowDimension = 0;
  nsCOMPtr<nsINativeScrollbar> native ( do_QueryInterface(mScrollbar) );
  if ( !native ) return size;
  native->GetNarrowSize(&narrowDimension);

  if ( IsVertical() )
    size.width = aState.PresContext()->DevPixelsToAppUnits(narrowDimension);
  else
    size.height = aState.PresContext()->DevPixelsToAppUnits(narrowDimension);

  
  
  
  Hookup();

  return size;
}










void
nsNativeScrollbarFrame::Hookup()
{
  if (!mScrollbarNeedsContent)
    return;

  nsCOMPtr<nsINativeScrollbar> scrollbar(do_QueryInterface(mScrollbar));
  if (!scrollbar) {
    NS_WARNING("Native scrollbar widget doesn't implement nsINativeScrollbar");
    return;
  }

  Parts parts = FindParts();
  if (!parts.mScrollbarFrame) {
    
    return;
  }
  
  
  
  nsIContent* scrollbarContent = parts.mScrollbarFrame->GetContent();
  scrollbar->SetContent(scrollbarContent,
                        parts.mIScrollbarFrame, parts.mMediator ? this : nsnull);
  mScrollbarNeedsContent = PR_FALSE;

  if (!scrollbarContent)
    return;

  
  

  nsAutoString value;
  scrollbarContent->GetAttr(kNameSpaceID_None, nsGkAtoms::curpos, value);

  PRInt32 error;
  PRUint32 curpos = value.ToInteger(&error);
  if (!curpos || error)
    return;

  scrollbar->SetPosition(curpos);
}

NS_IMETHODIMP
nsNativeScrollbarFrame::PositionChanged(nsISupports* aScrollbar, PRInt32 aOldIndex, PRInt32& aNewIndex)
{
  Parts parts = FindParts();
  if (!parts.mMediator)
    return NS_OK;
  return parts.mMediator->PositionChanged(aScrollbar, aOldIndex, aNewIndex);
}

NS_IMETHODIMP
nsNativeScrollbarFrame::ScrollbarButtonPressed(nsISupports* aScrollbar, PRInt32 aOldIndex, PRInt32 aNewIndex)
{
  Parts parts = FindParts();
  if (!parts.mMediator)
    return NS_OK;
  return parts.mMediator->ScrollbarButtonPressed(aScrollbar, aOldIndex, aNewIndex);
}

NS_IMETHODIMP
nsNativeScrollbarFrame::VisibilityChanged(nsISupports* aScrollbar, PRBool aVisible)
{
  Parts parts = FindParts();
  if (!parts.mMediator)
    return NS_OK;
  return parts.mMediator->VisibilityChanged(aScrollbar, aVisible);
}

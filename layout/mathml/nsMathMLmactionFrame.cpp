





































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsINameSpaceManager.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"

#include "nsCSSRendering.h"
#include "prprf.h"         

#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIWebBrowserChrome.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDOMElement.h"

#include "nsIDOMEventTarget.h"
#include "nsIDOMMouseListener.h"

#include "nsMathMLmactionFrame.h"
#include "nsAutoPtr.h"
#include "nsStyleSet.h"
#include "nsDisplayList.h"
#include "nsContentUtils.h"





#define NS_MATHML_ACTION_TYPE_NONE         0
#define NS_MATHML_ACTION_TYPE_TOGGLE       1
#define NS_MATHML_ACTION_TYPE_STATUSLINE   2
#define NS_MATHML_ACTION_TYPE_TOOLTIP      3 // unsupported
#define NS_MATHML_ACTION_TYPE_RESTYLE      4


nsIFrame*
NS_NewMathMLmactionFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmactionFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmactionFrame)

nsMathMLmactionFrame::~nsMathMLmactionFrame()
{
  
  
  if (mListener)
    mContent->RemoveEventListenerByIID(mListener, NS_GET_IID(nsIDOMMouseListener));
}

NS_IMETHODIMP
nsMathMLmactionFrame::Init(nsIContent*      aContent,
                           nsIFrame*        aParent,
                           nsIFrame*        aPrevInFlow)
{
  nsAutoString value, prefix;

  

  mChildCount = -1; 
  mSelection = 0;
  mSelectedFrame = nsnull;
  nsRefPtr<nsStyleContext> newStyleContext;

  mActionType = NS_MATHML_ACTION_TYPE_NONE;
  aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::actiontype_, value);
  if (!value.IsEmpty()) {
    if (value.EqualsLiteral("toggle"))
      mActionType = NS_MATHML_ACTION_TYPE_TOGGLE;

    

    if (NS_MATHML_ACTION_TYPE_NONE == mActionType) {
      
      if (8 < value.Length() && 0 == value.Find("tooltip#"))
        mActionType = NS_MATHML_ACTION_TYPE_TOOLTIP;
    }

    if (NS_MATHML_ACTION_TYPE_NONE == mActionType) {
      
      if (11 < value.Length() && 0 == value.Find("statusline#"))
        mActionType = NS_MATHML_ACTION_TYPE_STATUSLINE;
    }

    if (NS_MATHML_ACTION_TYPE_NONE == mActionType) {
      
      if (8 < value.Length() && 0 == value.Find("restyle#")) {
        mActionType = NS_MATHML_ACTION_TYPE_RESTYLE;
        mRestyle = value;

        
        
        

        
        
        
        PRBool notify = PR_FALSE; 
        aContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::actiontype_, notify);

        
        nsStyleContext* parentStyleContext = GetStyleContext()->GetParent();
        newStyleContext = PresContext()->StyleSet()->
          ResolveStyleFor(aContent, parentStyleContext);

        if (!newStyleContext) 
          mRestyle.Truncate();
        else {
          if (newStyleContext != GetStyleContext())
            SetStyleContextWithoutNotification(newStyleContext);
          else
            mRestyle.Truncate();
        }
      }
    }
  }

  
  return nsMathMLContainerFrame::Init(aContent, aParent, aPrevInFlow);
}

nsresult
nsMathMLmactionFrame::ChildListChanged(PRInt32 aModType)
{
  
  mChildCount = -1;
  mSelection = 0;
  mSelectedFrame = nsnull;
  GetSelectedFrame();

  return nsMathMLContainerFrame::ChildListChanged(aModType);
}


nsIFrame* 
nsMathMLmactionFrame::GetSelectedFrame()
{
  nsAutoString value;
  PRInt32 selection; 

  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::selection_, value);
  if (!value.IsEmpty()) {
    PRInt32 errorCode;
    selection = value.ToInteger(&errorCode);
    if (NS_FAILED(errorCode)) 
      selection = 1;
  }
  else selection = 1; 

  if (-1 != mChildCount) { 
    
    if (selection > mChildCount || selection < 1) 
      selection = 1;
    
    if (selection == mSelection) 
      return mSelectedFrame;
  }

  
  PRInt32 count = 0;
  nsIFrame* childFrame = mFrames.FirstChild();
  while (childFrame) {
    if (!mSelectedFrame) 
      mSelectedFrame = childFrame; 
    if (++count == selection) 
      mSelectedFrame = childFrame;

    childFrame = childFrame->GetNextSibling();
  }
  
  if (selection > count || selection < 1) 
    selection = 1;

  mChildCount = count;
  mSelection = selection;

  
  
  mPresentationData.baseFrame = mSelectedFrame;
  GetEmbellishDataFrom(mSelectedFrame, mEmbellishData);

  return mSelectedFrame;
}

NS_IMETHODIMP
nsMathMLmactionFrame::SetInitialChildList(nsIAtom*        aListName,
                                          nsFrameList&    aChildList)
{
  nsresult rv = nsMathMLContainerFrame::SetInitialChildList(aListName, aChildList);

  
  
  if (!GetSelectedFrame()) {
    mActionType = NS_MATHML_ACTION_TYPE_NONE;
  }
  else {
    
    mListener = new nsMathMLmactionFrame::MouseListener(this);
    
    mContent->AddEventListenerByIID(mListener, NS_GET_IID(nsIDOMMouseListener));
  }
  return rv;
}


NS_IMETHODIMP
nsMathMLmactionFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                       const nsRect&           aDirtyRect,
                                       const nsDisplayListSet& aLists)
{
  nsresult rv = DisplayBorderBackgroundOutline(aBuilder, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIFrame* childFrame = GetSelectedFrame();
  if (childFrame) {
    
    nsDisplayListSet set(aLists, aLists.Content());
    
    rv = BuildDisplayListForChild(aBuilder, childFrame, aDirtyRect, set);
    NS_ENSURE_SUCCESS(rv, rv);
  }

#if defined(NS_DEBUG) && defined(SHOW_BOUNDING_BOX)
  
  rv = DisplayBoundingMetrics(aBuilder, this, mReference, mBoundingMetrics, aLists);
#endif
  return rv;
}


NS_IMETHODIMP
nsMathMLmactionFrame::Reflow(nsPresContext*          aPresContext,
                             nsHTMLReflowMetrics&     aDesiredSize,
                             const nsHTMLReflowState& aReflowState,
                             nsReflowStatus&          aStatus)
{
  nsresult rv = NS_OK;
  aStatus = NS_FRAME_COMPLETE;
  aDesiredSize.width = aDesiredSize.height = 0;
  aDesiredSize.ascent = 0;
  mBoundingMetrics.Clear();
  nsIFrame* childFrame = GetSelectedFrame();
  if (childFrame) {
    nsSize availSize(aReflowState.ComputedWidth(), NS_UNCONSTRAINEDSIZE);
    nsHTMLReflowState childReflowState(aPresContext, aReflowState,
                                       childFrame, availSize);
    rv = ReflowChild(childFrame, aPresContext, aDesiredSize,
                     childReflowState, aStatus);
    SaveReflowAndBoundingMetricsFor(childFrame, aDesiredSize,
                                    aDesiredSize.mBoundingMetrics);
    mBoundingMetrics = aDesiredSize.mBoundingMetrics;
  }
  FinalizeReflow(*aReflowState.rendContext, aDesiredSize);
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return rv;
}


 nsresult
nsMathMLmactionFrame::Place(nsIRenderingContext& aRenderingContext,
                            PRBool               aPlaceOrigin,
                            nsHTMLReflowMetrics& aDesiredSize)
{
  aDesiredSize.width = aDesiredSize.height = 0;
  aDesiredSize.ascent = 0;
  mBoundingMetrics.Clear();
  nsIFrame* childFrame = GetSelectedFrame();
  if (childFrame) {
    GetReflowAndBoundingMetricsFor(childFrame, aDesiredSize, mBoundingMetrics);
    if (aPlaceOrigin) {
      FinishReflowChild(childFrame, PresContext(), nsnull, aDesiredSize, 0, 0, 0);
    }
    mReference.x = 0;
    mReference.y = aDesiredSize.ascent;
  }
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;
  return NS_OK;
}





NS_IMPL_ISUPPORTS2(nsMathMLmactionFrame::MouseListener,
                   nsIDOMEventListener,
                   nsIDOMMouseListener)




void
ShowStatus(nsPresContext* aPresContext, nsString& aStatusMsg)
{
  nsCOMPtr<nsISupports> cont = aPresContext->GetContainer();
  if (cont) {
    nsCOMPtr<nsIDocShellTreeItem> docShellItem(do_QueryInterface(cont));
    if (docShellItem) {
      nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
      docShellItem->GetTreeOwner(getter_AddRefs(treeOwner));
      if (treeOwner) {
        nsCOMPtr<nsIWebBrowserChrome> browserChrome(do_GetInterface(treeOwner));
        if (browserChrome) {
          browserChrome->SetStatus(nsIWebBrowserChrome::STATUS_LINK, aStatusMsg.get());
        }
      }
    }
  }
}

NS_IMETHODIMP
nsMathMLmactionFrame::MouseListener::MouseOver(nsIDOMEvent* aMouseEvent)
{
  mOwner->MouseOver();
  return NS_OK;
}

void
nsMathMLmactionFrame::MouseOver()
{
  
  if (NS_MATHML_ACTION_TYPE_STATUSLINE == mActionType) {
    nsAutoString value;
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::actiontype_, value);
    
    if (11 < value.Length() && 0 == value.Find("statusline#")) {
      value.Cut(0, 11);
      ShowStatus(PresContext(), value);
    }
  }
}

NS_IMETHODIMP
nsMathMLmactionFrame::MouseListener::MouseOut(nsIDOMEvent* aMouseEvent) 
{
  mOwner->MouseOut();
  return NS_OK;
}

void
nsMathMLmactionFrame::MouseOut()
{
  
  if (NS_MATHML_ACTION_TYPE_STATUSLINE == mActionType) {
    nsAutoString value;
    value.SetLength(0);
    ShowStatus(PresContext(), value);
  }
}

NS_IMETHODIMP
nsMathMLmactionFrame::MouseListener::MouseClick(nsIDOMEvent* aMouseEvent)
{
  mOwner->MouseClick();
  return NS_OK;
}

void
nsMathMLmactionFrame::MouseClick()
{
  if (NS_MATHML_ACTION_TYPE_TOGGLE == mActionType) {
    if (mChildCount > 1) {
      PRInt32 selection = (mSelection == mChildCount)? 1 : mSelection + 1;
      nsAutoString value;
      char cbuf[10];
      PR_snprintf(cbuf, sizeof(cbuf), "%d", selection);
      value.AssignASCII(cbuf);
      PRBool notify = PR_FALSE; 
      mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::selection_, value, notify);

      
      PresContext()->PresShell()->
        FrameNeedsReflow(mSelectedFrame, nsIPresShell::eTreeChange,
                         NS_FRAME_IS_DIRTY);
    }
  }
  else if (NS_MATHML_ACTION_TYPE_RESTYLE == mActionType) {
    if (!mRestyle.IsEmpty()) {
      nsCOMPtr<nsIDOMElement> node( do_QueryInterface(mContent) );
      if (node.get()) {
        if (nsContentUtils::HasNonEmptyAttr(mContent, kNameSpaceID_None,
                                            nsGkAtoms::actiontype_))
          node->RemoveAttribute(NS_LITERAL_STRING("actiontype"));
        else
          node->SetAttribute(NS_LITERAL_STRING("actiontype"), mRestyle);

        
        PresContext()->PresShell()->
          FrameNeedsReflow(mSelectedFrame, nsIPresShell::eStyleChange,
                           NS_FRAME_IS_DIRTY);
      }
    }
  }
}

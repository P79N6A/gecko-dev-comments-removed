




#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsINameSpaceManager.h"

#include "nsCSSRendering.h"
#include "prprf.h"         

#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIWebBrowserChrome.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDOMElement.h"
#include "nsTextFragment.h"

#include "nsIDOMEventTarget.h"

#include "nsMathMLmactionFrame.h"
#include "nsAutoPtr.h"
#include "nsStyleSet.h"
#include "nsDisplayList.h"





enum nsMactionActionTypes {
  NS_MATHML_ACTION_TYPE_CLASS_ERROR            = 0x10,
  NS_MATHML_ACTION_TYPE_CLASS_USE_SELECTION    = 0x20,
  NS_MATHML_ACTION_TYPE_CLASS_IGNORE_SELECTION = 0x40,
  NS_MATHML_ACTION_TYPE_CLASS_BITMASK          = 0xF0,

  NS_MATHML_ACTION_TYPE_NONE       = NS_MATHML_ACTION_TYPE_CLASS_ERROR|0x01,

  NS_MATHML_ACTION_TYPE_TOGGLE     = NS_MATHML_ACTION_TYPE_CLASS_USE_SELECTION|0x01,
  NS_MATHML_ACTION_TYPE_UNKNOWN    = NS_MATHML_ACTION_TYPE_CLASS_USE_SELECTION|0x02,

  NS_MATHML_ACTION_TYPE_STATUSLINE = NS_MATHML_ACTION_TYPE_CLASS_IGNORE_SELECTION|0x01,
  NS_MATHML_ACTION_TYPE_TOOLTIP    = NS_MATHML_ACTION_TYPE_CLASS_IGNORE_SELECTION|0x02
};



static int32_t
GetActionType(nsIContent* aContent)
{
  nsAutoString value;

  if (aContent) {
    if (!aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::actiontype_, value))
      return NS_MATHML_ACTION_TYPE_NONE; 
  }

  if (value.EqualsLiteral("toggle"))
    return NS_MATHML_ACTION_TYPE_TOGGLE;
  if (value.EqualsLiteral("statusline"))
    return NS_MATHML_ACTION_TYPE_STATUSLINE;
  if (value.EqualsLiteral("tooltip"))
    return NS_MATHML_ACTION_TYPE_TOOLTIP;

  return NS_MATHML_ACTION_TYPE_UNKNOWN;
}

nsIFrame*
NS_NewMathMLmactionFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmactionFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmactionFrame)

nsMathMLmactionFrame::~nsMathMLmactionFrame()
{
  
  
  if (mListener) {
    mContent->RemoveSystemEventListener(NS_LITERAL_STRING("click"), mListener,
                                        false);
    mContent->RemoveSystemEventListener(NS_LITERAL_STRING("mouseover"), mListener,
                                        false);
    mContent->RemoveSystemEventListener(NS_LITERAL_STRING("mouseout"), mListener,
                                        false);
  }
}

NS_IMETHODIMP
nsMathMLmactionFrame::Init(nsIContent*      aContent,
                           nsIFrame*        aParent,
                           nsIFrame*        aPrevInFlow)
{
  

  mChildCount = -1; 
  mSelection = 0;
  mSelectedFrame = nullptr;
  mActionType = GetActionType(aContent);

  
  return nsMathMLContainerFrame::Init(aContent, aParent, aPrevInFlow);
}

NS_IMETHODIMP
nsMathMLmactionFrame::TransmitAutomaticData() {
  
  
  
  nsIMathMLFrame* mathMLFrame = do_QueryFrame(mSelectedFrame);
  if (mathMLFrame && mathMLFrame->IsSpaceLike()) {
    mPresentationData.flags |= NS_MATHML_SPACE_LIKE;
  } else {
    mPresentationData.flags &= ~NS_MATHML_SPACE_LIKE;
  }

  
  
  
  mPresentationData.baseFrame = mSelectedFrame;
  GetEmbellishDataFrom(mSelectedFrame, mEmbellishData);

  return NS_OK;
}

nsresult
nsMathMLmactionFrame::ChildListChanged(int32_t aModType)
{
  
  mChildCount = -1;
  mSelection = 0;
  mSelectedFrame = nullptr;
  GetSelectedFrame();

  return nsMathMLContainerFrame::ChildListChanged(aModType);
}


nsIFrame* 
nsMathMLmactionFrame::GetSelectedFrame()
{
  nsAutoString value;
  int32_t selection; 

  if ((mActionType & NS_MATHML_ACTION_TYPE_CLASS_BITMASK) == 
       NS_MATHML_ACTION_TYPE_CLASS_ERROR) {
    
    mSelection = -1;
    mSelectedFrame = nullptr;
    return mSelectedFrame;
  }

  
  
  if ((mActionType & NS_MATHML_ACTION_TYPE_CLASS_BITMASK) == 
       NS_MATHML_ACTION_TYPE_CLASS_IGNORE_SELECTION) {
    
    
    
    mSelection = 1;
    mSelectedFrame = mFrames.FirstChild();
    return mSelectedFrame;
  }

  GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::selection_,
               value);
  if (!value.IsEmpty()) {
    nsresult errorCode;
    selection = value.ToInteger(&errorCode);
    if (NS_FAILED(errorCode)) 
      selection = 1;
  }
  else selection = 1; 

  if (-1 != mChildCount) { 
    
    if (selection > mChildCount || selection < 1)
      selection = -1;
    
    if (selection == mSelection) 
      return mSelectedFrame;
  }

  
  int32_t count = 0;
  nsIFrame* childFrame = mFrames.FirstChild();
  while (childFrame) {
    if (!mSelectedFrame) 
      mSelectedFrame = childFrame; 
    if (++count == selection) 
      mSelectedFrame = childFrame;

    childFrame = childFrame->GetNextSibling();
  }
  
  if (selection > count || selection < 1)
    selection = -1;

  mChildCount = count;
  mSelection = selection;
  TransmitAutomaticData();

  return mSelectedFrame;
}

NS_IMETHODIMP
nsMathMLmactionFrame::SetInitialChildList(ChildListID     aListID,
                                          nsFrameList&    aChildList)
{
  nsresult rv = nsMathMLContainerFrame::SetInitialChildList(aListID, aChildList);

  
  
  if (!GetSelectedFrame()) {
    mActionType = NS_MATHML_ACTION_TYPE_NONE;
  }
  else {
    
    mListener = new nsMathMLmactionFrame::MouseListener(this);
    
    mContent->AddSystemEventListener(NS_LITERAL_STRING("click"), mListener,
                                     false, false);
    mContent->AddSystemEventListener(NS_LITERAL_STRING("mouseover"), mListener,
                                     false, false);
    mContent->AddSystemEventListener(NS_LITERAL_STRING("mouseout"), mListener,
                                     false, false);
  }
  return rv;
}

NS_IMETHODIMP
nsMathMLmactionFrame::AttributeChanged(int32_t  aNameSpaceID,
                                       nsIAtom* aAttribute,
                                       int32_t  aModType)
{
  bool needsReflow = false;

  if (aAttribute == nsGkAtoms::actiontype_) {
    
    int32_t oldActionType = mActionType;
    mActionType = GetActionType(mContent);

    
    if ((oldActionType & NS_MATHML_ACTION_TYPE_CLASS_BITMASK) !=
          (mActionType & NS_MATHML_ACTION_TYPE_CLASS_BITMASK)) {
      needsReflow = true;
    }
  } else if (aAttribute == nsGkAtoms::selection_) {
    if ((mActionType & NS_MATHML_ACTION_TYPE_CLASS_BITMASK) == 
         NS_MATHML_ACTION_TYPE_CLASS_USE_SELECTION) {
      needsReflow = true;
    }
  } else {
    
    return 
      nsMathMLContainerFrame::AttributeChanged(aNameSpaceID, 
                                               aAttribute, aModType);
  }

  if (needsReflow) {
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange, NS_FRAME_IS_DIRTY);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsMathMLmactionFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                       const nsRect&           aDirtyRect,
                                       const nsDisplayListSet& aLists)
{
  
  
  
  if (NS_MATHML_HAS_ERROR(mPresentationData.flags)) {
    return nsMathMLContainerFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
  }

  nsresult rv = DisplayBorderBackgroundOutline(aBuilder, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIFrame* childFrame = GetSelectedFrame();
  if (childFrame) {
    
    nsDisplayListSet set(aLists, aLists.Content());
    
    rv = BuildDisplayListForChild(aBuilder, childFrame, aDirtyRect, set);
    NS_ENSURE_SUCCESS(rv, rv);
  }

#if defined(DEBUG) && defined(SHOW_BOUNDING_BOX)
  
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
  mBoundingMetrics = nsBoundingMetrics();
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
nsMathMLmactionFrame::Place(nsRenderingContext& aRenderingContext,
                            bool                 aPlaceOrigin,
                            nsHTMLReflowMetrics& aDesiredSize)
{
  nsIFrame* childFrame = GetSelectedFrame();

  if (mSelection == -1) {
    return ReflowError(aRenderingContext, aDesiredSize);
  }

  aDesiredSize.width = aDesiredSize.height = 0;
  aDesiredSize.ascent = 0;
  mBoundingMetrics = nsBoundingMetrics();
  if (childFrame) {
    GetReflowAndBoundingMetricsFor(childFrame, aDesiredSize, mBoundingMetrics);
    if (aPlaceOrigin) {
      FinishReflowChild(childFrame, PresContext(), nullptr, aDesiredSize, 0, 0, 0);
    }
    mReference.x = 0;
    mReference.y = aDesiredSize.ascent;
  }
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;
  return NS_OK;
}





NS_IMPL_ISUPPORTS1(nsMathMLmactionFrame::MouseListener,
                   nsIDOMEventListener)




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
nsMathMLmactionFrame::MouseListener::HandleEvent(nsIDOMEvent* aEvent)
{
  nsAutoString eventType;
  aEvent->GetType(eventType);
  if (eventType.EqualsLiteral("mouseover")) {
    mOwner->MouseOver();
  }
  else if (eventType.EqualsLiteral("click")) {
    mOwner->MouseClick();
  }
  else if (eventType.EqualsLiteral("mouseout")) {
    mOwner->MouseOut();
  }
  else {
    NS_ABORT();
  }

  return NS_OK;
}

void
nsMathMLmactionFrame::MouseOver()
{
  
  if (NS_MATHML_ACTION_TYPE_STATUSLINE == mActionType) {
    
    nsIFrame* childFrame = mFrames.FrameAt(1);
    if (!childFrame) return;

    nsIContent* content = childFrame->GetContent();
    if (!content) return;

    
    if (content->GetNameSpaceID() == kNameSpaceID_MathML &&
        content->Tag() == nsGkAtoms::mtext_) {
      
      content = content->GetFirstChild();
      if (!content) return;

      const nsTextFragment* textFrg = content->GetText();
      if (!textFrg) return;

      nsAutoString text;
      textFrg->AppendTo(text);
      
      text.CompressWhitespace();
      ShowStatus(PresContext(), text);
    }
  }
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

void
nsMathMLmactionFrame::MouseClick()
{
  if (NS_MATHML_ACTION_TYPE_TOGGLE == mActionType) {
    if (mChildCount > 1) {
      int32_t selection = (mSelection == mChildCount)? 1 : mSelection + 1;
      nsAutoString value;
      char cbuf[10];
      PR_snprintf(cbuf, sizeof(cbuf), "%d", selection);
      value.AssignASCII(cbuf);
      bool notify = false; 
      mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::selection_, value, notify);

      
      PresContext()->PresShell()->
        FrameNeedsReflow(mSelectedFrame, nsIPresShell::eTreeChange,
                         NS_FRAME_IS_DIRTY);
    }
  }
}

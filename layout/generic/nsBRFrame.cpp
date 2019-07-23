






































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsHTMLParts.h"
#include "nsPresContext.h"
#include "nsLineLayout.h"
#include "nsStyleConsts.h"
#include "nsGkAtoms.h"
#include "nsIFontMetrics.h"
#include "nsIRenderingContext.h"
#include "nsLayoutUtils.h"

#ifdef ACCESSIBILITY
#include "nsIServiceManager.h"
#include "nsIAccessible.h"
#include "nsIAccessibilityService.h"
#endif


#include "nsIContent.h"
#include "nsFrameSelection.h"


class BRFrame : public nsFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewBRFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  virtual ContentOffsets CalcContentOffsetsFromFramePoint(nsPoint aPoint);

  virtual PRBool PeekOffsetNoAmount(PRBool aForward, PRInt32* aOffset);
  virtual PRBool PeekOffsetCharacter(PRBool aForward, PRInt32* aOffset);
  virtual PRBool PeekOffsetWord(PRBool aForward, PRBool aWordSelectEatSpace, PRBool aIsKeyboardSelect,
                                PRInt32* aOffset, PeekWordState* aState);

  NS_IMETHOD Reflow(nsPresContext* aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus);
  virtual void AddInlineMinWidth(nsIRenderingContext *aRenderingContext,
                                 InlineMinWidthData *aData);
  virtual void AddInlinePrefWidth(nsIRenderingContext *aRenderingContext,
                                  InlinePrefWidthData *aData);
  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);
  virtual nsIAtom* GetType() const;

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsFrame::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced |
                                             nsIFrame::eLineParticipant));
  }

#ifdef ACCESSIBILITY  
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
#endif

protected:
  BRFrame(nsStyleContext* aContext) : nsFrame(aContext) {}
  virtual ~BRFrame();
};

nsIFrame*
NS_NewBRFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) BRFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(BRFrame)

BRFrame::~BRFrame()
{
}

NS_IMETHODIMP
BRFrame::Reflow(nsPresContext* aPresContext,
                nsHTMLReflowMetrics& aMetrics,
                const nsHTMLReflowState& aReflowState,
                nsReflowStatus& aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("BRFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aMetrics, aStatus);
  aMetrics.height = 0; 
                       
                       
  aMetrics.width = 0;
  aMetrics.ascent = 0;

  
  
  nsLineLayout* ll = aReflowState.mLineLayout;
  if (ll) {
    
    
    if ( ll->LineIsEmpty() ||
         aPresContext->CompatibilityMode() == eCompatibility_FullStandards ) {
      
      
      
      
      
      
      
      
      
      

      
      
      
      nsLayoutUtils::SetFontFromStyle(aReflowState.rendContext, mStyleContext);
      nsCOMPtr<nsIFontMetrics> fm;
      aReflowState.rendContext->GetFontMetrics(*getter_AddRefs(fm));
      if (fm) {
        nscoord logicalHeight = aReflowState.CalcLineHeight();
        aMetrics.height = logicalHeight;
        aMetrics.ascent =
          nsLayoutUtils::GetCenteredFontBaseline(fm, logicalHeight);
      }
      else {
        aMetrics.ascent = aMetrics.height = 0;
      }

      
      
      
      
      
      
      aMetrics.width = 1;
    }

    
    PRUint32 breakType = aReflowState.mStyleDisplay->mBreakType;
    if (NS_STYLE_CLEAR_NONE == breakType) {
      breakType = NS_STYLE_CLEAR_LINE;
    }

    aStatus = NS_INLINE_BREAK | NS_INLINE_BREAK_AFTER |
      NS_INLINE_MAKE_BREAK_TYPE(breakType);
    ll->SetLineEndsInBR(PR_TRUE);
  }
  else {
    aStatus = NS_FRAME_COMPLETE;
  }
  
  aMetrics.mOverflowArea = nsRect(0, 0, aMetrics.width, aMetrics.height);

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);
  return NS_OK;
}

 void
BRFrame::AddInlineMinWidth(nsIRenderingContext *aRenderingContext,
                           nsIFrame::InlineMinWidthData *aData)
{
  aData->ForceBreak(aRenderingContext);
}

 void
BRFrame::AddInlinePrefWidth(nsIRenderingContext *aRenderingContext,
                            nsIFrame::InlinePrefWidthData *aData)
{
  aData->ForceBreak(aRenderingContext);
}

 nscoord
BRFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result = 0;
  DISPLAY_MIN_WIDTH(this, result);
  return result;
}

 nscoord
BRFrame::GetPrefWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result = 0;
  DISPLAY_PREF_WIDTH(this, result);
  return result;
}

nsIAtom*
BRFrame::GetType() const
{
  return nsGkAtoms::brFrame;
}

nsIFrame::ContentOffsets BRFrame::CalcContentOffsetsFromFramePoint(nsPoint aPoint)
{
  ContentOffsets offsets;
  offsets.content = mContent->GetParent();
  if (offsets.content) {
    offsets.offset = offsets.content->IndexOf(mContent);
    offsets.secondaryOffset = offsets.offset;
    offsets.associateWithNext = PR_TRUE;
  }
  return offsets;
}

PRBool
BRFrame::PeekOffsetNoAmount(PRBool aForward, PRInt32* aOffset)
{
  NS_ASSERTION (aOffset && *aOffset <= 1, "aOffset out of range");
  PRInt32 startOffset = *aOffset;
  
  if (!aForward && startOffset != 0) {
    *aOffset = 0;
    return PR_TRUE;
  }
  
  return (startOffset == 0);
}

PRBool
BRFrame::PeekOffsetCharacter(PRBool aForward, PRInt32* aOffset)
{
  NS_ASSERTION (aOffset && *aOffset <= 1, "aOffset out of range");
  
  return PR_FALSE;
}

PRBool
BRFrame::PeekOffsetWord(PRBool aForward, PRBool aWordSelectEatSpace, PRBool aIsKeyboardSelect,
                        PRInt32* aOffset, PeekWordState* aState)
{
  NS_ASSERTION (aOffset && *aOffset <= 1, "aOffset out of range");
  
  return PR_FALSE;
}

#ifdef ACCESSIBILITY
NS_IMETHODIMP BRFrame::GetAccessible(nsIAccessible** aAccessible)
{
  NS_ENSURE_TRUE(mContent, NS_ERROR_FAILURE);
  nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");
  NS_ENSURE_TRUE(accService, NS_ERROR_FAILURE);
  nsIContent *parent = mContent->GetParent();
  if (parent &&
      parent->IsRootOfNativeAnonymousSubtree() &&
      parent->GetChildCount() == 1) {
    
    
    return NS_ERROR_FAILURE;
  }
  return accService->CreateHTMLBRAccessible(this, aAccessible);
}
#endif


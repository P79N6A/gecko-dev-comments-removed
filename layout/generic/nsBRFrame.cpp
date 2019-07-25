






































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsHTMLParts.h"
#include "nsPresContext.h"
#include "nsLineLayout.h"
#include "nsStyleConsts.h"
#include "nsGkAtoms.h"
#include "nsRenderingContext.h"
#include "nsLayoutUtils.h"

#ifdef ACCESSIBILITY
#include "nsIServiceManager.h"
#include "nsAccessibilityService.h"
#endif


#include "nsIContent.h"
#include "nsFrameSelection.h"


#define BR_USING_CENTERED_FONT_BASELINE NS_FRAME_STATE_BIT(63)

class BRFrame : public nsFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewBRFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  virtual ContentOffsets CalcContentOffsetsFromFramePoint(nsPoint aPoint);

  virtual PRBool PeekOffsetNoAmount(PRBool aForward, PRInt32* aOffset);
  virtual PRBool PeekOffsetCharacter(PRBool aForward, PRInt32* aOffset,
                                     PRBool aRespectClusters = PR_TRUE);
  virtual PRBool PeekOffsetWord(PRBool aForward, PRBool aWordSelectEatSpace, PRBool aIsKeyboardSelect,
                                PRInt32* aOffset, PeekWordState* aState);

  NS_IMETHOD Reflow(nsPresContext* aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus);
  virtual void AddInlineMinWidth(nsRenderingContext *aRenderingContext,
                                 InlineMinWidthData *aData);
  virtual void AddInlinePrefWidth(nsRenderingContext *aRenderingContext,
                                  InlinePrefWidthData *aData);
  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);
  virtual nsIAtom* GetType() const;
  virtual nscoord GetBaseline() const;

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsFrame::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced |
                                             nsIFrame::eLineParticipant));
  }

#ifdef ACCESSIBILITY
  virtual already_AddRefed<nsAccessible> CreateAccessible();
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
  RemoveStateBits(BR_USING_CENTERED_FONT_BASELINE);

  
  
  nsLineLayout* ll = aReflowState.mLineLayout;
  if (ll) {
    
    
    if ( ll->LineIsEmpty() ||
         aPresContext->CompatibilityMode() == eCompatibility_FullStandards ) {
      
      
      
      
      
      
      
      
      
      

      
      
      
      nsLayoutUtils::SetFontFromStyle(aReflowState.rendContext, mStyleContext);
      nsFontMetrics *fm = aReflowState.rendContext->FontMetrics();
      if (fm) {
        nscoord logicalHeight = aReflowState.CalcLineHeight();
        aMetrics.height = logicalHeight;
        aMetrics.ascent =
          nsLayoutUtils::GetCenteredFontBaseline(fm, logicalHeight);
        AddStateBits(BR_USING_CENTERED_FONT_BASELINE);
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

  aMetrics.SetOverflowAreasToDesiredBounds();

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);
  return NS_OK;
}

 void
BRFrame::AddInlineMinWidth(nsRenderingContext *aRenderingContext,
                           nsIFrame::InlineMinWidthData *aData)
{
  aData->ForceBreak(aRenderingContext);
}

 void
BRFrame::AddInlinePrefWidth(nsRenderingContext *aRenderingContext,
                            nsIFrame::InlinePrefWidthData *aData)
{
  aData->ForceBreak(aRenderingContext);
}

 nscoord
BRFrame::GetMinWidth(nsRenderingContext *aRenderingContext)
{
  nscoord result = 0;
  DISPLAY_MIN_WIDTH(this, result);
  return result;
}

 nscoord
BRFrame::GetPrefWidth(nsRenderingContext *aRenderingContext)
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

nscoord
BRFrame::GetBaseline() const
{
  nscoord ascent = 0;
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm));
  if (fm) {
    nscoord logicalHeight = GetRect().height;
    if (GetStateBits() & BR_USING_CENTERED_FONT_BASELINE) {
      ascent = nsLayoutUtils::GetCenteredFontBaseline(fm, logicalHeight);
    } else {
      ascent = fm->MaxAscent() + GetUsedBorderAndPadding().top;
    }
  }
  return NS_MIN(mRect.height, ascent);
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
BRFrame::PeekOffsetCharacter(PRBool aForward, PRInt32* aOffset,
                             PRBool aRespectClusters)
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
already_AddRefed<nsAccessible>
BRFrame::CreateAccessible()
{
  nsAccessibilityService* accService = nsIPresShell::AccService();
  if (!accService) {
    return nsnull;
  }
  nsIContent *parent = mContent->GetParent();
  if (parent &&
      parent->IsRootOfNativeAnonymousSubtree() &&
      parent->GetChildCount() == 1) {
    
    
    return nsnull;
  }
  return accService->CreateHTMLBRAccessible(mContent,
                                            PresContext()->PresShell());
}
#endif


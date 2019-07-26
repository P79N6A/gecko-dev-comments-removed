






#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsLineLayout.h"
#include "nsStyleConsts.h"
#include "nsGkAtoms.h"
#include "nsRenderingContext.h"
#include "nsLayoutUtils.h"


#include "nsIContent.h"


using namespace mozilla;

class BRFrame : public nsFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewBRFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  virtual ContentOffsets CalcContentOffsetsFromFramePoint(nsPoint aPoint) MOZ_OVERRIDE;

  virtual FrameSearchResult PeekOffsetNoAmount(bool aForward, int32_t* aOffset) MOZ_OVERRIDE;
  virtual FrameSearchResult PeekOffsetCharacter(bool aForward, int32_t* aOffset,
                                     bool aRespectClusters = true) MOZ_OVERRIDE;
  virtual FrameSearchResult PeekOffsetWord(bool aForward, bool aWordSelectEatSpace,
                              bool aIsKeyboardSelect, int32_t* aOffset,
                              PeekWordState* aState) MOZ_OVERRIDE;

  virtual void Reflow(nsPresContext* aPresContext,
                          nsHTMLReflowMetrics& aDesiredSize,
                          const nsHTMLReflowState& aReflowState,
                          nsReflowStatus& aStatus) MOZ_OVERRIDE;
  virtual void AddInlineMinWidth(nsRenderingContext *aRenderingContext,
                                 InlineMinWidthData *aData) MOZ_OVERRIDE;
  virtual void AddInlinePrefWidth(nsRenderingContext *aRenderingContext,
                                  InlinePrefWidthData *aData) MOZ_OVERRIDE;
  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nsIAtom* GetType() const MOZ_OVERRIDE;
  virtual nscoord GetBaseline() const MOZ_OVERRIDE;

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    return nsFrame::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced |
                                             nsIFrame::eLineParticipant));
  }

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

protected:
  BRFrame(nsStyleContext* aContext) : nsFrame(aContext) {}
  virtual ~BRFrame();

  nscoord mAscent;
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

void
BRFrame::Reflow(nsPresContext* aPresContext,
                nsHTMLReflowMetrics& aMetrics,
                const nsHTMLReflowState& aReflowState,
                nsReflowStatus& aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("BRFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aMetrics, aStatus);
  aMetrics.Height() = 0; 
                       
                       
  aMetrics.Width() = 0;
  aMetrics.SetTopAscent(0);

  
  
  nsLineLayout* ll = aReflowState.mLineLayout;
  if (ll) {
    
    
    if ( ll->LineIsEmpty() ||
         aPresContext->CompatibilityMode() == eCompatibility_FullStandards ) {
      
      
      
      
      
      
      
      
      
      

      
      
      
      nsRefPtr<nsFontMetrics> fm;
      nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm),
        nsLayoutUtils::FontSizeInflationFor(this));
      aReflowState.rendContext->SetFont(fm); 
      if (fm) {
        nscoord logicalHeight = aReflowState.CalcLineHeight();
        aMetrics.Height() = logicalHeight;
        aMetrics.SetTopAscent(nsLayoutUtils::GetCenteredFontBaseline(fm, logicalHeight));
      }
      else {
        aMetrics.SetTopAscent(aMetrics.Height() = 0);
      }

      
      
      
      
      
      
      aMetrics.Width() = 1;
    }

    
    uint32_t breakType = aReflowState.mStyleDisplay->mBreakType;
    if (NS_STYLE_CLEAR_NONE == breakType) {
      breakType = NS_STYLE_CLEAR_LINE;
    }

    aStatus = NS_INLINE_BREAK | NS_INLINE_BREAK_AFTER |
      NS_INLINE_MAKE_BREAK_TYPE(breakType);
    ll->SetLineEndsInBR(true);
  }
  else {
    aStatus = NS_FRAME_COMPLETE;
  }

  aMetrics.SetOverflowAreasToDesiredBounds();

  mAscent = aMetrics.TopAscent();

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);
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
  return mAscent;
}

nsIFrame::ContentOffsets BRFrame::CalcContentOffsetsFromFramePoint(nsPoint aPoint)
{
  ContentOffsets offsets;
  offsets.content = mContent->GetParent();
  if (offsets.content) {
    offsets.offset = offsets.content->IndexOf(mContent);
    offsets.secondaryOffset = offsets.offset;
    offsets.associateWithNext = true;
  }
  return offsets;
}

nsIFrame::FrameSearchResult
BRFrame::PeekOffsetNoAmount(bool aForward, int32_t* aOffset)
{
  NS_ASSERTION (aOffset && *aOffset <= 1, "aOffset out of range");
  int32_t startOffset = *aOffset;
  
  if (!aForward && startOffset != 0) {
    *aOffset = 0;
    return FOUND;
  }
  
  return (startOffset == 0) ? FOUND : CONTINUE;
}

nsIFrame::FrameSearchResult
BRFrame::PeekOffsetCharacter(bool aForward, int32_t* aOffset,
                             bool aRespectClusters)
{
  NS_ASSERTION (aOffset && *aOffset <= 1, "aOffset out of range");
  
  return CONTINUE;
}

nsIFrame::FrameSearchResult
BRFrame::PeekOffsetWord(bool aForward, bool aWordSelectEatSpace, bool aIsKeyboardSelect,
                        int32_t* aOffset, PeekWordState* aState)
{
  NS_ASSERTION (aOffset && *aOffset <= 1, "aOffset out of range");
  
  return CONTINUE;
}

#ifdef ACCESSIBILITY
a11y::AccType
BRFrame::AccessibleType()
{
  nsIContent *parent = mContent->GetParent();
  if (parent && parent->IsRootOfNativeAnonymousSubtree() &&
      parent->GetChildCount() == 1) {
    
    
    return a11y::eNoType;
  }

  return a11y::eHTMLBRType;
}
#endif


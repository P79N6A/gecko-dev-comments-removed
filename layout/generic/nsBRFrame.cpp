






#include "nsCOMPtr.h"
#include "nsFontMetrics.h"
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

  virtual ContentOffsets CalcContentOffsetsFromFramePoint(nsPoint aPoint) override;

  virtual FrameSearchResult PeekOffsetNoAmount(bool aForward, int32_t* aOffset) override;
  virtual FrameSearchResult PeekOffsetCharacter(bool aForward, int32_t* aOffset,
                                     bool aRespectClusters = true) override;
  virtual FrameSearchResult PeekOffsetWord(bool aForward, bool aWordSelectEatSpace,
                              bool aIsKeyboardSelect, int32_t* aOffset,
                              PeekWordState* aState) override;

  virtual void Reflow(nsPresContext* aPresContext,
                          nsHTMLReflowMetrics& aDesiredSize,
                          const nsHTMLReflowState& aReflowState,
                          nsReflowStatus& aStatus) override;
  virtual void AddInlineMinISize(nsRenderingContext *aRenderingContext,
                                 InlineMinISizeData *aData) override;
  virtual void AddInlinePrefISize(nsRenderingContext *aRenderingContext,
                                  InlinePrefISizeData *aData) override;
  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) override;
  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) override;
  virtual nsIAtom* GetType() const override;
  virtual nscoord GetLogicalBaseline(mozilla::WritingMode aWritingMode) const override;

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return nsFrame::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced |
                                             nsIFrame::eLineParticipant));
  }

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() override;
#endif

protected:
  explicit BRFrame(nsStyleContext* aContext) : nsFrame(aContext) {}
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
  MarkInReflow();
  DO_GLOBAL_REFLOW_COUNT("BRFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aMetrics, aStatus);
  WritingMode wm = aReflowState.GetWritingMode();
  LogicalSize finalSize(wm);
  finalSize.BSize(wm) = 0; 
                           
                           
  finalSize.ISize(wm) = 0;
  aMetrics.SetBlockStartAscent(0);

  
  
  
  
  
  
  nsLineLayout* ll = aReflowState.mLineLayout;
  if (ll && !GetParent()->StyleContext()->ShouldSuppressLineBreak()) {
    
    
    if ( ll->LineIsEmpty() ||
         aPresContext->CompatibilityMode() == eCompatibility_FullStandards ) {
      
      
      
      
      
      
      
      
      
      

      
      
      
      nsRefPtr<nsFontMetrics> fm;
      nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm),
        nsLayoutUtils::FontSizeInflationFor(this));
      if (fm) {
        nscoord logicalHeight = aReflowState.CalcLineHeight();
        finalSize.BSize(wm) = logicalHeight;
        aMetrics.SetBlockStartAscent(nsLayoutUtils::GetCenteredFontBaseline(
                                       fm, logicalHeight, wm.IsLineInverted()));
      }
      else {
        aMetrics.SetBlockStartAscent(aMetrics.BSize(wm) = 0);
      }

      
      
      
      
      
      
      finalSize.ISize(wm) = 1;
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

  aMetrics.SetSize(wm, finalSize);
  aMetrics.SetOverflowAreasToDesiredBounds();

  mAscent = aMetrics.BlockStartAscent();

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);
}

 void
BRFrame::AddInlineMinISize(nsRenderingContext *aRenderingContext,
                           nsIFrame::InlineMinISizeData *aData)
{
  if (!GetParent()->StyleContext()->ShouldSuppressLineBreak()) {
    aData->ForceBreak(aRenderingContext);
  }
}

 void
BRFrame::AddInlinePrefISize(nsRenderingContext *aRenderingContext,
                            nsIFrame::InlinePrefISizeData *aData)
{
  if (!GetParent()->StyleContext()->ShouldSuppressLineBreak()) {
    aData->ForceBreak(aRenderingContext);
  }
}

 nscoord
BRFrame::GetMinISize(nsRenderingContext *aRenderingContext)
{
  nscoord result = 0;
  DISPLAY_MIN_WIDTH(this, result);
  return result;
}

 nscoord
BRFrame::GetPrefISize(nsRenderingContext *aRenderingContext)
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
BRFrame::GetLogicalBaseline(mozilla::WritingMode aWritingMode) const
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
    offsets.associate = CARET_ASSOCIATE_AFTER;
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


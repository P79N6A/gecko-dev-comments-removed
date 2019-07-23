






































#include "nsHTMLParts.h"
#include "nsFrame.h"
#include "nsLineLayout.h"
#include "nsPresContext.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsINameSpaceManager.h"


#define TYPE_WORD  0            // horizontal space
#define TYPE_LINE  1            // line-break + vertical space
#define TYPE_IMAGE 2            // acts like a sized image with nothing to see

class SpacerFrame : public nsFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewSpacerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  
  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);
  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  PRUint8 GetSpacerType();

protected:
  SpacerFrame(nsStyleContext* aContext) : nsFrame(aContext) {}
  virtual ~SpacerFrame();
  void GetDesiredSize(nsHTMLReflowMetrics& aMetrics, nsSize aPercentBase);
};

nsIFrame*
NS_NewSpacerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
#ifdef DEBUG
  const nsStyleDisplay* disp = aContext->GetStyleDisplay();
  NS_ASSERTION(!disp->IsAbsolutelyPositioned() && !disp->IsFloating(),
               "Spacers should not be positioned and should not float");
#endif

  return new (aPresShell) SpacerFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(SpacerFrame)

SpacerFrame::~SpacerFrame()
{
}

 nscoord
SpacerFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
{
  nsHTMLReflowMetrics metrics;
  DISPLAY_MIN_WIDTH(this, metrics.width);
  GetDesiredSize(metrics, nsSize(0, 0));
  return metrics.width;
}

 nscoord
SpacerFrame::GetPrefWidth(nsIRenderingContext *aRenderingContext)
{
  nsHTMLReflowMetrics metrics;
  DISPLAY_PREF_WIDTH(this, metrics.width);
  GetDesiredSize(metrics, nsSize(0, 0));
  return metrics.width;
}

NS_IMETHODIMP
SpacerFrame::Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aMetrics,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("SpacerFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aMetrics, aStatus);
  aStatus = NS_FRAME_COMPLETE;

  
  nsSize percentBase(aReflowState.availableWidth, aReflowState.availableHeight);
  if (percentBase.width == NS_UNCONSTRAINEDSIZE)
    percentBase.width = 0;
  if (percentBase.height == NS_UNCONSTRAINEDSIZE)
    percentBase.height = 0;

  if (GetSpacerType() == TYPE_LINE)
    aStatus = NS_INLINE_LINE_BREAK_AFTER(NS_FRAME_COMPLETE);

  GetDesiredSize(aMetrics, percentBase);

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);
  return NS_OK;
}

void
SpacerFrame::GetDesiredSize(nsHTMLReflowMetrics& aMetrics, nsSize aPercentBase)
{
  
  aMetrics.width = 0;
  aMetrics.height = 0;

  
  
  
  
  

  const nsStylePosition* position = GetStylePosition();

  PRUint8 type = GetSpacerType();
  switch (type) {
  case TYPE_WORD:
    break;

  case TYPE_LINE:
    if (eStyleUnit_Coord == position->mHeight.GetUnit()) {
      aMetrics.height = position->mHeight.GetCoordValue();
    }
    break;

  case TYPE_IMAGE:
    
    nsStyleUnit unit = position->mWidth.GetUnit();
    if (eStyleUnit_Coord == unit) {
      aMetrics.width = position->mWidth.GetCoordValue();
    }
    else if (eStyleUnit_Percent == unit) 
    {
      float factor = position->mWidth.GetPercentValue();
      aMetrics.width = NSToCoordRound(factor * aPercentBase.width);
    }

    
    unit = position->mHeight.GetUnit();
    if (eStyleUnit_Coord == unit) {
      aMetrics.height = position->mHeight.GetCoordValue();
    }
    else if (eStyleUnit_Percent == unit) 
    {
      float factor = position->mHeight.GetPercentValue();
      aMetrics.height = NSToCoordRound(factor * aPercentBase.height);
    }
    break;
  }

  if (aMetrics.width || aMetrics.height) {
    
    if (!aMetrics.width) aMetrics.width = 1;
    if (!aMetrics.height) aMetrics.height = 1;
  }
}

PRUint8
SpacerFrame::GetSpacerType()
{
  PRUint8 type = TYPE_WORD;
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::line, &nsGkAtoms::vert, &nsGkAtoms::vertical,
     &nsGkAtoms::block, nsnull};
  switch (mContent->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::type,
                                    strings, eIgnoreCase)) {
    case 0:
    case 1:
    case 2:
      return TYPE_LINE;
    case 3:
      return TYPE_IMAGE;
  }
  return type;
}

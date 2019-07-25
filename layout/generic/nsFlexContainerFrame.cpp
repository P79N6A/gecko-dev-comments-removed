








#include "nsFlexContainerFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "prlog.h"

#ifdef PR_LOGGING 
static PRLogModuleInfo* nsFlexContainerFrameLM = PR_NewLogModule("nsFlexContainerFrame");
#endif 

NS_IMPL_FRAMEARENA_HELPERS(nsFlexContainerFrame)

nsIFrame*
NS_NewFlexContainerFrame(nsIPresShell* aPresShell,
                         nsStyleContext* aContext)
{
  return new (aPresShell) nsFlexContainerFrame(aContext);
}




nsFlexContainerFrame::~nsFlexContainerFrame()
{
}

NS_QUERYFRAME_HEAD(nsFlexContainerFrame)
  NS_QUERYFRAME_ENTRY(nsFlexContainerFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsFlexContainerFrameSuper)

void
nsFlexContainerFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  DestroyAbsoluteFrames(aDestructRoot);
  nsFlexContainerFrameSuper::DestroyFrom(aDestructRoot);
}

nsIAtom*
nsFlexContainerFrame::GetType() const
{
  return nsGkAtoms::flexContainerFrame;
}

#ifdef DEBUG
NS_IMETHODIMP
nsFlexContainerFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("FlexContainer"), aResult);
}
#endif 

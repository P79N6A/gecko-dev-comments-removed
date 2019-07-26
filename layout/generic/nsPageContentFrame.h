



#ifndef nsPageContentFrame_h___
#define nsPageContentFrame_h___

#include "mozilla/Attributes.h"
#include "nsViewportFrame.h"
class nsPageFrame;
class nsSharedPageData;


class nsPageContentFrame : public ViewportFrame {

public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewPageContentFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
  friend class nsPageFrame;

  
  NS_IMETHOD  Reflow(nsPresContext*      aPresContext,
                     nsHTMLReflowMetrics& aDesiredSize,
                     const nsHTMLReflowState& aMaxSize,
                     nsReflowStatus&      aStatus) MOZ_OVERRIDE;

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    return ViewportFrame::IsFrameOfType(aFlags &
             ~(nsIFrame::eCanContainOverflowContainers));
  }

  virtual void SetSharedPageData(nsSharedPageData* aPD) { mPD = aPD; }

  virtual bool HasTransformGetter() const MOZ_OVERRIDE { return true; }

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;
  
#ifdef DEBUG_FRAME_DUMP
  
  NS_IMETHOD  GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

protected:
  nsPageContentFrame(nsStyleContext* aContext) : ViewportFrame(aContext) {}

  nsSharedPageData*         mPD;
};

#endif 


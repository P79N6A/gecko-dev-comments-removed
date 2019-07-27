



#ifndef nsPageContentFrame_h___
#define nsPageContentFrame_h___

#include "mozilla/Attributes.h"
#include "nsViewportFrame.h"
class nsPageFrame;
class nsSharedPageData;


class nsPageContentFrame : public ViewportFrame {

public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsPageContentFrame* NS_NewPageContentFrame(nsIPresShell* aPresShell,
                                                    nsStyleContext* aContext);
  friend class nsPageFrame;

  
  virtual void Reflow(nsPresContext*      aPresContext,
                      nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aMaxSize,
                      nsReflowStatus&      aStatus) override;

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return ViewportFrame::IsFrameOfType(aFlags &
             ~(nsIFrame::eCanContainOverflowContainers));
  }

  virtual void SetSharedPageData(nsSharedPageData* aPD) { mPD = aPD; }

  virtual bool HasTransformGetter() const override { return true; }

  




  virtual nsIAtom* GetType() const override;
  
#ifdef DEBUG_FRAME_DUMP
  
  virtual nsresult  GetFrameName(nsAString& aResult) const override;
#endif

protected:
  explicit nsPageContentFrame(nsStyleContext* aContext) : ViewportFrame(aContext) {}

  nsSharedPageData*         mPD;
};

#endif 


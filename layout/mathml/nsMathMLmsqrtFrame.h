




#ifndef nsMathMLmsqrtFrame_h___
#define nsMathMLmsqrtFrame_h___

#include "mozilla/Attributes.h"
#include "nsMathMLmencloseFrame.h"
























class nsMathMLmsqrtFrame : public nsMathMLmencloseFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmsqrtFrame(nsIPresShell*   aPresShell,
                                          nsStyleContext* aContext);

  virtual void
  Init(nsIContent*       aContent,
       nsContainerFrame* aParent,
       nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;

  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent) MOZ_OVERRIDE;

  virtual nsresult
  AttributeChanged(int32_t         aNameSpaceID,
                   nsIAtom*        aAttribute,
                   int32_t         aModType) MOZ_OVERRIDE;

  virtual bool
  IsMrowLike() MOZ_OVERRIDE
  {
    return mFrames.FirstChild() != mFrames.LastChild() ||
           !mFrames.FirstChild();
  }

protected:
  explicit nsMathMLmsqrtFrame(nsStyleContext* aContext);
  virtual ~nsMathMLmsqrtFrame();
};

#endif 


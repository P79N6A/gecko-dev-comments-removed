




#ifndef nsMathMLmphantomFrame_h___
#define nsMathMLmphantomFrame_h___

#include "mozilla/Attributes.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmphantomFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmphantomFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent) MOZ_OVERRIDE;

  NS_IMETHOD
  TransmitAutomaticData() MOZ_OVERRIDE {
    return TransmitAutomaticDataForMrowLikeElement();
  }

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE {}

  bool
  IsMrowLike() MOZ_OVERRIDE {
    return mFrames.FirstChild() != mFrames.LastChild() ||
           !mFrames.FirstChild();
  }

protected:
  explicit nsMathMLmphantomFrame(nsStyleContext* aContext)
    : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmphantomFrame();
};

#endif 

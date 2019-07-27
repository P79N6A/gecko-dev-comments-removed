




#ifndef nsMathMLmrowFrame_h___
#define nsMathMLmrowFrame_h___

#include "mozilla/Attributes.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmrowFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmrowFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  virtual nsresult
  AttributeChanged(int32_t  aNameSpaceID,
                   nsIAtom* aAttribute,
                   int32_t  aModType) MOZ_OVERRIDE;

  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent) MOZ_OVERRIDE;

  NS_IMETHOD
  TransmitAutomaticData() MOZ_OVERRIDE {
    return TransmitAutomaticDataForMrowLikeElement();
  }

  virtual eMathMLFrameType
  GetMathMLFrameType() MOZ_OVERRIDE; 

  bool
  IsMrowLike() MOZ_OVERRIDE {
    
    
    
    return mFrames.FirstChild() != mFrames.LastChild() ||
           !mFrames.FirstChild();
  }

protected:
  explicit nsMathMLmrowFrame(nsStyleContext* aContext) : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmrowFrame();
};

#endif 

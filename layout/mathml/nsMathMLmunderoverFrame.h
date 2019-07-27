




#ifndef nsMathMLmunderoverFrame_h___
#define nsMathMLmunderoverFrame_h___

#include "mozilla/Attributes.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmunderoverFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmunderoverFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  virtual nsresult
  Place(nsRenderingContext& aRenderingContext,
        bool                 aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize) MOZ_OVERRIDE;

  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent) MOZ_OVERRIDE;

  NS_IMETHOD
  TransmitAutomaticData() MOZ_OVERRIDE;

  NS_IMETHOD
  UpdatePresentationData(uint32_t        aFlagsValues,
                         uint32_t        aFlagsToUpdate) MOZ_OVERRIDE;

  virtual nsresult
  AttributeChanged(int32_t         aNameSpaceID,
                   nsIAtom*        aAttribute,
                   int32_t         aModType) MOZ_OVERRIDE;

  uint8_t
  ScriptIncrement(nsIFrame* aFrame) MOZ_OVERRIDE;

protected:
  explicit nsMathMLmunderoverFrame(nsStyleContext* aContext) : nsMathMLContainerFrame(aContext),
                                                               mIncrementUnder(false),
                                                               mIncrementOver(false) {}
  virtual ~nsMathMLmunderoverFrame();

private:
  bool mIncrementUnder;
  bool mIncrementOver;
};


#endif 

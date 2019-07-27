




#ifndef nsMathMLmoFrame_h___
#define nsMathMLmoFrame_h___

#include "mozilla/Attributes.h"
#include "nsMathMLTokenFrame.h"
#include "nsMathMLChar.h"





class nsMathMLmoFrame : public nsMathMLTokenFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmoFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  virtual eMathMLFrameType GetMathMLFrameType() MOZ_OVERRIDE;

  virtual void
  SetAdditionalStyleContext(int32_t          aIndex, 
                            nsStyleContext*  aStyleContext) MOZ_OVERRIDE;
  virtual nsStyleContext*
  GetAdditionalStyleContext(int32_t aIndex) const MOZ_OVERRIDE;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent) MOZ_OVERRIDE;

  NS_IMETHOD
  TransmitAutomaticData() MOZ_OVERRIDE;

  virtual void
  SetInitialChildList(ChildListID     aListID,
                      nsFrameList&    aChildList) MOZ_OVERRIDE;

  virtual void
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  virtual void MarkIntrinsicISizesDirty() MOZ_OVERRIDE;

  virtual void
  GetIntrinsicISizeMetrics(nsRenderingContext* aRenderingContext,
                           nsHTMLReflowMetrics& aDesiredSize) MOZ_OVERRIDE;

  virtual nsresult
  AttributeChanged(int32_t         aNameSpaceID,
                   nsIAtom*        aAttribute,
                   int32_t         aModType) MOZ_OVERRIDE;

  
  
  NS_IMETHOD
  Stretch(nsRenderingContext& aRenderingContext,
          nsStretchDirection   aStretchDirection,
          nsBoundingMetrics&   aContainerSize,
          nsHTMLReflowMetrics& aDesiredStretchSize) MOZ_OVERRIDE;

  virtual nsresult
  ChildListChanged(int32_t aModType) MOZ_OVERRIDE
  {
    ProcessTextData();
    return nsMathMLContainerFrame::ChildListChanged(aModType);
  }

protected:
  explicit nsMathMLmoFrame(nsStyleContext* aContext) : nsMathMLTokenFrame(aContext) {}
  virtual ~nsMathMLmoFrame();
  
  nsMathMLChar     mMathMLChar; 
  nsOperatorFlags  mFlags;
  float            mMinSize;
  float            mMaxSize;

  bool UseMathMLChar();

  
  void ProcessTextData();

  
  
  
  void
  ProcessOperatorData();

  
  bool
  IsFrameInSelection(nsIFrame* aFrame);
};

#endif 

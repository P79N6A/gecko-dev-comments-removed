




#ifndef nsMathMLmfencedFrame_h
#define nsMathMLmfencedFrame_h

#include "mozilla/Attributes.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmfencedFrame MOZ_FINAL : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmfencedFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  virtual void
  SetAdditionalStyleContext(int32_t          aIndex, 
                            nsStyleContext*  aStyleContext) MOZ_OVERRIDE;
  virtual nsStyleContext*
  GetAdditionalStyleContext(int32_t aIndex) const MOZ_OVERRIDE;

  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent) MOZ_OVERRIDE;

  virtual void
  SetInitialChildList(ChildListID     aListID,
                      nsFrameList&    aChildList) MOZ_OVERRIDE;

  virtual void
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  virtual void
  GetIntrinsicISizeMetrics(nsRenderingContext* aRenderingContext,
                           nsHTMLReflowMetrics& aDesiredSize) MOZ_OVERRIDE;

  virtual nsresult
  AttributeChanged(int32_t         aNameSpaceID,
                   nsIAtom*        aAttribute,
                   int32_t         aModType) MOZ_OVERRIDE;

  
  virtual nsresult
  ChildListChanged(int32_t aModType) MOZ_OVERRIDE;

  
  virtual nscoord
  FixInterFrameSpacing(nsHTMLReflowMetrics& aDesiredSize) MOZ_OVERRIDE;

  
  static nsresult
  ReflowChar(nsPresContext*      aPresContext,
             nsRenderingContext& aRenderingContext,
             nsMathMLChar*        aMathMLChar,
             nsOperatorFlags      aForm,
             int32_t              aScriptLevel,
             nscoord              axisHeight,
             nscoord              leading,
             nscoord              em,
             nsBoundingMetrics&   aContainerSize,
             nscoord&             aAscent,
             nscoord&             aDescent,
             bool                 aRTL);

  static void
  PlaceChar(nsMathMLChar*      aMathMLChar,
            nscoord            aDesiredSize,
            nsBoundingMetrics& bm,
            nscoord&           dx);

  virtual bool
  IsMrowLike() MOZ_OVERRIDE
  {
    
    
    
    
    
    return true;
  }

protected:
  explicit nsMathMLmfencedFrame(nsStyleContext* aContext) : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmfencedFrame();
  
  nsMathMLChar* mOpenChar;
  nsMathMLChar* mCloseChar;
  nsMathMLChar* mSeparatorsChar;
  int32_t       mSeparatorsCount;

  
  void
  RemoveFencesAndSeparators();

  
  void
  CreateFencesAndSeparators(nsPresContext* aPresContext);
};

#endif 

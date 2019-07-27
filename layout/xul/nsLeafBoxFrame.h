



#ifndef nsLeafBoxFrame_h___
#define nsLeafBoxFrame_h___

#include "mozilla/Attributes.h"
#include "nsLeafFrame.h"
#include "nsBox.h"

class nsLeafBoxFrame : public nsLeafFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewLeafBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  virtual nsSize GetPrefSize(nsBoxLayoutState& aState) override;
  virtual nsSize GetMinSize(nsBoxLayoutState& aState) override;
  virtual nsSize GetMaxSize(nsBoxLayoutState& aState) override;
  virtual nscoord GetFlex(nsBoxLayoutState& aState) override;
  virtual nscoord GetBoxAscent(nsBoxLayoutState& aState) override;

  virtual nsIAtom* GetType() const override;
  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    
    
    return nsLeafFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock | nsIFrame::eXULBox));
  }

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
#endif

  

  virtual void MarkIntrinsicISizesDirty() override;
  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) override;
  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) override;

  
  virtual mozilla::LogicalSize
  ComputeAutoSize(nsRenderingContext *aRenderingContext,
                  mozilla::WritingMode aWritingMode,
                  const mozilla::LogicalSize& aCBSize,
                  nscoord aAvailableISize,
                  const mozilla::LogicalSize& aMargin,
                  const mozilla::LogicalSize& aBorder,
                  const mozilla::LogicalSize& aPadding,
                  bool aShrinkWrap) override;

  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) override;

  virtual nsresult CharacterDataChanged(CharacterDataChangeInfo* aInfo) override;

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         asPrevInFlow) override;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override;

  virtual nsresult AttributeChanged(int32_t aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    int32_t aModType) override;

  virtual bool ComputesOwnOverflowArea() override { return false; }

protected:

  NS_IMETHOD DoLayout(nsBoxLayoutState& aState) override;

#ifdef DEBUG_LAYOUT
  virtual void GetBoxName(nsAutoString& aName) override;
#endif

  virtual nscoord GetIntrinsicISize() override;

 explicit nsLeafBoxFrame(nsStyleContext* aContext);

private:

 void UpdateMouseThrough();


}; 

#endif 

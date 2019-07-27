




#ifndef nsRangeFrame_h___
#define nsRangeFrame_h___

#include "mozilla/Attributes.h"
#include "mozilla/Decimal.h"
#include "mozilla/EventForwards.h"
#include "nsContainerFrame.h"
#include "nsIAnonymousContentCreator.h"
#include "nsCOMPtr.h"

class nsBaseContentList;
class nsDisplayRangeFocusRing;

class nsRangeFrame : public nsContainerFrame,
                     public nsIAnonymousContentCreator
{
  friend nsIFrame*
  NS_NewRangeFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  friend class nsDisplayRangeFocusRing;

  explicit nsRangeFrame(nsStyleContext* aContext);
  virtual ~nsRangeFrame();

  typedef mozilla::dom::Element Element;

public:
  NS_DECL_QUERYFRAME_TARGET(nsRangeFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;

  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                        const nsRect&           aDirtyRect,
                        const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE {
    return MakeFrameName(NS_LITERAL_STRING("Range"), aResult);
  }
#endif

  virtual bool IsLeaf() const MOZ_OVERRIDE { return true; }

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements) MOZ_OVERRIDE;
  virtual void AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                        uint32_t aFilter) MOZ_OVERRIDE;

  virtual nsresult AttributeChanged(int32_t  aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    int32_t  aModType) MOZ_OVERRIDE;

  virtual mozilla::LogicalSize
  ComputeAutoSize(nsRenderingContext *aRenderingContext,
                  mozilla::WritingMode aWritingMode,
                  const mozilla::LogicalSize& aCBSize,
                  nscoord aAvailableISize,
                  const mozilla::LogicalSize& aMargin,
                  const mozilla::LogicalSize& aBorder,
                  const mozilla::LogicalSize& aPadding,
                  bool aShrinkWrap) MOZ_OVERRIDE;

  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;

  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    return nsContainerFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

  nsStyleContext* GetAdditionalStyleContext(int32_t aIndex) const MOZ_OVERRIDE;
  void SetAdditionalStyleContext(int32_t aIndex,
                                 nsStyleContext* aStyleContext) MOZ_OVERRIDE;

  









  bool IsHorizontal(const nsSize *aFrameSizeOverride = nullptr) const;

  double GetMin() const;
  double GetMax() const;
  double GetValue() const;

  




  
  double GetValueAsFractionOfRange();

  


  bool ShouldUseNativeStyle() const;

  mozilla::Decimal GetValueAtEventPoint(mozilla::WidgetGUIEvent* aEvent);

  






  void UpdateForValueChange();

  virtual Element* GetPseudoElement(nsCSSPseudoElements::Type aType) MOZ_OVERRIDE;

private:

  nsresult MakeAnonymousDiv(Element** aResult,
                            nsCSSPseudoElements::Type aPseudoType,
                            nsTArray<ContentInfo>& aElements);

  
  void ReflowAnonymousContent(nsPresContext*           aPresContext,
                              nsHTMLReflowMetrics&     aDesiredSize,
                              const nsHTMLReflowState& aReflowState);

  void DoUpdateThumbPosition(nsIFrame* aThumbFrame,
                             const nsSize& aRangeSize);

  void DoUpdateRangeProgressFrame(nsIFrame* aProgressFrame,
                                  const nsSize& aRangeSize);

  



  nsCOMPtr<Element> mTrackDiv;

  





  nsCOMPtr<Element> mProgressDiv;

  



  nsCOMPtr<Element> mThumbDiv;

  


  nsRefPtr<nsStyleContext> mOuterFocusStyle;
};

#endif

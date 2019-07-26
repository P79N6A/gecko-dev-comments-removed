




#ifndef nsRangeFrame_h___
#define nsRangeFrame_h___

#include "mozilla/Attributes.h"
#include "nsContainerFrame.h"
#include "nsIAnonymousContentCreator.h"
#include "nsCOMPtr.h"

class nsBaseContentList;
class nsGUIEvent;

class nsRangeFrame : public nsContainerFrame,
                     public nsIAnonymousContentCreator
{
  friend nsIFrame*
  NS_NewRangeFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  nsRangeFrame(nsStyleContext* aContext);
  virtual ~nsRangeFrame();

public:
  NS_DECL_QUERYFRAME_TARGET(nsRangeFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                        const nsRect&           aDirtyRect,
                        const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  NS_IMETHOD Reflow(nsPresContext*           aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus) MOZ_OVERRIDE;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE {
    return MakeFrameName(NS_LITERAL_STRING("Range"), aResult);
  }
#endif

  virtual bool IsLeaf() const MOZ_OVERRIDE { return true; }

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements) MOZ_OVERRIDE;
  virtual void AppendAnonymousContentTo(nsBaseContentList& aElements,
                                        uint32_t aFilter) MOZ_OVERRIDE;

  NS_IMETHOD AttributeChanged(int32_t  aNameSpaceID,
                              nsIAtom* aAttribute,
                              int32_t  aModType) MOZ_OVERRIDE;

  virtual nsSize ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                 nsSize aCBSize, nscoord aAvailableWidth,
                                 nsSize aMargin, nsSize aBorder,
                                 nsSize aPadding, bool aShrinkWrap) MOZ_OVERRIDE;

  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;

  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    return nsContainerFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

  









  bool IsHorizontal(const nsSize *aFrameSizeOverride = nullptr) const;

  double GetMin() const;
  double GetMax() const;
  double GetValue() const;

  


  bool ShouldUseNativeStyle() const;

  double GetValueAtEventPoint(nsGUIEvent* aEvent);

  






  void UpdateForValueChange();

private:

  nsresult MakeAnonymousDiv(nsIContent** aResult,
                            nsCSSPseudoElements::Type aPseudoType,
                            nsTArray<ContentInfo>& aElements);

  
  nsresult ReflowAnonymousContent(nsPresContext*           aPresContext,
                                  nsHTMLReflowMetrics&     aDesiredSize,
                                  const nsHTMLReflowState& aReflowState);

  void DoUpdateThumbPosition(nsIFrame* aThumbFrame,
                             const nsSize& aRangeSize);

  void DoUpdateRangeProgressFrame(nsIFrame* aProgressFrame,
                                  const nsSize& aRangeSize);

  





  double GetValueAsFractionOfRange();

  



  nsCOMPtr<nsIContent> mTrackDiv;

  





  nsCOMPtr<nsIContent> mProgressDiv;

  



  nsCOMPtr<nsIContent> mThumbDiv;
};

#endif

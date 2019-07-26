




#ifndef nsRangeFrame_h___
#define nsRangeFrame_h___

#include "mozilla/Attributes.h"
#include "nsContainerFrame.h"
#include "nsIAnonymousContentCreator.h"
#include "nsRepeatService.h"
#include "nsCOMPtr.h"
#include "nsDOMTouchEvent.h"
#include "nsIDOMEventListener.h"

class nsBaseContentList;
class nsRangeFrame;

class nsRangeMediator : public nsIDOMEventListener
{
public:

  NS_DECL_ISUPPORTS

  nsRangeFrame* mRange;

  nsRangeMediator(nsRangeFrame* aRange) {  mRange = aRange; }
  virtual ~nsRangeMediator() {}

  virtual void SetRange(nsRangeFrame* aRange) { mRange = aRange; }
};

class nsRangeFrame : public nsContainerFrame,
                     public nsIAnonymousContentCreator
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsRangeFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  NS_IMETHODIMP SetInitialChildList(ChildListID     aListID,
                                   nsFrameList&    aChildList) MOZ_OVERRIDE;

  nsRangeFrame(nsStyleContext* aContext);
  virtual ~nsRangeFrame();

  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  NS_IMETHOD Reflow(nsPresContext*           aCX,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const {
    return MakeFrameName(NS_LITERAL_STRING("Range"), aResult);
  }
#endif

  virtual bool IsLeaf() const MOZ_OVERRIDE { return true; }

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements) MOZ_OVERRIDE;
  virtual void AppendAnonymousContentTo(nsBaseContentList& aElements,
                                        uint32_t aFilter) MOZ_OVERRIDE;

  NS_IMETHOD AttributeChanged(int32_t  aNameSpaceID,
                              nsIAtom* aAttribute,
                              int32_t  aModType);

  virtual nsSize ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                 nsSize aCBSize, nscoord aAvailableWidth,
                                 nsSize aMargin, nsSize aBorder,
                                 nsSize aPadding, bool aShrinkWrap) MOZ_OVERRIDE;

  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);

  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    return nsContainerFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

  double GetMin() const;
  double GetMax() const;
  double GetValue() const;

  


  bool ShouldUseNativeStyle() const;

private:
  bool IsHorizontal(nscoord width, nscoord height);
  void SetCurrentThumbPosition(double position);
  void AddListener();
  void RemoveListener();

  nscoord mChange;

protected:
  
  void ReflowBarFrame(nsPresContext*           aPresContext,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus);

  



  nsCOMPtr<nsIContent> mThumbDiv;

  



  nsCOMPtr<nsIContent> mProgressDiv;

};

#endif







#ifndef nsMeterFrame_h___
#define nsMeterFrame_h___

#include "nsContainerFrame.h"
#include "nsIAnonymousContentCreator.h"
#include "nsCOMPtr.h"

class nsMeterFrame : public nsContainerFrame,
                     public nsIAnonymousContentCreator

{
public:
  NS_DECL_QUERYFRAME_TARGET(nsMeterFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  nsMeterFrame(nsStyleContext* aContext);
  virtual ~nsMeterFrame();

  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  NS_IMETHOD Reflow(nsPresContext*           aCX,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const {
    return MakeFrameName(NS_LITERAL_STRING("Meter"), aResult);
  }
#endif

  virtual bool IsLeaf() const { return true; }

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements);
  virtual void AppendAnonymousContentTo(nsBaseContentList& aElements,
                                        uint32_t aFilter);

  NS_IMETHOD AttributeChanged(int32_t  aNameSpaceID,
                              nsIAtom* aAttribute,
                              int32_t  aModType);

  virtual nsSize ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                 nsSize aCBSize, nscoord aAvailableWidth,
                                 nsSize aMargin, nsSize aBorder,
                                 nsSize aPadding, bool aShrinkWrap);

  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);

  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    return nsContainerFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

  


  bool ShouldUseNativeStyle() const;

protected:
  
  void ReflowBarFrame(nsIFrame*                aBarFrame,
                      nsPresContext*           aPresContext,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus);
  



  nsCOMPtr<nsIContent> mBarDiv;
};

#endif

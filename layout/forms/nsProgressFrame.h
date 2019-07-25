




































#ifndef nsProgressFrame_h___
#define nsProgressFrame_h___

#include "nsHTMLContainerFrame.h"
#include "nsIAnonymousContentCreator.h"
#include "nsCOMPtr.h"


class nsProgressFrame : public nsHTMLContainerFrame,
                        public nsIAnonymousContentCreator
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsProgressFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  nsProgressFrame(nsStyleContext* aContext);
  virtual ~nsProgressFrame();

  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  NS_IMETHOD Reflow(nsPresContext*           aCX,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const {
    return MakeFrameName(NS_LITERAL_STRING("Progress"), aResult);
  }
#endif

  virtual PRBool IsLeaf() const { return PR_TRUE; }

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements);
  virtual void AppendAnonymousContentTo(nsBaseContentList& aElements,
                                        PRUint32 aFilter);

  NS_IMETHOD AttributeChanged(PRInt32  aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32  aModType);

  virtual nsSize ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                 nsSize aCBSize, nscoord aAvailableWidth,
                                 nsSize aMargin, nsSize aBorder,
                                 nsSize aPadding, PRBool aShrinkWrap);

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsHTMLContainerFrame::IsFrameOfType(aFlags &
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


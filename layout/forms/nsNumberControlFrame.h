




#ifndef nsNumberControlFrame_h__
#define nsNumberControlFrame_h__

#include "nsContainerFrame.h"
#include "nsIFormControlFrame.h"
#include "nsIAnonymousContentCreator.h"
#include "nsCOMPtr.h"

class nsPresContext;

namespace mozilla {
namespace dom {
class HTMLInputElement;
}
}




class nsNumberControlFrame MOZ_FINAL : public nsContainerFrame
                                     , public nsIAnonymousContentCreator
{
  friend nsIFrame*
  NS_NewNumberControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  typedef mozilla::dom::HTMLInputElement HTMLInputElement;

  nsNumberControlFrame(nsStyleContext* aContext);

public:
  NS_DECL_QUERYFRAME_TARGET(nsNumberControlFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  virtual bool IsLeaf() const MOZ_OVERRIDE { return true; }

  NS_IMETHOD Reflow(nsPresContext*           aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements) MOZ_OVERRIDE;
  virtual void AppendAnonymousContentTo(nsBaseContentList& aElements,
                                        uint32_t aFilter) MOZ_OVERRIDE;

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE {
    return MakeFrameName(NS_LITERAL_STRING("NumberControl"), aResult);
  }
#endif

  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    return nsContainerFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

  



  void UpdateForValueChange(const nsAString& aValue);

  



  void HandlingInputEvent(bool aHandlingEvent)
  {
    mHandlingInputEvent = aHandlingEvent;
  }

  HTMLInputElement* GetAnonTextControl();

private:

  nsresult MakeAnonymousElement(nsIContent** aResult,
                                nsTArray<ContentInfo>& aElements,
                                nsIAtom* aTagName,
                                nsCSSPseudoElements::Type aPseudoType,
                                nsStyleContext* aParentContext);

  nsresult ReflowAnonymousContent(nsPresContext* aPresContext,
                                  nsHTMLReflowMetrics& aWrappersDesiredSize,
                                  const nsHTMLReflowState& aReflowState,
                                  nsIFrame* aOuterWrapperFrame);

  



  nsCOMPtr<nsIContent> mOuterWrapper;
  nsCOMPtr<nsIContent> mTextField;
  nsCOMPtr<nsIContent> mSpinBox;
  nsCOMPtr<nsIContent> mSpinUp;
  nsCOMPtr<nsIContent> mSpinDown;
  bool mHandlingInputEvent;
};

#endif 

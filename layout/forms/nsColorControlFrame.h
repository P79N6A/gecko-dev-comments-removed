




#ifndef nsColorControlFrame_h___
#define nsColorControlFrame_h___

#include "nsCOMPtr.h"
#include "nsHTMLButtonControlFrame.h"
#include "nsIAnonymousContentCreator.h"

typedef nsHTMLButtonControlFrame nsColorControlFrameSuper;



class nsColorControlFrame final : public nsColorControlFrameSuper,
                                  public nsIAnonymousContentCreator
{
  typedef mozilla::dom::Element Element;

public:
  friend nsIFrame* NS_NewColorControlFrame(nsIPresShell* aPresShell,
                                           nsStyleContext* aContext);

  virtual void DestroyFrom(nsIFrame* aDestructRoot) override;

  NS_DECL_QUERYFRAME_TARGET(nsColorControlFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  virtual nsIAtom* GetType() const override;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
#endif

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements) override;
  virtual void AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                        uint32_t aFilter) override;

  
  virtual nsresult AttributeChanged(int32_t  aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    int32_t  aModType) override;
  virtual bool IsLeaf() const override { return true; }
  virtual nsContainerFrame* GetContentInsertionFrame() override;

  virtual Element* GetPseudoElement(nsCSSPseudoElements::Type aType) override;

  
  nsresult UpdateColor();

private:
  explicit nsColorControlFrame(nsStyleContext* aContext);

  nsCOMPtr<Element> mColorContent;
};


#endif 

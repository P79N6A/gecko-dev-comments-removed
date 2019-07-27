




#ifndef nsColorControlFrame_h___
#define nsColorControlFrame_h___

#include "nsCOMPtr.h"
#include "nsHTMLButtonControlFrame.h"
#include "nsIAnonymousContentCreator.h"

typedef nsHTMLButtonControlFrame nsColorControlFrameSuper;



class nsColorControlFrame MOZ_FINAL : public nsColorControlFrameSuper,
                                      public nsIAnonymousContentCreator
{
  typedef mozilla::dom::Element Element;

public:
  friend nsIFrame* NS_NewColorControlFrame(nsIPresShell* aPresShell,
                                           nsStyleContext* aContext);

  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  NS_DECL_QUERYFRAME_TARGET(nsColorControlFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements) MOZ_OVERRIDE;
  virtual void AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                        uint32_t aFilter) MOZ_OVERRIDE;

  
  virtual nsresult AttributeChanged(int32_t  aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    int32_t  aModType) MOZ_OVERRIDE;
  virtual bool IsLeaf() const MOZ_OVERRIDE { return true; }
  virtual nsContainerFrame* GetContentInsertionFrame() MOZ_OVERRIDE;

  virtual Element* GetPseudoElement(nsCSSPseudoElements::Type aType) MOZ_OVERRIDE;

  
  nsresult UpdateColor();

private:
  explicit nsColorControlFrame(nsStyleContext* aContext);

  nsCOMPtr<Element> mColorContent;
};


#endif 

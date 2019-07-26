




#ifndef nsColorControlFrame_h___
#define nsColorControlFrame_h___

#include "nsCOMPtr.h"
#include "nsHTMLButtonControlFrame.h"
#include "nsIAnonymousContentCreator.h"

typedef nsHTMLButtonControlFrame nsColorControlFrameSuper;



class nsColorControlFrame MOZ_FINAL : public nsColorControlFrameSuper,
                                      public nsIAnonymousContentCreator
{
public:
  friend nsIFrame* NS_NewColorControlFrame(nsIPresShell* aPresShell,
                                           nsStyleContext* aContext);

  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME

  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements) MOZ_OVERRIDE;
  virtual void AppendAnonymousContentTo(nsBaseContentList& aElements,
                                        uint32_t aFilter) MOZ_OVERRIDE;

  
  NS_IMETHOD AttributeChanged(int32_t  aNameSpaceID,
                              nsIAtom* aAttribute,
                              int32_t  aModType) MOZ_OVERRIDE;
  virtual bool IsLeaf() const MOZ_OVERRIDE { return true; }
  virtual nsIFrame* GetContentInsertionFrame() MOZ_OVERRIDE;

  virtual nsIContent* GetPseudoElementContent(nsCSSPseudoElements::Type aType) MOZ_OVERRIDE;

private:
  nsColorControlFrame(nsStyleContext* aContext);

  
  nsresult UpdateColor();

  nsCOMPtr<nsIContent> mColorContent;
};


#endif 

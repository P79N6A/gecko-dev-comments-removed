




#ifndef nsGfxButtonControlFrame_h___
#define nsGfxButtonControlFrame_h___

#include "mozilla/Attributes.h"
#include "nsFormControlFrame.h"
#include "nsHTMLButtonControlFrame.h"
#include "nsCOMPtr.h"
#include "nsIAnonymousContentCreator.h"






class nsGfxButtonControlFrame : public nsHTMLButtonControlFrame,
                                public nsIAnonymousContentCreator
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsGfxButtonControlFrame(nsStyleContext* aContext);

  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus) MOZ_OVERRIDE;

  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

  NS_DECL_QUERYFRAME

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements);
  virtual void AppendAnonymousContentTo(nsBaseContentList& aElements,
                                        uint32_t aFilter) MOZ_OVERRIDE;
  virtual nsIFrame* CreateFrameFor(nsIContent* aContent) MOZ_OVERRIDE;

  NS_IMETHOD AttributeChanged(int32_t         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              int32_t         aModType);

  virtual bool IsLeaf() const;

  virtual nsIFrame* GetContentInsertionFrame() MOZ_OVERRIDE;

protected:
  nsresult GetDefaultLabel(nsXPIDLString& aLabel) const;

  nsresult GetLabel(nsXPIDLString& aLabel);

  virtual bool IsInput() MOZ_OVERRIDE { return true; }
private:
  nsCOMPtr<nsIContent> mTextContent;
};


#endif


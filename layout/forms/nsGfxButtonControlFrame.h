




#ifndef nsGfxButtonControlFrame_h___
#define nsGfxButtonControlFrame_h___

#include "mozilla/Attributes.h"
#include "nsHTMLButtonControlFrame.h"
#include "nsCOMPtr.h"
#include "nsIAnonymousContentCreator.h"






class nsGfxButtonControlFrame : public nsHTMLButtonControlFrame,
                                public nsIAnonymousContentCreator
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  explicit nsGfxButtonControlFrame(nsStyleContext* aContext);

  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  virtual nsresult HandleEvent(nsPresContext* aPresContext, 
                               mozilla::WidgetGUIEvent* aEvent,
                               nsEventStatus* aEventStatus) MOZ_OVERRIDE;

  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

  NS_DECL_QUERYFRAME

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements) MOZ_OVERRIDE;
  virtual void AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                        uint32_t aFilter) MOZ_OVERRIDE;
  virtual nsIFrame* CreateFrameFor(nsIContent* aContent) MOZ_OVERRIDE;

  virtual nsresult AttributeChanged(int32_t         aNameSpaceID,
                                    nsIAtom*        aAttribute,
                                    int32_t         aModType) MOZ_OVERRIDE;

  virtual bool IsLeaf() const MOZ_OVERRIDE;

  virtual nsContainerFrame* GetContentInsertionFrame() MOZ_OVERRIDE;

protected:
  nsresult GetDefaultLabel(nsXPIDLString& aLabel) const;

  nsresult GetLabel(nsXPIDLString& aLabel);

  virtual bool IsInput() MOZ_OVERRIDE { return true; }
private:
  nsCOMPtr<nsIContent> mTextContent;
};


#endif


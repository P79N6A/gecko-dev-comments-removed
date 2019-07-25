




#ifndef nsGfxButtonControlFrame_h___
#define nsGfxButtonControlFrame_h___

#include "nsFormControlFrame.h"
#include "nsHTMLButtonControlFrame.h"
#include "nsCOMPtr.h"
#include "nsIAnonymousContentCreator.h"

#ifdef ACCESSIBILITY
class nsIAccessible;
#endif






class nsGfxButtonControlFrame : public nsHTMLButtonControlFrame,
                                public nsIAnonymousContentCreator
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsGfxButtonControlFrame(nsStyleContext* aContext);

  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);

  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  NS_DECL_QUERYFRAME

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements);
  virtual void AppendAnonymousContentTo(nsBaseContentList& aElements,
                                        uint32_t aFilter);
  virtual nsIFrame* CreateFrameFor(nsIContent* aContent);

  
  virtual nsresult GetFormProperty(nsIAtom* aName, nsAString& aValue) const; 


  NS_IMETHOD AttributeChanged(int32_t         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              int32_t         aModType);

  virtual bool IsLeaf() const;

  virtual nsIFrame* GetContentInsertionFrame();

protected:
  nsresult GetDefaultLabel(nsXPIDLString& aLabel);

  nsresult GetLabel(nsXPIDLString& aLabel);

  bool IsFileBrowseButton(int32_t type); 

  virtual bool IsInput() { return true; }
private:
  nsCOMPtr<nsIContent> mTextContent;
};


#endif


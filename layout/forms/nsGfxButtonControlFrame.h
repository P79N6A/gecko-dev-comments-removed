




































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
  nsGfxButtonControlFrame(nsStyleContext* aContext);

  virtual void Destroy();

  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);

 
#ifdef ACCESSIBILITY
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
#endif

  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);


  
  virtual nsresult CreateAnonymousContent(nsTArray<nsIContent*>& aElements);
  virtual nsIFrame* CreateFrameFor(nsIContent* aContent);

  
  virtual nsresult GetFormProperty(nsIAtom* aName, nsAString& aValue) const; 


  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

  virtual PRBool IsLeaf() const;

  virtual nsIFrame* GetContentInsertionFrame();

protected:
  nsresult GetDefaultLabel(nsXPIDLString& aLabel);

  nsresult GetLabel(nsXPIDLString& aLabel);

  PRBool IsFileBrowseButton(PRInt32 type); 

private:
  NS_IMETHOD_(nsrefcnt) AddRef() { return NS_OK; }
  NS_IMETHOD_(nsrefcnt) Release() { return NS_OK; }

  nsSize mSuggestedSize;
  nsCOMPtr<nsIContent> mTextContent;
};


#endif


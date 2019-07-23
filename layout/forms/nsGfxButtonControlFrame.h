




































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

  NS_DECL_QUERYFRAME

  
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

  virtual PRBool IsInput() { return PR_TRUE; }
private:
  nsSize mSuggestedSize;
  nsCOMPtr<nsIContent> mTextContent;
};


#endif


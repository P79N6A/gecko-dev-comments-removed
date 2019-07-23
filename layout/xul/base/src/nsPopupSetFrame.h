









































#ifndef nsPopupSetFrame_h__
#define nsPopupSetFrame_h__

#include "prtypes.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"

#include "nsIPopupSetFrame.h"
#include "nsBoxFrame.h"
#include "nsFrameList.h"
#include "nsIMenuParent.h"
#include "nsITimer.h"
#include "nsISupportsArray.h"

nsIFrame* NS_NewPopupSetFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

struct nsPopupFrameList {
  nsPopupFrameList* mNextPopup;  
  nsIFrame* mPopupFrame;         
  nsIContent* mPopupContent;     
  
  nsIContent* mElementContent; 

  PRInt32 mXPos;                
  PRInt32 mYPos;                

  nsAutoString mPopupAnchor;        
  nsAutoString mPopupAlign;         

  nsAutoString mPopupType;
  PRPackedBool mCreateHandlerSucceeded;  
  PRPackedBool mIsOpen;
  nsSize mLastPref;

public:
  nsPopupFrameList(nsIContent* aPopupContent, nsPopupFrameList* aNext);
  nsPopupFrameList* GetEntry(nsIContent* aPopupContent);
  nsPopupFrameList* GetEntryByFrame(nsIFrame* aPopupFrame);
};

class nsPopupSetFrame : public nsBoxFrame, public nsIPopupSetFrame
{
public:
  nsPopupSetFrame(nsIPresShell* aShell, nsStyleContext* aContext):
    nsBoxFrame(aShell, aContext) {}

  NS_DECL_ISUPPORTS
  
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
  NS_IMETHOD AppendFrames(nsIAtom*        aListName,
                          nsIFrame*       aFrameList);
  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);
  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsIFrame*       aFrameList);
  NS_IMETHOD  SetInitialChildList(nsIAtom*        aListName,
                                  nsIFrame*       aChildList);
  
    
  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState);
#ifdef DEBUG_LAYOUT
  NS_IMETHOD SetDebug(nsBoxLayoutState& aState, PRBool aDebug);
#endif

  
  virtual void Destroy();

  
  virtual void RepositionPopup(nsPopupFrameList* aEntry, nsBoxLayoutState& aState);

  NS_IMETHOD ShowPopup(nsIContent* aElementContent, nsIContent* aPopupContent, 
                       PRInt32 aXPos, PRInt32 aYPos, 
                       const nsString& aPopupType, const nsString& anAnchorAlignment,
                       const nsString& aPopupAlignment);
  NS_IMETHOD HidePopup(nsIFrame* aPopup);
  NS_IMETHOD DestroyPopup(nsIFrame* aPopup, PRBool aDestroyEntireChain);

  PRBool OnCreate(PRInt32 aX, PRInt32 aY, nsIContent* aPopupContent);
  PRBool OnDestroy(nsIContent* aPopupContent);
  PRBool OnCreated(PRInt32 aX, PRInt32 aY, nsIContent* aPopupContent);
  static PRBool OnDestroyed(nsPresContext* aPresContext,
                            nsIContent* aPopupContent);

  void ActivatePopup(nsPopupFrameList* aEntry, PRBool aActivateFlag);
  void OpenPopup(nsPopupFrameList* aEntry, PRBool aOpenFlag);

  




  static PRBool MayOpenPopup(nsIFrame* aFrame);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
      return MakeFrameName(NS_LITERAL_STRING("PopupSet"), aResult);
  }
#endif

protected:

  nsresult AddPopupFrameList(nsIFrame* aPopupFrameList);
  nsresult AddPopupFrame(nsIFrame* aPopup);
  nsresult RemovePopupFrame(nsIFrame* aPopup);
  
  void MarkAsGenerated(nsIContent* aPopupContent);

protected:
#ifdef DEBUG_LAYOUT
  nsresult SetDebug(nsBoxLayoutState& aState, nsIFrame* aList, PRBool aDebug);
#endif

  nsPopupFrameList* mPopupList;

}; 

#endif

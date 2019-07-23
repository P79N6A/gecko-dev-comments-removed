









































#ifndef nsPopupSetFrame_h__
#define nsPopupSetFrame_h__

#include "prtypes.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"
#include "nsGkAtoms.h"

#include "nsBoxFrame.h"
#include "nsFrameList.h"
#include "nsMenuPopupFrame.h"
#include "nsIMenuParent.h"
#include "nsITimer.h"
#include "nsISupportsArray.h"

nsIFrame* NS_NewPopupSetFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

struct nsPopupFrameList {
  nsPopupFrameList* mNextPopup;  
  nsMenuPopupFrame* mPopupFrame; 
  nsIContent* mPopupContent;     

public:
  nsPopupFrameList(nsIContent* aPopupContent, nsPopupFrameList* aNext);
};

class nsPopupSetFrame : public nsBoxFrame
{
public:
  nsPopupSetFrame(nsIPresShell* aShell, nsStyleContext* aContext):
    nsBoxFrame(aShell, aContext) {}

  ~nsPopupSetFrame() {}
  
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

  
  virtual void Destroy();

  virtual nsIAtom* GetType() const { return nsGkAtoms::popupSetFrame; }

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
  
  nsPopupFrameList* mPopupList;

}; 

#endif

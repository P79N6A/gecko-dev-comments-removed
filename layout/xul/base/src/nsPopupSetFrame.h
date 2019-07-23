









































#ifndef nsPopupSetFrame_h__
#define nsPopupSetFrame_h__

#include "nsIAtom.h"

#include "nsBoxFrame.h"
#include "nsMenuPopupFrame.h"

class nsCSSFrameConstructor;

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
  NS_DECL_FRAMEARENA_HELPERS

  nsPopupSetFrame(nsIPresShell* aShell, nsStyleContext* aContext):
    nsBoxFrame(aShell, aContext) {}

  ~nsPopupSetFrame() {}
  
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
  NS_IMETHOD AppendFrames(nsIAtom*        aListName,
                          nsFrameList&    aFrameList);
  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);
  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList);
  NS_IMETHOD  SetInitialChildList(nsIAtom*        aListName,
                                  nsFrameList&    aChildList);

    
  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState);

  
  virtual void Destroy();

  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD List(FILE* out, PRInt32 aIndent) const;
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
      return MakeFrameName(NS_LITERAL_STRING("PopupSet"), aResult);
  }
#endif

protected:

  nsresult AddPopupFrameList(nsFrameList& aPopupFrameList);
  nsresult AddPopupFrame(nsIFrame* aPopup);
  nsresult RemovePopupFrame(nsIFrame* aPopup);
  
  nsPopupFrameList* mPopupList;

}; 

#endif

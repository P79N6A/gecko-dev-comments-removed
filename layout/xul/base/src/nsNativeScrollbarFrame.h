












































#ifndef nsNativeScrollbarFrame_h__
#define nsNativeScrollbarFrame_h__


#include "nsScrollbarFrame.h"
#include "nsIWidget.h"
#include "nsIScrollbarMediator.h"

class nsISupportsArray;
class nsIPresShell;
class nsPresContext;
class nsIContent;
class nsStyleContext;

nsIFrame* NS_NewNativeScrollbarFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

class nsNativeScrollbarFrame : public nsBoxFrame, public nsIScrollbarMediator
{
public:
  nsNativeScrollbarFrame(nsIPresShell* aShell, nsStyleContext* aContext):
    nsBoxFrame(aShell, aContext), mScrollbarNeedsContent(PR_TRUE) {}

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const {
    return MakeFrameName(NS_LITERAL_STRING("NativeScrollbarFrame"), aResult);
  }
#endif

  NS_IMETHOD Init(nsIContent*     aContent,
                  nsIFrame*       aParent,
                  nsIFrame*       aPrevInFlow);
           
  
  NS_IMETHOD AttributeChanged(PRInt32 aNameSpaceID, nsIAtom* aAttribute, PRInt32 aModType);

  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
  NS_IMETHOD_(nsrefcnt) AddRef() { return NS_OK; }
  NS_IMETHOD_(nsrefcnt) Release() { return NS_OK; }

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  virtual nsSize GetPrefSize(nsBoxLayoutState& aState);

  virtual void Destroy();

  
  NS_IMETHOD PositionChanged(nsISupports* aScrollbar, PRInt32 aOldIndex, PRInt32& aNewIndex);
  NS_IMETHOD ScrollbarButtonPressed(nsISupports* aScrollbar, PRInt32 aOldIndex, PRInt32 aNewIndex);
  NS_IMETHOD VisibilityChanged(nsISupports* aScrollbar, PRBool aVisible);

protected:
  
  void Hookup();

  struct Parts {
    nsIFrame*             mScrollbarFrame;
    nsIScrollbarFrame*    mIScrollbarFrame;
    nsIScrollbarMediator* mMediator;
    
    Parts(nsIFrame* aFrame, nsIScrollbarFrame* aIScrollbarFrame, nsIScrollbarMediator* aMediator) :
      mScrollbarFrame(aFrame), mIScrollbarFrame(aIScrollbarFrame), mMediator(aMediator) {}
  };
  Parts FindParts();

  PRBool IsVertical() const { return mIsVertical; }

private:

  PRPackedBool mIsVertical;
  PRPackedBool mScrollbarNeedsContent;
  nsCOMPtr<nsIWidget> mScrollbar;
  
}; 

#endif

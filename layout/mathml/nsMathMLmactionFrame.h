





































#ifndef nsMathMLmactionFrame_h___
#define nsMathMLmactionFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmactionFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmactionFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD
  Init(nsIContent*      aContent,
       nsIFrame*        aParent,
       nsIFrame*        aPrevInFlow);

  NS_IMETHOD
  SetInitialChildList(nsIAtom*        aListName,
                      nsFrameList&    aChildList);

  virtual nsresult
  ChildListChanged(PRInt32 aModType);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  virtual nsresult
  Place(nsIRenderingContext& aRenderingContext,
        PRBool               aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize);

  NS_IMETHOD
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus);

private:
  void MouseClick();
  void MouseOver();
  void MouseOut();

  class MouseListener : public nsIDOMMouseListener {
    NS_DECL_ISUPPORTS

    MouseListener(nsMathMLmactionFrame* aOwner) : mOwner(aOwner) { };



#if DEBUG_mouse
#define MOUSE(_msg) printf("maction:%p MOUSE: "#_msg" ...\n", mOwner);
#else
#define MOUSE(_msg)
#endif

    
    NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent)  { MOUSE(down) return NS_OK; }
    NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent) { MOUSE(up) return NS_OK; }
    NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent);
    NS_IMETHOD MouseDblClick(nsIDOMEvent* aMouseEvent) { MOUSE(dblclik) return NS_OK; }
    NS_IMETHOD MouseOver(nsIDOMEvent* aMouseEvent);
    NS_IMETHOD MouseOut(nsIDOMEvent* aMouseEvent);

    
    NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent)  { MOUSE(event); return NS_OK; }

    nsMathMLmactionFrame* mOwner;
  };

protected:
  nsMathMLmactionFrame(nsStyleContext* aContext) : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmactionFrame();
  
  virtual PRIntn GetSkipSides() const { return 0; }

private:
  PRInt32         mActionType;
  PRInt32         mChildCount;
  PRInt32         mSelection;
  nsIFrame*       mSelectedFrame;
  nsString        mRestyle;
  nsCOMPtr<MouseListener> mListener;

  
  nsIFrame* 
  GetSelectedFrame();
};

#endif 

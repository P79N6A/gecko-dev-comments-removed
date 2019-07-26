




#ifndef nsMathMLmactionFrame_h___
#define nsMathMLmactionFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLSelectedFrame.h"
#include "nsIDOMEventListener.h"
#include "mozilla/Attributes.h"





class nsMathMLmactionFrame : public nsMathMLSelectedFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmactionFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  virtual void
  Init(nsIContent*      aContent,
       nsIFrame*        aParent,
       nsIFrame*        aPrevInFlow) MOZ_OVERRIDE;

  NS_IMETHOD
  SetInitialChildList(ChildListID     aListID,
                      nsFrameList&    aChildList);

  virtual nsresult
  ChildListChanged(int32_t aModType);

  NS_IMETHOD
  AttributeChanged(int32_t  aNameSpaceID,
                   nsIAtom* aAttribute,
                   int32_t  aModType);

private:
  void MouseClick();
  void MouseOver();
  void MouseOut();

  class MouseListener MOZ_FINAL : public nsIDOMEventListener
  {
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOMEVENTLISTENER

    MouseListener(nsMathMLmactionFrame* aOwner) : mOwner(aOwner) { }

    nsMathMLmactionFrame* mOwner;
  };

protected:
  nsMathMLmactionFrame(nsStyleContext* aContext) :
    nsMathMLSelectedFrame(aContext) {}
  virtual ~nsMathMLmactionFrame();
  
private:
  int32_t         mActionType;
  int32_t         mChildCount;
  int32_t         mSelection;
  nsCOMPtr<MouseListener> mListener;

  
  nsIFrame* 
  GetSelectedFrame();
};

#endif 






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
  Init(nsIContent*       aContent,
       nsContainerFrame* aParent,
       nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;

  virtual void
  SetInitialChildList(ChildListID     aListID,
                      nsFrameList&    aChildList) MOZ_OVERRIDE;

  virtual nsresult
  ChildListChanged(int32_t aModType) MOZ_OVERRIDE;

  virtual nsresult
  AttributeChanged(int32_t  aNameSpaceID,
                   nsIAtom* aAttribute,
                   int32_t  aModType) MOZ_OVERRIDE;

private:
  void MouseClick();
  void MouseOver();
  void MouseOut();

  class MouseListener MOZ_FINAL : public nsIDOMEventListener
  {
  private:
    ~MouseListener() {}

  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOMEVENTLISTENER

    explicit MouseListener(nsMathMLmactionFrame* aOwner) : mOwner(aOwner) { }

    nsMathMLmactionFrame* mOwner;
  };

protected:
  explicit nsMathMLmactionFrame(nsStyleContext* aContext) :
    nsMathMLSelectedFrame(aContext) {}
  virtual ~nsMathMLmactionFrame();
  
private:
  int32_t         mActionType;
  int32_t         mChildCount;
  int32_t         mSelection;
  nsRefPtr<MouseListener> mListener;

  
  nsIFrame* 
  GetSelectedFrame() MOZ_OVERRIDE;
};

#endif 

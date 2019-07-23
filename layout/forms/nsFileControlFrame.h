




































#ifndef nsFileControlFrame_h___
#define nsFileControlFrame_h___

#include "nsBlockFrame.h"
#include "nsIFormControlFrame.h"
#include "nsIDOMMouseListener.h"
#include "nsIAnonymousContentCreator.h"
#include "nsCOMPtr.h"

#include "nsTextControlFrame.h"
typedef   nsTextControlFrame nsNewFrame;

class nsFileControlFrame : public nsBlockFrame,
                           public nsIFormControlFrame,
                           public nsIAnonymousContentCreator
{
public:
  nsFileControlFrame(nsStyleContext* aContext);
  virtual ~nsFileControlFrame();

  NS_IMETHOD Init(nsIContent* aContent,
                  nsIFrame*   aParent,
                  nsIFrame*   aPrevInFlow);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue);
  virtual nsresult GetFormProperty(nsIAtom* aName, nsAString& aValue) const;
  virtual void SetFocus(PRBool aOn, PRBool aRepaint);

  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  
  NS_IMETHOD Reflow(nsPresContext*          aCX,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  virtual void Destroy();

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);
  virtual PRBool IsLeaf() const;



  
  virtual nsresult CreateAnonymousContent(nsTArray<nsIContent*>& aElements);

#ifdef ACCESSIBILITY
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
#endif

protected:
  class MouseListener;
  friend class MouseListener;
  class MouseListener : public nsIDOMMouseListener {
  public:
    NS_DECL_ISUPPORTS
    
    MouseListener(nsFileControlFrame* aFrame) :
      mFrame(aFrame)
    {}

    void ForgetFrame() {
      mFrame = nsnull;
    }
    
    
    
    NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent) { return NS_OK; }
    NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent) { return NS_OK; }
    NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent);
    NS_IMETHOD MouseDblClick(nsIDOMEvent* aMouseEvent) { return NS_OK; }
    NS_IMETHOD MouseOver(nsIDOMEvent* aMouseEvent) { return NS_OK; }
    NS_IMETHOD MouseOut(nsIDOMEvent* aMouseEvent) { return NS_OK; }
    NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) { return NS_OK; }

  private:
    nsFileControlFrame* mFrame;
  };
  
  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsBlockFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

  virtual PRIntn GetSkipSides() const;

  



  nsNewFrame* mTextFrame;
  



  nsCOMPtr<nsIContent> mTextContent;
  



  nsCOMPtr<nsIContent> mBrowse;
  



  nsString*           mCachedState;

  


  nsRefPtr<MouseListener> mMouseListener;

private:
  









  nsNewFrame* GetTextControlFrame(nsPresContext* aPresContext,
                                  nsIFrame* aStart);

  






  void SyncAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                PRInt32 aWhichControls);
};

#endif



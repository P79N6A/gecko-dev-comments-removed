




































#ifndef nsFileControlFrame_h___
#define nsFileControlFrame_h___

#include "nsBlockFrame.h"
#include "nsIFormControlFrame.h"
#include "nsIDOMMouseListener.h"
#include "nsIAnonymousContentCreator.h"
#include "nsICapturePicker.h"
#include "nsCOMPtr.h"

#include "nsTextControlFrame.h"
typedef   nsTextControlFrame nsNewFrame;

class nsIDOMDragEvent;

class nsFileControlFrame : public nsBlockFrame,
                           public nsIFormControlFrame,
                           public nsIAnonymousContentCreator
{
public:
  nsFileControlFrame(nsStyleContext* aContext);

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

  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);
  
  NS_IMETHOD Reflow(nsPresContext*          aCX,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  virtual void DestroyFrom(nsIFrame* aDestructRoot);

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);
  virtual void ContentStatesChanged(nsEventStates aStates);
  virtual PRBool IsLeaf() const;



  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements);
  virtual void AppendAnonymousContentTo(nsBaseContentList& aElements,
                                        PRUint32 aFilter);

#ifdef ACCESSIBILITY
  virtual already_AddRefed<nsAccessible> CreateAccessible();
#endif

  typedef PRBool (*AcceptAttrCallback)(const nsAString&, void*);
  void ParseAcceptAttribute(AcceptAttrCallback aCallback, void* aClosure) const;

  nsIFrame* GetTextFrame() { return mTextFrame; }

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
    NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent) = 0;
    NS_IMETHOD MouseDblClick(nsIDOMEvent* aMouseEvent) { return NS_OK; }
    NS_IMETHOD MouseOver(nsIDOMEvent* aMouseEvent) { return NS_OK; }
    NS_IMETHOD MouseOut(nsIDOMEvent* aMouseEvent) { return NS_OK; }
    NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) { return NS_OK; }

  protected:
    nsFileControlFrame* mFrame;
  };

  class SyncDisabledStateEvent;
  friend class SyncDisabledStateEvent;
  class SyncDisabledStateEvent : public nsRunnable
  {
  public:
    SyncDisabledStateEvent(nsFileControlFrame* aFrame)
      : mFrame(aFrame)
    {}

    NS_IMETHOD Run() {
      nsFileControlFrame* frame = static_cast<nsFileControlFrame*>(mFrame.GetFrame());
      NS_ENSURE_STATE(frame);

      frame->SyncDisabledState();
      return NS_OK;
    }

  private:
    nsWeakFrame mFrame;
  };

  class CaptureMouseListener: public MouseListener {
  public:
    CaptureMouseListener(nsFileControlFrame* aFrame) : MouseListener(aFrame),
                                                       mMode(0) {};
    NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent);
    PRUint32 mMode;
  };
  
  class BrowseMouseListener: public MouseListener {
  public:
    BrowseMouseListener(nsFileControlFrame* aFrame) : MouseListener(aFrame) {};
     NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent);
     NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

     static PRBool IsValidDropData(nsIDOMDragEvent* aEvent);
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

  



  nsCOMPtr<nsIContent> mCapture;

  


  nsRefPtr<BrowseMouseListener> mMouseListener;
  nsRefPtr<CaptureMouseListener> mCaptureMouseListener;

private:
  









  nsNewFrame* GetTextControlFrame(nsPresContext* aPresContext,
                                  nsIFrame* aStart);

  





  void SyncAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                PRInt32 aWhichControls);

  


  void SyncDisabledState();
};

#endif



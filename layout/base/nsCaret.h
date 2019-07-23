








































#include "nsCoord.h"
#include "nsISelectionListener.h"
#include "nsIRenderingContext.h"
#include "nsITimer.h"
#include "nsICaret.h"
#include "nsWeakPtr.h"

class nsIView;



class nsCaret : public nsICaret,
                public nsISelectionListener
{
  public:

                  nsCaret();
    virtual       ~nsCaret();
        
    NS_DECL_ISUPPORTS

  public:
  
    
    NS_IMETHOD    Init(nsIPresShell *inPresShell);
    NS_IMETHOD    Terminate();

    NS_IMETHOD    GetCaretDOMSelection(nsISelection **outDOMSel);
    NS_IMETHOD    SetCaretDOMSelection(nsISelection *inDOMSel);
    NS_IMETHOD    GetCaretVisible(PRBool *outMakeVisible);
    NS_IMETHOD    SetCaretVisible(PRBool intMakeVisible);
    NS_IMETHOD    SetCaretReadOnly(PRBool inMakeReadonly);
    virtual PRBool GetCaretReadOnly()
    {
      return mReadOnly;
    }
    NS_IMETHOD    GetCaretCoordinates(EViewCoordinates aRelativeToType,
                                      nsISelection *inDOMSel,
                                      nsRect* outCoordinates,
                                      PRBool* outIsCollapsed,
                                      nsIView **outView);
    NS_IMETHOD    EraseCaret();

    NS_IMETHOD    SetVisibilityDuringSelection(PRBool aVisibility);
    NS_IMETHOD    DrawAtPosition(nsIDOMNode* aNode, PRInt32 aOffset);
    nsIFrame*     GetCaretFrame();
    nsRect        GetCaretRect()
    {
      nsRect r;
      r.UnionRect(mCaretRect, GetHookRect());
      return r;
    }
    nsIContent*   GetCaretContent()
    {
      if (mDrawn)
        return mLastContent;

      return nsnull;
    }

    void      InvalidateOutsideCaret();
    void      UpdateCaretPosition();

    void      PaintCaret(nsDisplayListBuilder *aBuilder,
                         nsIRenderingContext *aCtx,
                         const nsPoint &aOffset,
                         nscolor aColor);

    void SetIgnoreUserModify(PRBool aIgnoreUserModify);

    
    NS_DECL_NSISELECTIONLISTENER

    static void   CaretBlinkCallback(nsITimer *aTimer, void *aClosure);
  
    NS_IMETHOD    GetCaretFrameForNodeOffset(nsIContent* aContentNode,
                                             PRInt32 aOffset,
                                             nsFrameSelection::HINT aFrameHint,
                                             PRUint8 aBidiLevel,
                                             nsIFrame** aReturnFrame,
                                             PRInt32* aReturnOffset);
  protected:

    void          KillTimer();
    nsresult      PrimeTimer();

    nsresult      StartBlinking();
    nsresult      StopBlinking();
    
    void          GetViewForRendering(nsIFrame *caretFrame,
                                      EViewCoordinates coordType,
                                      nsPoint &viewOffset,
                                      nsIView **outRenderingView,
                                      nsIView **outRelativeView);
    PRBool        DrawAtPositionWithHint(nsIDOMNode* aNode,
                                         PRInt32 aOffset,
                                         nsFrameSelection::HINT aFrameHint,
                                         PRUint8 aBidiLevel,
                                         PRBool aInvalidate);
    PRBool        MustDrawCaret();
    void          DrawCaret(PRBool aInvalidate);
    void          DrawCaretAfterBriefDelay();
    nsresult      UpdateCaretRects(nsIFrame* aFrame, PRInt32 aFrameOffset);
    nsresult      UpdateHookRect(nsPresContext* aPresContext);
    static void   InvalidateRects(const nsRect &aRect, const nsRect &aHook,
                                  nsIFrame *aFrame);
    nsRect        GetHookRect()
    {
#ifdef IBMBIDI
      return mHookRect;
#else
      return nsRect();
#endif
    }
    void          ToggleDrawnStatus() { mDrawn = !mDrawn; }

    nsFrameSelection* GetFrameSelection();

protected:

    nsWeakPtr             mPresShell;
    nsWeakPtr             mDomSelectionWeak;

    nsCOMPtr<nsITimer>              mBlinkTimer;
    nsCOMPtr<nsIRenderingContext>   mRendContext;

    PRUint32              mBlinkRate;         

    nscoord               mCaretWidth;   
    nscoord               mBidiIndicatorSize;   

    PRPackedBool          mVisible;           
    PRPackedBool          mDrawn;             
    PRPackedBool          mReadOnly;          
    PRPackedBool          mShowDuringSelection; 

    nsRect                mCaretRect;         

    nsCOMPtr<nsIContent>  mLastContent;       
                                              
                                              
                                              
    PRInt32               mLastContentOffset; 

    nsFrameSelection::HINT mLastHint;        
                                              

    PRPackedBool          mIgnoreUserModify;

#ifdef IBMBIDI
    nsRect                mHookRect;          
    PRUint8               mLastBidiLevel;     
    PRPackedBool          mKeyboardRTL;       
#endif
};


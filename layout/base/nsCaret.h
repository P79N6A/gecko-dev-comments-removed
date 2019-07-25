







































#ifndef nsCaret_h__
#define nsCaret_h__

#include "nsCoord.h"
#include "nsISelectionListener.h"
#include "nsITimer.h"
#include "nsWeakPtr.h"
#include "nsFrameSelection.h"

class nsRenderingContext;
class nsDisplayListBuilder;


class nsCaret : public nsISelectionListener
{
  public:

                  nsCaret();
    virtual       ~nsCaret();

    enum EViewCoordinates {
      eTopLevelWindowCoordinates,
      eRenderingViewCoordinates,
      eClosestViewCoordinates
    };

  public:

    NS_DECL_ISUPPORTS

    nsresult    Init(nsIPresShell *inPresShell);
    void    Terminate();

    nsISelection*    GetCaretDOMSelection();
    nsresult    SetCaretDOMSelection(nsISelection *inDOMSel);

    





    virtual nsresult    GetCaretVisible(bool *outMakeVisible);

    


    void    SetCaretVisible(bool intMakeVisible);

    



    void    SetCaretReadOnly(bool inMakeReadonly);

    



    bool GetCaretReadOnly()
    {
      return mReadOnly;
    }

    







    virtual nsIFrame* GetGeometry(nsISelection* aSelection,
                                  nsRect* aRect,
                                  nscoord* aBidiIndicatorSize = nsnull);

    


    void    EraseCaret();

    void    SetVisibilityDuringSelection(bool aVisibility);

    







    nsresult    DrawAtPosition(nsIDOMNode* aNode, PRInt32 aOffset);

    






    nsIFrame*     GetCaretFrame(PRInt32 *aOffset = nsnull);

    



    nsRect        GetCaretRect()
    {
      nsRect r;
      r.UnionRect(mCaretRect, GetHookRect());
      return r;
    }

    




    void      InvalidateOutsideCaret();

    




    void      UpdateCaretPosition();

    


    void      PaintCaret(nsDisplayListBuilder *aBuilder,
                         nsRenderingContext *aCtx,
                         nsIFrame *aForFrame,
                         const nsPoint &aOffset);
    








    void SetIgnoreUserModify(bool aIgnoreUserModify);

    
    NS_DECL_NSISELECTIONLISTENER

    static void   CaretBlinkCallback(nsITimer *aTimer, void *aClosure);

    nsresult      GetCaretFrameForNodeOffset(nsIContent* aContentNode,
                                             PRInt32 aOffset,
                                             nsFrameSelection::HINT aFrameHint,
                                             PRUint8 aBidiLevel,
                                             nsIFrame** aReturnFrame,
                                             PRInt32* aReturnOffset);

    NS_IMETHOD CheckCaretDrawingState();

protected:

    void          KillTimer();
    nsresult      PrimeTimer();

    void          StartBlinking();
    void          StopBlinking();

    
    
    void          InvalidateTextOverflowBlock();
    
    bool          DrawAtPositionWithHint(nsIDOMNode* aNode,
                                         PRInt32 aOffset,
                                         nsFrameSelection::HINT aFrameHint,
                                         PRUint8 aBidiLevel,
                                         bool aInvalidate);

    struct Metrics {
      nscoord mBidiIndicatorSize; 
      nscoord mCaretWidth;        
    };
    Metrics ComputeMetrics(nsIFrame* aFrame, PRInt32 aOffset, nscoord aCaretHeight);
    nsresult GetGeometryForFrame(nsIFrame* aFrame,
                                 PRInt32   aFrameOffset,
                                 nsRect*   aRect,
                                 nscoord*  aBidiIndicatorSize);

    
    
    
    
    
    bool          MustDrawCaret(bool aIgnoreDrawnState);

    void          DrawCaret(bool aInvalidate);
    void          DrawCaretAfterBriefDelay();
    bool          UpdateCaretRects(nsIFrame* aFrame, PRInt32 aFrameOffset);
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

    already_AddRefed<nsFrameSelection> GetFrameSelection();

    
    
    
    
    
    
    
    
    bool IsMenuPopupHidingCaret();

protected:

    nsWeakPtr             mPresShell;
    nsWeakPtr             mDomSelectionWeak;

    nsCOMPtr<nsITimer>    mBlinkTimer;

    
    
    PRUint32              mBlinkRate;         
    nscoord               mCaretWidthCSSPx;   
    float                 mCaretAspectRatio;  
    
    bool                  mVisible;           

    bool                  mDrawn;             
    bool                  mPendingDraw;       

    bool                  mReadOnly;          
    bool                  mShowDuringSelection; 

    bool                  mIgnoreUserModify;

#ifdef IBMBIDI
    bool                  mKeyboardRTL;       
    bool                  mBidiUI;            
    nsRect                mHookRect;          
    PRUint8               mLastBidiLevel;     
#endif
    nsRect                mCaretRect;         

    nsCOMPtr<nsIContent>  mLastContent;       
                                              
                                              
                                              
    PRInt32               mLastContentOffset; 

    nsFrameSelection::HINT mLastHint;        
                                              

};



class StCaretHider
{
public:
               StCaretHider(nsCaret* aSelCon)
               : mWasVisible(PR_FALSE), mCaret(aSelCon)
               {
                 if (mCaret)
                 {
                   mCaret->GetCaretVisible(&mWasVisible);
                   if (mWasVisible)
                     mCaret->SetCaretVisible(PR_FALSE);
                 }
               }

               ~StCaretHider()
               {
                 if (mCaret && mWasVisible)
                   mCaret->SetCaretVisible(PR_TRUE);
                 
               }

protected:

    bool                    mWasVisible;
    nsCOMPtr<nsCaret>  mCaret;
};

#endif 

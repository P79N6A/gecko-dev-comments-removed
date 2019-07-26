







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
                                  nscoord* aBidiIndicatorSize = nullptr);

    


    void    EraseCaret();

    void    SetVisibilityDuringSelection(bool aVisibility);

    







    nsresult    DrawAtPosition(nsIDOMNode* aNode, int32_t aOffset);

    






    nsIFrame*     GetCaretFrame(int32_t *aOffset = nullptr);

    



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
                                             int32_t aOffset,
                                             nsFrameSelection::HINT aFrameHint,
                                             uint8_t aBidiLevel,
                                             nsIFrame** aReturnFrame,
                                             int32_t* aReturnOffset);

    NS_IMETHOD CheckCaretDrawingState();

protected:

    void          KillTimer();
    nsresult      PrimeTimer();

    void          StartBlinking();
    void          StopBlinking();

    bool          DrawAtPositionWithHint(nsIDOMNode* aNode,
                                         int32_t aOffset,
                                         nsFrameSelection::HINT aFrameHint,
                                         uint8_t aBidiLevel,
                                         bool aInvalidate);

    struct Metrics {
      nscoord mBidiIndicatorSize; 
      nscoord mCaretWidth;        
    };
    Metrics ComputeMetrics(nsIFrame* aFrame, int32_t aOffset, nscoord aCaretHeight);
    nsresult GetGeometryForFrame(nsIFrame* aFrame,
                                 int32_t   aFrameOffset,
                                 nsRect*   aRect,
                                 nscoord*  aBidiIndicatorSize);

    
    
    
    
    
    bool          MustDrawCaret(bool aIgnoreDrawnState);

    void          DrawCaret(bool aInvalidate);
    void          DrawCaretAfterBriefDelay();
    bool          UpdateCaretRects(nsIFrame* aFrame, int32_t aFrameOffset);
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

    
    
    
    
    
    
    
    
    bool IsMenuPopupHidingCaret();

protected:

    nsWeakPtr             mPresShell;
    nsWeakPtr             mDomSelectionWeak;

    nsCOMPtr<nsITimer>    mBlinkTimer;

    
    
    uint32_t              mBlinkRate;         
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
    uint8_t               mLastBidiLevel;     
#endif
    nsRect                mCaretRect;         

    nsCOMPtr<nsIContent>  mLastContent;       
                                              
                                              
                                              
    int32_t               mLastContentOffset; 

    nsFrameSelection::HINT mLastHint;        
                                              

};

#endif 

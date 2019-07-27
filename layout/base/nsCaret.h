







#ifndef nsCaret_h__
#define nsCaret_h__

#include "mozilla/MemoryReporting.h"
#include "nsCoord.h"
#include "nsISelectionListener.h"
#include "nsIWeakReferenceUtils.h"
#include "nsFrameSelection.h"

class nsRenderingContext;
class nsDisplayListBuilder;
class nsITimer;


class nsCaret : public nsISelectionListener
{
  public:
    nsCaret();

  protected:
    virtual ~nsCaret();

  public:
    NS_DECL_ISUPPORTS

    nsresult Init(nsIPresShell *inPresShell);
    void Terminate();

    nsresult SetCaretDOMSelection(nsISelection *inDOMSel);
    nsISelection* GetCaretDOMSelection();
    







    void SetIgnoreUserModify(bool aIgnoreUserModify);
    void CheckCaretDrawingState();
    


    void SetCaretVisible(bool intMakeVisible);
    





    virtual nsresult GetCaretVisible(bool *outMakeVisible);
    



    void SetCaretReadOnly(bool inMakeReadonly);
    



    void SetVisibilityDuringSelection(bool aVisibility);

    


    void EraseCaret();
    




    void UpdateCaretPosition();
    







    nsresult DrawAtPosition(nsIDOMNode* aNode, int32_t aOffset);

    







    virtual nsIFrame* GetGeometry(nsISelection* aSelection,
                                  nsRect* aRect,
                                  nscoord* aBidiIndicatorSize = nullptr);
    






    nsIFrame* GetCaretFrame(int32_t *aOffset = nullptr);
    



    nsRect GetCaretRect()
    {
      nsRect r;
      r.UnionRect(mCaretRect, GetHookRect());
      return r;
    }

    


    void PaintCaret(nsDisplayListBuilder *aBuilder,
                    nsRenderingContext *aCtx,
                    nsIFrame *aForFrame,
                    const nsPoint &aOffset);

    
    NS_DECL_NSISELECTIONLISTENER

    static nsresult GetCaretFrameForNodeOffset(nsFrameSelection* aFrameSelection,
                                               nsIContent* aContentNode,
                                               int32_t aOffset,
                                               nsFrameSelection::HINT aFrameHint,
                                               uint8_t aBidiLevel,
                                               nsIFrame** aReturnFrame,
                                               int32_t* aReturnOffset);

    size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

protected:
    static void   CaretBlinkCallback(nsITimer *aTimer, void *aClosure);

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
      return mHookRect;
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

    bool                  mKeyboardRTL;       
    nsRect                mHookRect;          
    uint8_t               mLastBidiLevel;     
    nsRect                mCaretRect;         

    nsCOMPtr<nsIContent>  mLastContent;       
                                              
                                              
                                              
    int32_t               mLastContentOffset; 

    nsFrameSelection::HINT mLastHint;        
                                              

};

#endif 

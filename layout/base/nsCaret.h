







#ifndef nsCaret_h__
#define nsCaret_h__

#include "mozilla/MemoryReporting.h"
#include "nsCoord.h"
#include "nsISelectionListener.h"
#include "nsIWeakReferenceUtils.h"
#include "CaretAssociationHint.h"
#include "nsPoint.h"
#include "nsRect.h"

class nsDisplayListBuilder;
class nsFrameSelection;
class nsIContent;
class nsIDOMNode;
class nsIFrame;
class nsINode;
class nsIPresShell;
class nsITimer;
class nsRenderingContext;

namespace mozilla {
namespace dom {
class Selection;
}
}


class nsCaret : public nsISelectionListener
{
  public:
    nsCaret();

  protected:
    virtual ~nsCaret();

  public:
    NS_DECL_ISUPPORTS

    typedef mozilla::CaretAssociationHint CaretAssociationHint;

    nsresult Init(nsIPresShell *inPresShell);
    void Terminate();

    void SetSelection(nsISelection *aDOMSel);
    nsISelection* GetSelection();

    







    void SetIgnoreUserModify(bool aIgnoreUserModify);
    


    void SetVisible(bool intMakeVisible);
    






    bool IsVisible();
    



    void SetCaretReadOnly(bool inMakeReadonly);
    



    void SetVisibilityDuringSelection(bool aVisibility);

    







    nsresult DrawAtPosition(nsIDOMNode* aNode, int32_t aOffset);

    



    void SchedulePaint();

    






    nsIFrame* GetPaintGeometry(nsRect* aRect);
    



    nsIFrame* GetGeometry(nsRect* aRect)
    {
      return GetGeometry(GetSelection(), aRect);
    }

    


    void PaintCaret(nsDisplayListBuilder *aBuilder,
                    nsRenderingContext *aCtx,
                    nsIFrame *aForFrame,
                    const nsPoint &aOffset);

    
    NS_DECL_NSISELECTIONLISTENER

    









    static nsIFrame* GetGeometry(nsISelection* aSelection,
                                 nsRect* aRect);
    static nsresult GetCaretFrameForNodeOffset(nsFrameSelection* aFrameSelection,
                                               nsIContent* aContentNode,
                                               int32_t aOffset,
                                               CaretAssociationHint aFrameHint,
                                               uint8_t aBidiLevel,
                                               nsIFrame** aReturnFrame,
                                               int32_t* aReturnOffset);

    size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

protected:
    static void   CaretBlinkCallback(nsITimer *aTimer, void *aClosure);

    void          CheckSelectionLanguageChange();

    void          ResetBlinking();
    void          StopBlinking();

    mozilla::dom::Selection* GetSelectionInternal();

    struct Metrics {
      nscoord mBidiIndicatorSize; 
      nscoord mCaretWidth;        
    };
    static Metrics ComputeMetrics(nsIFrame* aFrame, int32_t aOffset,
                                  nscoord aCaretHeight);
    static nsresult GetGeometryForFrame(nsIFrame* aFrame,
                                        int32_t   aFrameOffset,
                                        nsRect*   aRect,
                                        nscoord*  aBidiIndicatorSize);

    void          ComputeCaretRects(nsIFrame* aFrame, int32_t aFrameOffset,
                                    nsRect* aCaretRect, nsRect* aHookRect);

    nsFrameSelection* GetFrameSelection();

    
    
    
    
    
    
    
    
    bool IsMenuPopupHidingCaret();

    nsWeakPtr             mPresShell;
    nsWeakPtr             mDomSelectionWeak;

    nsCOMPtr<nsITimer>    mBlinkTimer;

    



    nsCOMPtr<nsINode>     mOverrideContent;
    



    int32_t               mOverrideOffset;

    



    bool                  mIsBlinking;
    


    bool                  mIsBlinkOn;
    


    bool                  mVisible;
    



    bool                  mReadOnly;
    



    bool                  mShowDuringSelection;
    



    bool                  mIgnoreUserModify;
};

#endif 

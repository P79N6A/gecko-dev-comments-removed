







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

namespace mozilla {
namespace dom {
class Selection;
}
namespace gfx {
class DrawTarget;
}
}


class nsCaret final : public nsISelectionListener
{
    typedef mozilla::gfx::DrawTarget DrawTarget;

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

    




    void SetCaretPosition(nsIDOMNode* aNode, int32_t aOffset);

    



    void SchedulePaint();

    






    nsIFrame* GetPaintGeometry(nsRect* aRect);
    



    nsIFrame* GetGeometry(nsRect* aRect)
    {
      return GetGeometry(GetSelection(), aRect);
    }

    


    void PaintCaret(nsDisplayListBuilder *aBuilder,
                    DrawTarget& aDrawTarget,
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
    static nsRect GetGeometryForFrame(nsIFrame* aFrame,
                                      int32_t   aFrameOffset,
                                      nscoord*  aBidiIndicatorSize);

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

    void          ComputeCaretRects(nsIFrame* aFrame, int32_t aFrameOffset,
                                    nsRect* aCaretRect, nsRect* aHookRect);

    
    
    
    
    
    
    
    
    bool IsMenuPopupHidingCaret();

    nsWeakPtr             mPresShell;
    nsWeakPtr             mDomSelectionWeak;

    nsCOMPtr<nsITimer>    mBlinkTimer;

    



    nsCOMPtr<nsINode>     mOverrideContent;
    



    int32_t               mOverrideOffset;

    


    bool                  mIsBlinkOn;
    




    int32_t               mBlinkCount;
    


    bool                  mVisible;
    



    bool                  mReadOnly;
    



    bool                  mShowDuringSelection;
    



    bool                  mIgnoreUserModify;
};

#endif 

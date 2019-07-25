






































#include "nsBoxFrame.h"
#include "nsCSSRendering.h"
#include "nsStyleContext.h"
#include "nsDisplayList.h"

class nsGroupBoxFrame : public nsBoxFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsGroupBoxFrame(nsIPresShell* aShell, nsStyleContext* aContext):
    nsBoxFrame(aShell, aContext) {}

  NS_IMETHOD GetBorderAndPadding(nsMargin& aBorderAndPadding);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const {
    return MakeFrameName(NS_LITERAL_STRING("GroupBoxFrame"), aResult);
  }
#endif

  virtual PRBool HonorPrintBackgroundSettings() { return PR_FALSE; }

  void PaintBorderBackground(nsIRenderingContext& aRenderingContext,
      nsPoint aPt, const nsRect& aDirtyRect);

  
  
  
  virtual void GetInitialOrientation(PRBool& aHorizontal) { aHorizontal = PR_FALSE; }
  virtual PRBool GetInitialHAlignment(Halignment& aHalign)  { aHalign = hAlign_Left; return PR_TRUE; } 
  virtual PRBool GetInitialVAlignment(Valignment& aValign)  { aValign = vAlign_Top; return PR_TRUE; } 
  virtual PRBool GetInitialAutoStretch(PRBool& aStretch)    { aStretch = PR_TRUE; return PR_TRUE; } 

  nsIBox* GetCaptionBox(nsPresContext* aPresContext, nsRect& aCaptionRect);
};





















nsIFrame*
NS_NewGroupBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsGroupBoxFrame(aPresShell, aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsGroupBoxFrame)

class nsDisplayXULGroupBackground : public nsDisplayItem {
public:
  nsDisplayXULGroupBackground(nsDisplayListBuilder* aBuilder,
                              nsGroupBoxFrame* aFrame) :
    nsDisplayItem(aBuilder, aFrame) {
    MOZ_COUNT_CTOR(nsDisplayXULGroupBackground);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayXULGroupBackground() {
    MOZ_COUNT_DTOR(nsDisplayXULGroupBackground);
  }
#endif

  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) {
    aOutFrames->AppendElement(mFrame);
  }
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsIRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("XULGroupBackground", TYPE_XUL_GROUP_BACKGROUND)
};

void
nsDisplayXULGroupBackground::Paint(nsDisplayListBuilder* aBuilder,
                                   nsIRenderingContext* aCtx)
{
  static_cast<nsGroupBoxFrame*>(mFrame)->
    PaintBorderBackground(*aCtx, ToReferenceFrame(), mVisibleRect);
}

NS_IMETHODIMP
nsGroupBoxFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                  const nsRect&           aDirtyRect,
                                  const nsDisplayListSet& aLists)
{
  
  if (IsVisibleForPainting(aBuilder)) {
    nsresult rv = aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
        nsDisplayXULGroupBackground(aBuilder, this));
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = DisplayOutline(aBuilder, aLists);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return BuildDisplayListForChildren(aBuilder, aDirtyRect, aLists);
  
}

void
nsGroupBoxFrame::PaintBorderBackground(nsIRenderingContext& aRenderingContext,
    nsPoint aPt, const nsRect& aDirtyRect) {
  PRIntn skipSides = 0;
  const nsStyleBorder* borderStyleData = GetStyleBorder();
  const nsMargin& border = borderStyleData->GetActualBorder();
  nscoord yoff = 0;
  nsPresContext* presContext = PresContext();

  nsRect groupRect;
  nsIBox* groupBox = GetCaptionBox(presContext, groupRect);

  if (groupBox) {        
    
    
    nsMargin groupMargin;
    groupBox->GetStyleMargin()->GetMargin(groupMargin);
    groupRect.Inflate(groupMargin);
 
    if (border.top < groupRect.height)
        yoff = (groupRect.height - border.top)/2 + groupRect.y;
  }

  nsRect rect(aPt.x, aPt.y + yoff, mRect.width, mRect.height - yoff);

  groupRect += aPt;

  nsCSSRendering::PaintBackground(presContext, aRenderingContext, this,
                                  aDirtyRect, rect,
                                  nsCSSRendering::PAINTBG_SYNC_DECODE_IMAGES);

  if (groupBox) {

    
    

    
    nsRect clipRect(rect);
    clipRect.width = groupRect.x - rect.x;
    clipRect.height = border.top;

    aRenderingContext.PushState();
    aRenderingContext.SetClipRect(clipRect, nsClipCombine_kIntersect);
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect, rect, mStyleContext, skipSides);

    aRenderingContext.PopState();


    
    clipRect = rect;
    clipRect.x = groupRect.XMost();
    clipRect.width = rect.XMost() - groupRect.XMost();
    clipRect.height = border.top;

    aRenderingContext.PushState();
    aRenderingContext.SetClipRect(clipRect, nsClipCombine_kIntersect);
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect, rect, mStyleContext, skipSides);

    aRenderingContext.PopState();

    
  
    

    clipRect = rect;
    clipRect.y += border.top;
    clipRect.height = mRect.height - (yoff + border.top);
  
    aRenderingContext.PushState();
    aRenderingContext.SetClipRect(clipRect, nsClipCombine_kIntersect);
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect, rect, mStyleContext, skipSides);

    aRenderingContext.PopState();
    
  } else {
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect, nsRect(aPt, GetSize()),
                                mStyleContext, skipSides);
  }
}

nsIBox*
nsGroupBoxFrame::GetCaptionBox(nsPresContext* aPresContext, nsRect& aCaptionRect)
{
    
    nsIBox* box = GetChildBox();

    
    if (!box)
      return nsnull;

    
    box = box->GetChildBox();

    
    if (!box)
      return nsnull;

    
    nsIBox* child = box->GetChildBox();

    if (child) {
       
       nsRect parentRect(box->GetRect());
       aCaptionRect = child->GetRect();
       aCaptionRect.x += parentRect.x;
       aCaptionRect.y += parentRect.y;
    }

    return child;
}

NS_IMETHODIMP
nsGroupBoxFrame::GetBorderAndPadding(nsMargin& aBorderAndPadding)
{
  aBorderAndPadding.SizeTo(0,0,0,0);
  return NS_OK;
}


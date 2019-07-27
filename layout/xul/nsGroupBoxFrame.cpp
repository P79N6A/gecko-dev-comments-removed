






#include "nsBoxFrame.h"
#include "nsCSSRendering.h"
#include "nsRenderingContext.h"
#include "nsStyleContext.h"
#include "nsDisplayList.h"

class nsGroupBoxFrame : public nsBoxFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsGroupBoxFrame(nsIPresShell* aShell, nsStyleContext* aContext):
    nsBoxFrame(aShell, aContext) {}

  virtual nsresult GetBorderAndPadding(nsMargin& aBorderAndPadding);

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const {
    return MakeFrameName(NS_LITERAL_STRING("GroupBoxFrame"), aResult);
  }
#endif

  virtual bool HonorPrintBackgroundSettings() { return false; }

  void PaintBorderBackground(nsRenderingContext& aRenderingContext,
      nsPoint aPt, const nsRect& aDirtyRect);

  
  
  
  virtual void GetInitialOrientation(bool& aHorizontal) { aHorizontal = false; }
  virtual bool GetInitialHAlignment(Halignment& aHalign)  { aHalign = hAlign_Left; return true; } 
  virtual bool GetInitialVAlignment(Valignment& aValign)  { aValign = vAlign_Top; return true; } 
  virtual bool GetInitialAutoStretch(bool& aStretch)    { aStretch = true; return true; } 

  nsIFrame* GetCaptionBox(nsPresContext* aPresContext, nsRect& aCaptionRect);
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
                     nsRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("XULGroupBackground", TYPE_XUL_GROUP_BACKGROUND)
};

void
nsDisplayXULGroupBackground::Paint(nsDisplayListBuilder* aBuilder,
                                   nsRenderingContext* aCtx)
{
  static_cast<nsGroupBoxFrame*>(mFrame)->
    PaintBorderBackground(*aCtx, ToReferenceFrame(), mVisibleRect);
}

void
nsGroupBoxFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                  const nsRect&           aDirtyRect,
                                  const nsDisplayListSet& aLists)
{
  
  if (IsVisibleForPainting(aBuilder)) {
    aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
      nsDisplayXULGroupBackground(aBuilder, this));
    
    DisplayOutline(aBuilder, aLists);
  }

  BuildDisplayListForChildren(aBuilder, aDirtyRect, aLists);
}

void
nsGroupBoxFrame::PaintBorderBackground(nsRenderingContext& aRenderingContext,
    nsPoint aPt, const nsRect& aDirtyRect) {
  Sides skipSides;
  const nsStyleBorder* borderStyleData = StyleBorder();
  const nsMargin& border = borderStyleData->GetComputedBorder();
  nscoord yoff = 0;
  nsPresContext* presContext = PresContext();

  nsRect groupRect;
  nsIFrame* groupBox = GetCaptionBox(presContext, groupRect);

  if (groupBox) {        
    
    
    nsMargin groupMargin;
    groupBox->StyleMargin()->GetMargin(groupMargin);
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

    aRenderingContext.ThebesContext()->Save();
    aRenderingContext.IntersectClip(clipRect);
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect, rect, mStyleContext, skipSides);

    aRenderingContext.ThebesContext()->Restore();


    
    clipRect = rect;
    clipRect.x = groupRect.XMost();
    clipRect.width = rect.XMost() - groupRect.XMost();
    clipRect.height = border.top;

    aRenderingContext.ThebesContext()->Save();
    aRenderingContext.IntersectClip(clipRect);
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect, rect, mStyleContext, skipSides);

    aRenderingContext.ThebesContext()->Restore();

    
  
    

    clipRect = rect;
    clipRect.y += border.top;
    clipRect.height = mRect.height - (yoff + border.top);
  
    aRenderingContext.ThebesContext()->Save();
    aRenderingContext.IntersectClip(clipRect);
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect, rect, mStyleContext, skipSides);

    aRenderingContext.ThebesContext()->Restore();
    
  } else {
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect, nsRect(aPt, GetSize()),
                                mStyleContext, skipSides);
  }
}

nsIFrame*
nsGroupBoxFrame::GetCaptionBox(nsPresContext* aPresContext, nsRect& aCaptionRect)
{
    
    nsIFrame* box = nsBox::GetChildBox(this);

    
    if (!box)
      return nullptr;

    
    box = nsBox::GetChildBox(box);

    
    if (!box)
      return nullptr;

    
    nsIFrame* child = nsBox::GetChildBox(box);

    if (child) {
       
       nsRect parentRect(box->GetRect());
       aCaptionRect = child->GetRect();
       aCaptionRect.x += parentRect.x;
       aCaptionRect.y += parentRect.y;
    }

    return child;
}

nsresult
nsGroupBoxFrame::GetBorderAndPadding(nsMargin& aBorderAndPadding)
{
  aBorderAndPadding.SizeTo(0,0,0,0);
  return NS_OK;
}


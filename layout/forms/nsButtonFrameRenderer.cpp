



#include "nsButtonFrameRenderer.h"
#include "nsCSSRendering.h"
#include "nsPresContext.h"
#include "nsGkAtoms.h"
#include "nsCSSPseudoElements.h"
#include "nsINameSpaceManager.h"
#include "nsStyleSet.h"
#include "nsDisplayList.h"
#include "nsITheme.h"
#include "nsThemeConstants.h"
#include "nsEventStates.h"
#include "mozilla/dom/Element.h"

#define ACTIVE   "active"
#define HOVER    "hover"
#define FOCUS    "focus"

nsButtonFrameRenderer::nsButtonFrameRenderer()
{
  MOZ_COUNT_CTOR(nsButtonFrameRenderer);
}

nsButtonFrameRenderer::~nsButtonFrameRenderer()
{
  MOZ_COUNT_DTOR(nsButtonFrameRenderer);
}

void
nsButtonFrameRenderer::SetFrame(nsFrame* aFrame, nsPresContext* aPresContext)
{
  mFrame = aFrame;
  ReResolveStyles(aPresContext);
}

nsIFrame*
nsButtonFrameRenderer::GetFrame()
{
  return mFrame;
}

void
nsButtonFrameRenderer::SetDisabled(bool aDisabled, bool notify)
{
  if (aDisabled)
    mFrame->GetContent()->SetAttr(kNameSpaceID_None, nsGkAtoms::disabled, EmptyString(),
                                  notify);
  else
    mFrame->GetContent()->UnsetAttr(kNameSpaceID_None, nsGkAtoms::disabled, notify);
}

bool
nsButtonFrameRenderer::isDisabled() 
{
  return mFrame->GetContent()->AsElement()->
    State().HasState(NS_EVENT_STATE_DISABLED);
}

class nsDisplayButtonBoxShadowOuter : public nsDisplayItem {
public:
  nsDisplayButtonBoxShadowOuter(nsDisplayListBuilder* aBuilder,
                                nsButtonFrameRenderer* aRenderer)
    : nsDisplayItem(aBuilder, aRenderer->GetFrame()), mBFR(aRenderer) {
    MOZ_COUNT_CTOR(nsDisplayButtonBoxShadowOuter);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayButtonBoxShadowOuter() {
    MOZ_COUNT_DTOR(nsDisplayButtonBoxShadowOuter);
  }
#endif  
  
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx);
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap);
  NS_DISPLAY_DECL_NAME("ButtonBoxShadowOuter", TYPE_BUTTON_BOX_SHADOW_OUTER)
private:
  nsButtonFrameRenderer* mBFR;
};

nsRect
nsDisplayButtonBoxShadowOuter::GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) {
  *aSnap = false;
  return mFrame->GetVisualOverflowRectRelativeToSelf() + ToReferenceFrame();
}

void
nsDisplayButtonBoxShadowOuter::Paint(nsDisplayListBuilder* aBuilder,
                                     nsRenderingContext* aCtx) {
  nsRect frameRect = nsRect(ToReferenceFrame(), mFrame->GetSize());

  nsRect buttonRect;
  mBFR->GetButtonRect(frameRect, buttonRect);

  nsCSSRendering::PaintBoxShadowOuter(mFrame->PresContext(), *aCtx, mFrame,
                                      buttonRect, mVisibleRect);
}

class nsDisplayButtonBorderBackground : public nsDisplayItem {
public:
  nsDisplayButtonBorderBackground(nsDisplayListBuilder* aBuilder,
                                  nsButtonFrameRenderer* aRenderer)
    : nsDisplayItem(aBuilder, aRenderer->GetFrame()), mBFR(aRenderer) {
    MOZ_COUNT_CTOR(nsDisplayButtonBorderBackground);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayButtonBorderBackground() {
    MOZ_COUNT_DTOR(nsDisplayButtonBorderBackground);
  }
#endif  
  
  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) {
    aOutFrames->AppendElement(mFrame);
  }
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx);
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap);
  NS_DISPLAY_DECL_NAME("ButtonBorderBackground", TYPE_BUTTON_BORDER_BACKGROUND)
private:
  nsButtonFrameRenderer* mBFR;
};

nsRect
nsDisplayButtonBorderBackground::GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) {
  *aSnap = false;
  return aBuilder->IsForEventDelivery() ? nsRect(ToReferenceFrame(), mFrame->GetSize())
          : mFrame->GetVisualOverflowRectRelativeToSelf() + ToReferenceFrame();
}

class nsDisplayButtonForeground : public nsDisplayItem {
public:
  nsDisplayButtonForeground(nsDisplayListBuilder* aBuilder,
                            nsButtonFrameRenderer* aRenderer)
    : nsDisplayItem(aBuilder, aRenderer->GetFrame()), mBFR(aRenderer) {
    MOZ_COUNT_CTOR(nsDisplayButtonForeground);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayButtonForeground() {
    MOZ_COUNT_DTOR(nsDisplayButtonForeground);
  }
#endif  

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("ButtonForeground", TYPE_BUTTON_FOREGROUND)
private:
  nsButtonFrameRenderer* mBFR;
};

void nsDisplayButtonBorderBackground::Paint(nsDisplayListBuilder* aBuilder,
                                            nsRenderingContext* aCtx)
{
  NS_ASSERTION(mFrame, "No frame?");
  nsPresContext* pc = mFrame->PresContext();
  nsRect r = nsRect(ToReferenceFrame(), mFrame->GetSize());
  
  
  mBFR->PaintBorderAndBackground(pc, *aCtx, mVisibleRect, r,
                                 aBuilder->GetBackgroundPaintFlags());
}

void nsDisplayButtonForeground::Paint(nsDisplayListBuilder* aBuilder,
                                      nsRenderingContext* aCtx)
{
  nsPresContext *presContext = mFrame->PresContext();
  const nsStyleDisplay *disp = mFrame->StyleDisplay();
  if (!mFrame->IsThemed(disp) ||
      !presContext->GetTheme()->ThemeDrawsFocusForWidget(disp->mAppearance)) {
    
    nsRect r = nsRect(ToReferenceFrame(), mFrame->GetSize());
    mBFR->PaintOutlineAndFocusBorders(presContext, *aCtx, mVisibleRect, r);
  }
}

nsresult
nsButtonFrameRenderer::DisplayButton(nsDisplayListBuilder* aBuilder,
                                     nsDisplayList* aBackground,
                                     nsDisplayList* aForeground)
{
  if (mFrame->StyleBorder()->mBoxShadow) {
    aBackground->AppendNewToTop(new (aBuilder)
      nsDisplayButtonBoxShadowOuter(aBuilder, this));
  }

  
  
  aBackground->AppendNewToTop(new (aBuilder)
    nsDisplayButtonBorderBackground(aBuilder, this));

  
  
  if ((mOuterFocusStyle && mOuterFocusStyle->StyleBorder()->HasBorder()) ||
      (mInnerFocusStyle && mInnerFocusStyle->StyleBorder()->HasBorder())) {
    aForeground->AppendNewToTop(new (aBuilder)
      nsDisplayButtonForeground(aBuilder, this));
  }
  return NS_OK;
}

void
nsButtonFrameRenderer::PaintOutlineAndFocusBorders(nsPresContext* aPresContext,
          nsRenderingContext& aRenderingContext,
          const nsRect& aDirtyRect,
          const nsRect& aRect)
{
  
  
  
  
  
  

  nsRect rect;

  if (mOuterFocusStyle) {
    

    GetButtonOuterFocusRect(aRect, rect);

    nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, mFrame,
                                aDirtyRect, rect, mOuterFocusStyle);
  }

  if (mInnerFocusStyle) { 
    

    GetButtonInnerFocusRect(aRect, rect);

    nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, mFrame,
                                aDirtyRect, rect, mInnerFocusStyle);
  }
}


void
nsButtonFrameRenderer::PaintBorderAndBackground(nsPresContext* aPresContext,
          nsRenderingContext& aRenderingContext,
          const nsRect& aDirtyRect,
          const nsRect& aRect,
          uint32_t aBGFlags)

{
  
  nsRect buttonRect;
  GetButtonRect(aRect, buttonRect);

  nsStyleContext* context = mFrame->StyleContext();

  nsCSSRendering::PaintBackground(aPresContext, aRenderingContext, mFrame,
                                  aDirtyRect, buttonRect, aBGFlags);
  nsCSSRendering::PaintBoxShadowInner(aPresContext, aRenderingContext,
                                      mFrame, buttonRect, aDirtyRect);
  nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, mFrame,
                              aDirtyRect, buttonRect, context);
}


void
nsButtonFrameRenderer::GetButtonOuterFocusRect(const nsRect& aRect, nsRect& focusRect)
{
  focusRect = aRect;
}

void
nsButtonFrameRenderer::GetButtonRect(const nsRect& aRect, nsRect& r)
{
  r = aRect;
  r.Deflate(GetButtonOuterFocusBorderAndPadding());
}


void
nsButtonFrameRenderer::GetButtonInnerFocusRect(const nsRect& aRect, nsRect& focusRect)
{
  GetButtonRect(aRect, focusRect);
  focusRect.Deflate(GetButtonBorderAndPadding());
  focusRect.Deflate(GetButtonInnerFocusMargin());
}


nsMargin
nsButtonFrameRenderer::GetButtonOuterFocusBorderAndPadding()
{
  nsMargin result(0,0,0,0);

  if (mOuterFocusStyle) {
    if (!mOuterFocusStyle->StylePadding()->GetPadding(result)) {
      NS_NOTYETIMPLEMENTED("percentage padding");
    }
    result += mOuterFocusStyle->StyleBorder()->GetComputedBorder();
  }

  return result;
}

nsMargin
nsButtonFrameRenderer::GetButtonBorderAndPadding()
{
  return mFrame->GetUsedBorderAndPadding();
}




nsMargin
nsButtonFrameRenderer::GetButtonInnerFocusMargin()
{
  nsMargin innerFocusMargin(0,0,0,0);

  if (mInnerFocusStyle) {
    const nsStyleMargin* margin = mInnerFocusStyle->StyleMargin();
    if (!margin->GetMargin(innerFocusMargin)) {
      NS_NOTYETIMPLEMENTED("percentage margin");
    }
  }

  return innerFocusMargin;
}

nsMargin
nsButtonFrameRenderer::GetButtonInnerFocusBorderAndPadding()
{
  nsMargin result(0,0,0,0);

  if (mInnerFocusStyle) {
    if (!mInnerFocusStyle->StylePadding()->GetPadding(result)) {
      NS_NOTYETIMPLEMENTED("percentage padding");
    }
    result += mInnerFocusStyle->StyleBorder()->GetComputedBorder();
  }

  return result;
}


nsMargin
nsButtonFrameRenderer::GetAddedButtonBorderAndPadding()
{
  return GetButtonOuterFocusBorderAndPadding() + GetButtonInnerFocusMargin() + GetButtonInnerFocusBorderAndPadding();
}




void
nsButtonFrameRenderer::ReResolveStyles(nsPresContext* aPresContext)
{
  
  nsStyleContext* context = mFrame->StyleContext();
  nsStyleSet *styleSet = aPresContext->StyleSet();

  
  mInnerFocusStyle =
    styleSet->ProbePseudoElementStyle(mFrame->GetContent()->AsElement(),
                                      nsCSSPseudoElements::ePseudo_mozFocusInner,
                                      context);

  
  mOuterFocusStyle =
    styleSet->ProbePseudoElementStyle(mFrame->GetContent()->AsElement(),
                                      nsCSSPseudoElements::ePseudo_mozFocusOuter,
                                      context);
}

nsStyleContext*
nsButtonFrameRenderer::GetStyleContext(int32_t aIndex) const
{
  switch (aIndex) {
  case NS_BUTTON_RENDERER_FOCUS_INNER_CONTEXT_INDEX:
    return mInnerFocusStyle;
  case NS_BUTTON_RENDERER_FOCUS_OUTER_CONTEXT_INDEX:
    return mOuterFocusStyle;
  default:
    return nullptr;
  }
}

void 
nsButtonFrameRenderer::SetStyleContext(int32_t aIndex, nsStyleContext* aStyleContext)
{
  switch (aIndex) {
  case NS_BUTTON_RENDERER_FOCUS_INNER_CONTEXT_INDEX:
    mInnerFocusStyle = aStyleContext;
    break;
  case NS_BUTTON_RENDERER_FOCUS_OUTER_CONTEXT_INDEX:
    mOuterFocusStyle = aStyleContext;
    break;
  }
}

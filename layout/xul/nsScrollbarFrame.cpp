











#include "nsScrollbarFrame.h"
#include "nsSliderFrame.h"
#include "nsScrollbarButtonFrame.h"
#include "nsGkAtoms.h"
#include "nsIScrollableFrame.h"
#include "nsIScrollbarMediator.h"
#include "mozilla/LookAndFeel.h"
#include "nsThemeConstants.h"
#include "nsIContent.h"
#include "nsIDOMMutationEvent.h"

using namespace mozilla;






nsIFrame*
NS_NewScrollbarFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsScrollbarFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsScrollbarFrame)

NS_QUERYFRAME_HEAD(nsScrollbarFrame)
  NS_QUERYFRAME_ENTRY(nsScrollbarFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBoxFrame)

void
nsScrollbarFrame::Init(nsIContent*       aContent,
                       nsContainerFrame* aParent,
                       nsIFrame*         aPrevInFlow)
{
  nsBoxFrame::Init(aContent, aParent, aPrevInFlow);

  
  
  
  
  mState |= NS_FRAME_REFLOW_ROOT;
}

void
nsScrollbarFrame::Reflow(nsPresContext*          aPresContext,
                         nsHTMLReflowMetrics&     aDesiredSize,
                         const nsHTMLReflowState& aReflowState,
                         nsReflowStatus&          aStatus)
{
  nsBoxFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);

  
  
  if (aReflowState.AvailableWidth() == 0) {
    aDesiredSize.Width() = 0;
  }
  if (aReflowState.AvailableHeight() == 0) {
    aDesiredSize.Height() = 0;
  }
}

nsIAtom*
nsScrollbarFrame::GetType() const
{
  return nsGkAtoms::scrollbarFrame;
}

nsresult
nsScrollbarFrame::AttributeChanged(int32_t aNameSpaceID,
                                   nsIAtom* aAttribute,
                                   int32_t aModType)
{
  nsresult rv = nsBoxFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                             aModType);

  
  
  if (aAttribute != nsGkAtoms::curpos)
    return rv;

  nsIScrollableFrame* scrollable = do_QueryFrame(GetParent());
  if (!scrollable)
    return rv;

  nsCOMPtr<nsIContent> kungFuDeathGrip(mContent);
  scrollable->CurPosAttributeChanged(mContent);
  return rv;
}

NS_IMETHODIMP
nsScrollbarFrame::HandlePress(nsPresContext* aPresContext,
                              WidgetGUIEvent* aEvent,
                              nsEventStatus* aEventStatus)
{
  return NS_OK;
}

NS_IMETHODIMP
nsScrollbarFrame::HandleMultiplePress(nsPresContext* aPresContext,
                                      WidgetGUIEvent* aEvent,
                                      nsEventStatus* aEventStatus,
                                      bool aControlHeld)
{
  return NS_OK;
}

NS_IMETHODIMP 
nsScrollbarFrame::HandleDrag(nsPresContext* aPresContext,
                             WidgetGUIEvent* aEvent,
                             nsEventStatus* aEventStatus)
{
  return NS_OK;
}

NS_IMETHODIMP 
nsScrollbarFrame::HandleRelease(nsPresContext* aPresContext,
                                WidgetGUIEvent* aEvent,
                                nsEventStatus* aEventStatus)
{
  return NS_OK;
}

void
nsScrollbarFrame::SetScrollbarMediatorContent(nsIContent* aMediator)
{
  mScrollbarMediator = aMediator;
}

nsIScrollbarMediator*
nsScrollbarFrame::GetScrollbarMediator()
{
  if (!mScrollbarMediator) {
    return nullptr;
  }
  nsIFrame* f = mScrollbarMediator->GetPrimaryFrame();
  nsIScrollableFrame* scrollFrame = do_QueryFrame(f);
  nsIScrollbarMediator* sbm;

  if (scrollFrame) {
    nsIFrame* scrolledFrame = scrollFrame->GetScrolledFrame();
    sbm = do_QueryFrame(scrolledFrame);
    if (sbm) {
      return sbm;
    }
  }
  sbm = do_QueryFrame(f);
  if (f && !sbm) {
    f = f->PresContext()->PresShell()->GetRootScrollFrame();
    if (f && f->GetContent() == mScrollbarMediator) {
      return do_QueryFrame(f);
    }
  }
  return sbm;
}

nsresult
nsScrollbarFrame::GetMargin(nsMargin& aMargin)
{
  aMargin.SizeTo(0,0,0,0);

  if (LookAndFeel::GetInt(LookAndFeel::eIntID_UseOverlayScrollbars) != 0) {
    nsPresContext* presContext = PresContext();
    nsITheme* theme = presContext->GetTheme();
    if (theme) {
      LayoutDeviceIntSize size;
      bool isOverridable;
      theme->GetMinimumWidgetSize(presContext, this, NS_THEME_SCROLLBAR, &size,
                                  &isOverridable);
      if (IsHorizontal()) {
        aMargin.top = -presContext->DevPixelsToAppUnits(size.height);
      }
      else {
        if (StyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
          aMargin.right = -presContext->DevPixelsToAppUnits(size.width);
        }
        else {
          aMargin.left = -presContext->DevPixelsToAppUnits(size.width);
        }
      }
      return NS_OK;
    }
  }

  return nsBox::GetMargin(aMargin);
}

void
nsScrollbarFrame::SetIncrementToLine(int32_t aDirection)
{
  
  nsIContent* content = GetContent();
  mSmoothScroll = true;
  mIncrement = aDirection * nsSliderFrame::GetIncrement(content);
}

void
nsScrollbarFrame::SetIncrementToPage(int32_t aDirection)
{
  
  nsIContent* content = GetContent();
  mSmoothScroll = true;
  mIncrement = aDirection * nsSliderFrame::GetPageIncrement(content);
}

void
nsScrollbarFrame::SetIncrementToWhole(int32_t aDirection)
{
  
  nsIContent* content = GetContent();
  if (aDirection == -1)
    mIncrement = -nsSliderFrame::GetCurrentPosition(content);
  else
    mIncrement = nsSliderFrame::GetMaxPosition(content) -
                 nsSliderFrame::GetCurrentPosition(content);
  
  
  mSmoothScroll = false;
}

int32_t
nsScrollbarFrame::MoveToNewPosition()
{
  
  nsCOMPtr<nsIContent> content = GetContent();

  
  int32_t curpos = nsSliderFrame::GetCurrentPosition(content);

  
  int32_t maxpos = nsSliderFrame::GetMaxPosition(content);

  
  if (mIncrement) {
    curpos += mIncrement;
  }

  
  if (curpos < 0) {
    curpos = 0;
  } else if (curpos > maxpos) {
    curpos = maxpos;
  }

  
  nsAutoString curposStr;
  curposStr.AppendInt(curpos);

  nsWeakFrame weakFrame(this);
  if (mSmoothScroll) {
    content->SetAttr(kNameSpaceID_None, nsGkAtoms::smooth, NS_LITERAL_STRING("true"), false);
  }
  content->SetAttr(kNameSpaceID_None, nsGkAtoms::curpos, curposStr, false);
  
  AttributeChanged(kNameSpaceID_None, nsGkAtoms::curpos, nsIDOMMutationEvent::MODIFICATION);
  if (!weakFrame.IsAlive()) {
    return curpos;
  }
  
  nsIFrame::ChildListIterator childLists(this);
  for (; !childLists.IsDone(); childLists.Next()) {
    nsFrameList::Enumerator childFrames(childLists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      nsIFrame* f = childFrames.get();
      nsSliderFrame* sliderFrame = do_QueryFrame(f);
      if (sliderFrame) {
        sliderFrame->AttributeChanged(kNameSpaceID_None, nsGkAtoms::curpos, nsIDOMMutationEvent::MODIFICATION);
        if (!weakFrame.IsAlive()) {
          return curpos;
        }
      }
    }
  }
  
  const nsStyleDisplay* disp = StyleDisplay();
  nsPresContext* presContext = PresContext();
  if (disp->mAppearance) {
    nsITheme *theme = presContext->GetTheme();
    if (theme && theme->ThemeSupportsWidget(presContext, this, disp->mAppearance)) {
      bool repaint;
      theme->WidgetStateChanged(this, disp->mAppearance, nsGkAtoms::curpos, &repaint);
    }
  }
  content->UnsetAttr(kNameSpaceID_None, nsGkAtoms::smooth, false);
  return curpos;
}

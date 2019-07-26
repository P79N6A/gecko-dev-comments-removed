




#include "ContentHelper.h"

#include "nsContainerFrame.h"
#include "nsIContent.h"
#include "nsIScrollableFrame.h"
#include "nsLayoutUtils.h"
#include "nsStyleConsts.h"
#include "nsView.h"

namespace mozilla {
namespace widget {

uint32_t
ContentHelper::GetTouchActionFromFrame(nsIFrame* aFrame)
{
  if (!aFrame || !aFrame->GetContent() || !aFrame->GetContent()->GetPrimaryFrame()) {
    
    return NS_STYLE_TOUCH_ACTION_AUTO;
  }

  if (!aFrame->IsFrameOfType(nsIFrame::eSVG) && !aFrame->IsFrameOfType(nsIFrame::eBlockFrame)) {
    
    
    return NS_STYLE_TOUCH_ACTION_AUTO;
  }

  return (aFrame->GetContent()->GetPrimaryFrame()->StyleDisplay()->mTouchAction);
}

void
ContentHelper::UpdateAllowedBehavior(uint32_t aTouchActionValue, bool aConsiderPanning, TouchBehaviorFlags& aOutBehavior)
{
  if (aTouchActionValue != NS_STYLE_TOUCH_ACTION_AUTO) {
    
    aOutBehavior &= ~AllowedTouchBehavior::DOUBLE_TAP_ZOOM;
    if (aTouchActionValue != NS_STYLE_TOUCH_ACTION_MANIPULATION) {
      
      aOutBehavior &= ~AllowedTouchBehavior::PINCH_ZOOM;
    }
  }

  if (aConsiderPanning) {
    if (aTouchActionValue == NS_STYLE_TOUCH_ACTION_NONE) {
      aOutBehavior &= ~AllowedTouchBehavior::VERTICAL_PAN;
      aOutBehavior &= ~AllowedTouchBehavior::HORIZONTAL_PAN;
    }

    
    
    if ((aTouchActionValue & NS_STYLE_TOUCH_ACTION_PAN_X) && !(aTouchActionValue & NS_STYLE_TOUCH_ACTION_PAN_Y)) {
      aOutBehavior &= ~AllowedTouchBehavior::VERTICAL_PAN;
    } else if ((aTouchActionValue & NS_STYLE_TOUCH_ACTION_PAN_Y) && !(aTouchActionValue & NS_STYLE_TOUCH_ACTION_PAN_X)) {
      aOutBehavior &= ~AllowedTouchBehavior::HORIZONTAL_PAN;
    }
  }
}

ContentHelper::TouchBehaviorFlags
ContentHelper::GetAllowedTouchBehavior(nsIWidget* aWidget, const nsIntPoint& aPoint)
{
  nsView *view = nsView::GetViewFor(aWidget);
  nsIFrame *viewFrame = view->GetFrame();

  nsPoint relativePoint = nsLayoutUtils::GetEventCoordinatesRelativeTo(aWidget, aPoint, viewFrame);

  nsIFrame *target = nsLayoutUtils::GetFrameForPoint(viewFrame, relativePoint, nsLayoutUtils::IGNORE_ROOT_SCROLL_FRAME);
  nsIScrollableFrame *nearestScrollableParent = nsLayoutUtils::GetNearestScrollableFrame(target, 0);
  nsIFrame* nearestScrollableFrame = do_QueryFrame(nearestScrollableParent);

  
  
  
  
  
  
  
  
  

  
  
  
  
  

  bool considerPanning = true;
  TouchBehaviorFlags behavior = AllowedTouchBehavior::VERTICAL_PAN | AllowedTouchBehavior::HORIZONTAL_PAN |
                                AllowedTouchBehavior::PINCH_ZOOM | AllowedTouchBehavior::DOUBLE_TAP_ZOOM;

  for (nsIFrame *frame = target; frame && frame->GetContent() && behavior; frame = frame->GetParent()) {
    UpdateAllowedBehavior(GetTouchActionFromFrame(frame), considerPanning, behavior);

    if (frame == nearestScrollableFrame) {
      
      
      considerPanning = false;
    }
  }

  return behavior;
}

}
}

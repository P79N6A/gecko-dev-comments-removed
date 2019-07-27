




#ifndef __mozilla_widget_ContentHelper_h__
#define __mozilla_widget_ContentHelper_h__

#include "nsIFrame.h"
#include "nsIWidget.h"
#include "mozilla/layers/APZCTreeManager.h"

namespace mozilla {
namespace widget {




class ContentHelper
{
  typedef mozilla::layers::AllowedTouchBehavior AllowedTouchBehavior;
  typedef uint32_t TouchBehaviorFlags;

private:
  static uint32_t GetTouchActionFromFrame(nsIFrame* aFrame);
  static void UpdateAllowedBehavior(uint32_t aTouchActionValue, bool aConsiderPanning, TouchBehaviorFlags& aOutBehavior);

public:
  




  static TouchBehaviorFlags GetAllowedTouchBehavior(nsIWidget* aWidget, const nsIntPoint& aPoint);
};

}
}

#endif 
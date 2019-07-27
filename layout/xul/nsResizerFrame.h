



#ifndef nsResizerFrame_h___
#define nsResizerFrame_h___

#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"
#include "nsTitleBarFrame.h"

class nsIBaseWindow;

class nsResizerFrame : public nsTitleBarFrame 
{
protected:
  struct Direction {
    int8_t mHorizontal;
    int8_t mVertical;
  };

public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewResizerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);  

  explicit nsResizerFrame(nsStyleContext* aContext);

  virtual nsresult HandleEvent(nsPresContext* aPresContext,
                               mozilla::WidgetGUIEvent* aEvent,
                               nsEventStatus* aEventStatus) override;

  virtual void MouseClicked(nsPresContext* aPresContext,
                            mozilla::WidgetMouseEvent* aEvent) override;

protected:
  nsIContent* GetContentToResize(nsIPresShell* aPresShell, nsIBaseWindow** aWindow);

  Direction GetDirection();

  











  static void AdjustDimensions(int32_t* aPos, int32_t* aSize,
                               int32_t aMinSize, int32_t aMaxSize,
                               int32_t aMovement, int8_t aResizerDirection);

  struct SizeInfo {
    nsString width, height;
  };
  static void SizeInfoDtorFunc(void *aObject, nsIAtom *aPropertyName,
                               void *aPropertyValue, void *aData);
  static void ResizeContent(nsIContent* aContent, const Direction& aDirection,
                            const SizeInfo& aSizeInfo, SizeInfo* aOriginalSizeInfo);
  static void MaybePersistOriginalSize(nsIContent* aContent, const SizeInfo& aSizeInfo);
  static void RestoreOriginalSize(nsIContent* aContent);

protected:
  LayoutDeviceIntRect mMouseDownRect;
  LayoutDeviceIntPoint mMouseDownPoint;
}; 

#endif 

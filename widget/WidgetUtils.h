






#ifndef mozilla_WidgetUtils_h
#define mozilla_WidgetUtils_h

#include "mozilla/EventForwards.h"
#include "mozilla/gfx/Matrix.h"
#include "nsCOMPtr.h"
#include "nsIDOMWindow.h"
#include "nsIWidget.h"
#include "nsPIDOMWindow.h"
#include "nsRect.h"

namespace mozilla {


enum ScreenRotation {
  ROTATION_0 = 0,
  ROTATION_90,
  ROTATION_180,
  ROTATION_270,

  ROTATION_COUNT
};

gfx::Matrix ComputeTransformForRotation(const nsIntRect& aBounds,
                                        ScreenRotation aRotation);

gfx::Matrix ComputeTransformForUnRotation(const nsIntRect& aBounds,
                                          ScreenRotation aRotation);

nsIntRect RotateRect(nsIntRect aRect,
                     const nsIntRect& aBounds,
                     ScreenRotation aRotation);

namespace widget {

class WidgetUtils
{
public:
  




  static void Shutdown();

  



  static already_AddRefed<nsIWidget> DOMWindowToWidget(nsIDOMWindow *aDOMWindow);

  


  static uint32_t ComputeKeyCodeFromChar(uint32_t aCharCode);

  















  static void GetLatinCharCodeForKeyCode(uint32_t aKeyCode,
                                         bool aIsCapsLock,
                                         uint32_t* aUnshiftedCharCode,
                                         uint32_t* aShiftedCharCode);
};

} 
} 

#endif 







#ifndef __mozilla_widget_WidgetUtils_h__
#define __mozilla_widget_WidgetUtils_h__

#include "nsCOMPtr.h"
#include "nsIWidget.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMWindow.h"

namespace mozilla {
namespace widget {

class WidgetUtils
{
public:

  



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

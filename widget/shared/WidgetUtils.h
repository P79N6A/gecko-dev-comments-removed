







































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

  


  static PRUint32 ComputeKeyCodeFromChar(PRUint32 aCharCode);

  















  static void GetLatinCharCodeForKeyCode(PRUint32 aKeyCode,
                                         bool aIsCapsLock,
                                         PRUint32* aUnshiftedCharCode,
                                         PRUint32* aShiftedCharCode);
};

} 
} 

#endif

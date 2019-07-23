




































#ifndef nsWidgetSupport_h__
#define nsWidgetSupport_h__

#include "nscore.h"
#include "nsISupports.h"
#include "nsIWidget.h"


struct nsRect;
class nsIAppShell;
class nsIEventListener;
class nsILookAndFeel;
class nsIMouseListener;
class nsIToolkit;
class nsIWidget;
class nsITooltipWidget;

#if (defined(XP_MAC) || defined(XP_MACOSX)) && !defined(MOZ_WIDGET_COCOA)



enum {
  kTopLevelWidgetPropertyCreator = 'MOSS',
  kTopLevelWidgetRefPropertyTag  = 'GEKO'
};
#endif

extern nsresult 
NS_ShowWidget(nsISupports* aWidget, PRBool aShow);

extern nsresult 
NS_MoveWidget(nsISupports* aWidget, PRUint32 aX, PRUint32 aY);

extern nsresult 
NS_EnableWidget(nsISupports* aWidget, PRBool aEnable);

extern nsresult 
NS_SetFocusToWidget(nsISupports* aWidget);

extern nsresult 
NS_GetWidgetNativeData(nsISupports* aWidget, void** aNativeData);





#endif

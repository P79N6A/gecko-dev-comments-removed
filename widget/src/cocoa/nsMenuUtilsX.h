





































#ifndef nsMenuUtilsX_h_
#define nsMenuUtilsX_h_

#include "nscore.h"
#include "nsGUIEvent.h"

#import <Cocoa/Cocoa.h>

class nsIContent;
class nsString;
class nsMenuBarX;


namespace nsMenuUtilsX
{
  nsEventStatus DispatchCommandTo(nsIContent* aTargetContent);
  NSString*     CreateTruncatedCocoaLabel(const nsString& itemLabel); 
  PRUint8       GeckoModifiersForNodeAttribute(const nsString& modifiersAttribute);
  unsigned int  MacModifiersForGeckoModifiers(PRUint8 geckoModifiers);
  nsMenuBarX*   GetHiddenWindowMenuBar(); 
  NSMenuItem*   GetStandardEditMenuItem(); 
  PRBool        NodeIsHiddenOrCollapsed(nsIContent* inContent);
}

#endif 

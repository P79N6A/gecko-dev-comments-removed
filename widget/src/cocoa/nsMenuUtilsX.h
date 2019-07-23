





































#ifndef nsMenuUtilsX_h_
#define nsMenuUtilsX_h_

#include "nscore.h"
#include "nsEvent.h"
#include "nsMenuBaseX.h"

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>

class nsIContent;
class nsString;
class nsMenuBarX;

extern "C" MenuRef _NSGetCarbonMenu(NSMenu* aMenu);


namespace nsMenuUtilsX
{
  void          DispatchCommandTo(nsIContent* aTargetContent);
  NSString*     GetTruncatedCocoaLabel(const nsString& itemLabel);
  PRUint8       GeckoModifiersForNodeAttribute(const nsString& modifiersAttribute);
  unsigned int  MacModifiersForGeckoModifiers(PRUint8 geckoModifiers);
  nsMenuBarX*   GetHiddenWindowMenuBar(); 
  NSMenuItem*   GetStandardEditMenuItem(); 
  PRBool        NodeIsHiddenOrCollapsed(nsIContent* inContent);
  int           CalculateNativeInsertionPoint(nsMenuObjectX* aParent, nsMenuObjectX* aChild);
}

#endif 

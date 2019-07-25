





































#ifndef nsMenuUtilsX_h_
#define nsMenuUtilsX_h_

#include "nscore.h"
#include "nsEvent.h"
#include "nsMenuBaseX.h"

#import <Cocoa/Cocoa.h>

class nsIContent;
class nsString;
class nsMenuBarX;


namespace nsMenuUtilsX
{
  void          DispatchCommandTo(nsIContent* aTargetContent);
  NSString*     GetTruncatedCocoaLabel(const nsString& itemLabel);
  PRUint8       GeckoModifiersForNodeAttribute(const nsString& modifiersAttribute);
  unsigned int  MacModifiersForGeckoModifiers(PRUint8 geckoModifiers);
  nsMenuBarX*   GetHiddenWindowMenuBar(); 
  NSMenuItem*   GetStandardEditMenuItem(); 
  bool          NodeIsHiddenOrCollapsed(nsIContent* inContent);
  int           CalculateNativeInsertionPoint(nsMenuObjectX* aParent, nsMenuObjectX* aChild);
}

#endif 

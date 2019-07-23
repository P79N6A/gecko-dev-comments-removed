





































#ifndef nsGlobalWindowCommands_h__
#define nsGlobalWindowCommands_h__

#include "nscore.h"

class nsIControllerCommandTable;

class nsWindowCommandRegistration
{
public:
  static nsresult  RegisterWindowCommands(nsIControllerCommandTable *ccm);
};



#define NS_WINDOWCONTROLLER_CID        \
 { /* 7BD05C78-6A26-11D7-B16F-0003938A9D96 */       \
  0x7BD05C78, 0x6A26, 0x11D7, {0xB1, 0x6F, 0x00, 0x03, 0x93, 0x8A, 0x9D, 0x96} }

#define NS_WINDOWCONTROLLER_CONTRACTID "@mozilla.org/dom/window-controller;1"

#endif 







































#ifndef nsStandaloneNativeMenu_h_
#define nsStandaloneNativeMenu_h_

#include "nsMenuGroupOwnerX.h"
#include "nsMenuX.h"
#include "nsIStandaloneNativeMenu.h"

class nsStandaloneNativeMenu : public nsMenuGroupOwnerX, public nsIStandaloneNativeMenu
{
public:
  nsStandaloneNativeMenu();
  virtual ~nsStandaloneNativeMenu();

  NS_DECL_ISUPPORTS  
  NS_DECL_NSISTANDALONENATIVEMENU

  
  nsMenuObjectTypeX MenuObjectType() { return eStandaloneNativeMenuObjectType; }
  void * NativeData() { return mMenu != nsnull ? mMenu->NativeData() : nsnull; }

  nsMenuX * GetMenuXObject() { return mMenu; }

protected:
  nsMenuX * mMenu;
};

#endif

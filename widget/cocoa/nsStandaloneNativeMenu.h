




#ifndef nsStandaloneNativeMenu_h_
#define nsStandaloneNativeMenu_h_

#include "nsMenuGroupOwnerX.h"
#include "nsMenuX.h"
#include "nsIStandaloneNativeMenu.h"

class nsStandaloneNativeMenu : public nsMenuGroupOwnerX, public nsIStandaloneNativeMenu
{
public:
  nsStandaloneNativeMenu();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSISTANDALONENATIVEMENU

  
  nsMenuObjectTypeX MenuObjectType() { return eStandaloneNativeMenuObjectType; }
  void * NativeData() { return mMenu != nullptr ? mMenu->NativeData() : nullptr; }

  nsMenuX * GetMenuXObject() { return mMenu; }

protected:
  virtual ~nsStandaloneNativeMenu();

  nsMenuX * mMenu;
};

#endif

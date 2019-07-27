




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

  
  nsMenuObjectTypeX MenuObjectType() override { return eStandaloneNativeMenuObjectType; }
  void * NativeData() override { return mMenu != nullptr ? mMenu->NativeData() : nullptr; }
  virtual void IconUpdated() override;

  nsMenuX * GetMenuXObject() { return mMenu; }

  
  
  
  void SetContainerStatusBarItem(NSStatusItem* aItem);

protected:
  virtual ~nsStandaloneNativeMenu();

  nsMenuX * mMenu;
  NSStatusItem* mContainerStatusBarItem;
};

#endif

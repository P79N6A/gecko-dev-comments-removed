




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

  
  nsMenuObjectTypeX MenuObjectType() MOZ_OVERRIDE { return eStandaloneNativeMenuObjectType; }
  void * NativeData() MOZ_OVERRIDE { return mMenu != nullptr ? mMenu->NativeData() : nullptr; }
  virtual void IconUpdated() MOZ_OVERRIDE;

  nsMenuX * GetMenuXObject() { return mMenu; }

  
  
  
  void SetContainerStatusBarItem(NSStatusItem* aItem);

protected:
  virtual ~nsStandaloneNativeMenu();

  nsMenuX * mMenu;
  NSStatusItem* mContainerStatusBarItem;
};

#endif

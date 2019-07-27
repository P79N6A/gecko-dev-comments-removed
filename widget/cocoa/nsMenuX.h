




#ifndef nsMenuX_h_
#define nsMenuX_h_

#import <Cocoa/Cocoa.h>

#include "nsMenuBaseX.h"
#include "nsMenuBarX.h"
#include "nsMenuGroupOwnerX.h"
#include "nsCOMPtr.h"
#include "nsChangeObserver.h"
#include "nsAutoPtr.h"

class nsMenuX;
class nsMenuItemIconX;
class nsMenuItemX;
class nsIWidget;



#if defined(MAC_OS_X_VERSION_10_6) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_6)
@interface MenuDelegate : NSObject < NSMenuDelegate >
#else
@interface MenuDelegate : NSObject
#endif
{
  nsMenuX* mGeckoMenu; 
}
- (id)initWithGeckoMenu:(nsMenuX*)geckoMenu;
@end



class nsMenuX : public nsMenuObjectX,
                public nsChangeObserver
{
public:
  nsMenuX();
  virtual ~nsMenuX();

  
  
  
  static int32_t sIndexingMenuLevel;

  NS_DECL_CHANGEOBSERVER

  
  void*             NativeData()     {return (void*)mNativeMenu;}
  nsMenuObjectTypeX MenuObjectType() {return eSubmenuObjectType;}

  
  nsresult       Create(nsMenuObjectX* aParent, nsMenuGroupOwnerX* aMenuGroupOwner, nsIContent* aNode);
  uint32_t       GetItemCount();
  nsMenuObjectX* GetItemAt(uint32_t aPos);
  nsresult       GetVisibleItemCount(uint32_t &aCount);
  nsMenuObjectX* GetVisibleItemAt(uint32_t aPos);
  nsEventStatus  MenuOpened();
  void           MenuClosed();
  void           SetRebuild(bool aMenuEvent);
  NSMenuItem*    NativeMenuItem();
  nsresult       SetupIcon();

  static bool    IsXULHelpMenu(nsIContent* aMenuContent);

protected:
  void           MenuConstruct();
  nsresult       RemoveAll();
  nsresult       SetEnabled(bool aIsEnabled);
  nsresult       GetEnabled(bool* aIsEnabled);
  void           GetMenuPopupContent(nsIContent** aResult);
  bool           OnOpen();
  bool           OnClose();
  nsresult       AddMenuItem(nsMenuItemX* aMenuItem);
  nsresult       AddMenu(nsMenuX* aMenu);
  void           LoadMenuItem(nsIContent* inMenuItemContent);  
  void           LoadSubMenu(nsIContent* inMenuContent);
  GeckoNSMenu*   CreateMenuWithGeckoString(nsString& menuTitle);

  nsTArray< nsAutoPtr<nsMenuObjectX> > mMenuObjectsArray;
  nsString                  mLabel;
  uint32_t                  mVisibleItemsCount; 
  nsMenuObjectX*            mParent; 
  nsMenuGroupOwnerX*        mMenuGroupOwner; 
  
  nsRefPtr<nsMenuItemIconX> mIcon;
  GeckoNSMenu*              mNativeMenu; 
  MenuDelegate*             mMenuDelegate; 
  
  NSMenuItem*               mNativeMenuItem; 
  bool                      mIsEnabled;
  bool                      mDestroyHandlerCalled;
  bool                      mNeedsRebuild;
  bool                      mConstructed;
  bool                      mVisible;
  bool                      mXBLAttached;
};

#endif 

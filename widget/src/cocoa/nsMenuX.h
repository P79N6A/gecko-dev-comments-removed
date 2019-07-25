





































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

  
  
  
  static PRInt32 sIndexingMenuLevel;

  NS_DECL_CHANGEOBSERVER

  
  void*             NativeData()     {return (void*)mNativeMenu;}
  nsMenuObjectTypeX MenuObjectType() {return eSubmenuObjectType;}

  
  nsresult       Create(nsMenuObjectX* aParent, nsMenuGroupOwnerX* aMenuGroupOwner, nsIContent* aNode);
  PRUint32       GetItemCount();
  nsMenuObjectX* GetItemAt(PRUint32 aPos);
  nsresult       GetVisibleItemCount(PRUint32 &aCount);
  nsMenuObjectX* GetVisibleItemAt(PRUint32 aPos);
  nsEventStatus  MenuOpened();
  void           MenuClosed();
  void           SetRebuild(bool aMenuEvent);
  NSMenuItem*    NativeMenuItem();

  static bool    IsXULHelpMenu(nsIContent* aMenuContent);

protected:
  void           MenuConstruct();
  nsresult       RemoveAll();
  nsresult       SetEnabled(bool aIsEnabled);
  nsresult       GetEnabled(bool* aIsEnabled);
  nsresult       SetupIcon();
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
  PRUint32                  mVisibleItemsCount; 
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

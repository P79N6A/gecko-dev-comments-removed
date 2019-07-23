





































#ifndef nsMenuX_h_
#define nsMenuX_h_

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>

#include "nsMenuBaseX.h"
#include "nsMenuBarX.h"
#include "nsCOMPtr.h"
#include "nsChangeObserver.h"
#include "nsAutoPtr.h"

class nsMenuX;
class nsMenuItemIconX;
class nsMenuItemX;
class nsIWidget;



@interface MenuDelegate : NSObject
{
  nsMenuX* mGeckoMenu; 
  EventHandlerRef mEventHandler;
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

  
  nsresult       Create(nsMenuObjectX* aParent, nsMenuBarX* aMenuBar, nsIContent* aNode);
  PRUint32       GetItemCount();
  nsMenuObjectX* GetItemAt(PRUint32 aPos);
  nsresult       GetVisibleItemCount(PRUint32 &aCount);
  nsMenuObjectX* GetVisibleItemAt(PRUint32 aPos);
  nsEventStatus  MenuOpened();
  void           MenuClosed();
  void           SetRebuild(PRBool aMenuEvent);
  NSMenuItem*    NativeMenuItem();

protected:
  void           MenuConstruct();
  nsresult       RemoveAll();
  nsresult       SetEnabled(PRBool aIsEnabled);
  nsresult       GetEnabled(PRBool* aIsEnabled);
  nsresult       SetupIcon();
  void           GetMenuPopupContent(nsIContent** aResult);
  PRBool         OnOpen();
  PRBool         OnClose();
  nsresult       AddMenuItem(nsMenuItemX* aMenuItem);
  nsresult       AddMenu(nsMenuX* aMenu);
  void           LoadMenuItem(nsIContent* inMenuItemContent);  
  void           LoadSubMenu(nsIContent* inMenuContent);
  GeckoNSMenu*   CreateMenuWithGeckoString(nsString& menuTitle);

  nsTArray< nsAutoPtr<nsMenuObjectX> > mMenuObjectsArray;
  nsString                  mLabel;
  PRUint32                  mVisibleItemsCount; 
  nsMenuObjectX*            mParent; 
  nsMenuBarX*               mMenuBar; 
  
  nsRefPtr<nsMenuItemIconX> mIcon;
  GeckoNSMenu*              mNativeMenu; 
  MenuDelegate*             mMenuDelegate; 
  
  NSMenuItem*               mNativeMenuItem; 
  PRPackedBool              mIsEnabled;
  PRPackedBool              mDestroyHandlerCalled;
  PRPackedBool              mNeedsRebuild;
  PRPackedBool              mConstructed;
  PRPackedBool              mVisible;
  PRPackedBool              mXBLAttached;
};

#endif 

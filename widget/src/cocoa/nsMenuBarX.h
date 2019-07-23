





































#ifndef nsMenuBarX_h_
#define nsMenuBarX_h_

#import <Cocoa/Cocoa.h>

#include "nsMenuBaseX.h"
#include "nsMenuGroupOwnerX.h"
#include "nsChangeObserver.h"
#include "nsINativeMenuService.h"
#include "nsAutoPtr.h"
#include "nsString.h"

class nsMenuX;
class nsMenuItemX;
class nsIWidget;
class nsIContent;
class nsIDocument;


class nsNativeMenuServiceX : public nsINativeMenuService
{
public:
  NS_DECL_ISUPPORTS
  NS_IMETHOD CreateNativeMenuBar(nsIWidget* aParent, nsIContent* aMenuBarNode);
};




@interface GeckoNSMenu : NSMenu
{
}
- (BOOL)performKeyEquivalent:(NSEvent*)theEvent;
- (void)actOnKeyEquivalent:(NSEvent*)theEvent;
- (void)performMenuUserInterfaceEffectsForEvent:(NSEvent*)theEvent;
@end


@interface NativeMenuItemTarget : NSObject
{
}
-(IBAction)menuItemHit:(id)sender;
@end




@interface GeckoServicesNSMenuItem : NSMenuItem
{
}
- (id) target;
- (SEL) action;
- (void) _doNothing:(id)sender;
@end




@interface GeckoServicesNSMenu : NSMenu
{
}
- (void)addItem:(NSMenuItem *)newItem;
- (NSMenuItem *)addItemWithTitle:(NSString *)aString action:(SEL)aSelector keyEquivalent:(NSString *)keyEquiv;
- (void)insertItem:(NSMenuItem *)newItem atIndex:(NSInteger)index;
- (NSMenuItem *)insertItemWithTitle:(NSString *)aString action:(SEL)aSelector  keyEquivalent:(NSString *)keyEquiv atIndex:(NSInteger)index;
- (void) _overrideClassOfMenuItem:(NSMenuItem *)menuItem;
@end



class nsMenuBarX : public nsMenuGroupOwnerX, public nsChangeObserver
{
public:
  nsMenuBarX();
  virtual ~nsMenuBarX();

  static NativeMenuItemTarget* sNativeEventTarget;
  static nsMenuBarX*           sLastGeckoMenuBarPainted;

  
  
  nsCOMPtr<nsIContent> mAboutItemContent;
  nsCOMPtr<nsIContent> mPrefItemContent;
  nsCOMPtr<nsIContent> mQuitItemContent;

  
  NS_DECL_CHANGEOBSERVER

  
  void*             NativeData()     {return (void*)mNativeMenu;}
  nsMenuObjectTypeX MenuObjectType() {return eMenuBarObjectType;}

  
  nsresult          Create(nsIWidget* aParent, nsIContent* aContent);
  void              SetParent(nsIWidget* aParent);
  PRUint32          GetMenuCount();
  bool              MenuContainsAppMenu();
  nsMenuX*          GetMenuAt(PRUint32 aIndex);
  nsMenuX*          GetXULHelpMenu();
  void              SetSystemHelpMenu();
  nsresult          Paint();
  void              ForceUpdateNativeMenuAt(const nsAString& indexString);
  void              ForceNativeMenuReload(); 
  static char       GetLocalizedAccelKey(const char *shortcutID);

protected:
  void              ConstructNativeMenus();
  nsresult          InsertMenuAtIndex(nsMenuX* aMenu, PRUint32 aIndex);
  void              RemoveMenuAtIndex(PRUint32 aIndex);
  void              HideItem(nsIDOMDocument* inDoc, const nsAString & inID, nsIContent** outHiddenNode);
  void              AquifyMenuBar();
  NSMenuItem*       CreateNativeAppMenuItem(nsMenuX* inMenu, const nsAString& nodeID, SEL action,
                                            int tag, NativeMenuItemTarget* target);
  nsresult          CreateApplicationMenu(nsMenuX* inMenu);

  nsTArray< nsAutoPtr<nsMenuX> > mMenuArray;
  nsIWidget*         mParentWindow;        
  GeckoNSMenu*       mNativeMenu;            
};

#endif 

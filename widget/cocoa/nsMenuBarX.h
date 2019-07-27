




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
class nsMenuBarX;
class nsIWidget;
class nsIContent;


class nsNativeMenuServiceX : public nsINativeMenuService
{
public:
  NS_DECL_ISUPPORTS

  nsNativeMenuServiceX() {}

  NS_IMETHOD CreateNativeMenuBar(nsIWidget* aParent, nsIContent* aMenuBarNode) override;

protected:
  virtual ~nsNativeMenuServiceX() {}
};

@interface NSMenu (Undocumented)



- (void)_performActionWithHighlightingForItemAtIndex:(NSInteger)index;
@end



@interface GeckoNSMenu : NSMenu
{
@private
  nsMenuBarX *mMenuBarOwner; 
  bool mDelayResignMainMenu;
}
- (id)initWithTitle:(NSString *)aTitle andMenuBarOwner:(nsMenuBarX *)aMenuBarOwner;
- (void)resetMenuBarOwner;
- (bool)delayResignMainMenu;
- (void)setDelayResignMainMenu:(bool)aShouldDelay;
- (void)delayedPaintMenuBar:(id)unused;
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
  static nsMenuBarX*           sCurrentPaintDelayedMenuBar;

  
  
  nsCOMPtr<nsIContent> mAboutItemContent;
  nsCOMPtr<nsIContent> mPrefItemContent;
  nsCOMPtr<nsIContent> mQuitItemContent;

  
  NS_DECL_CHANGEOBSERVER

  
  void*             NativeData() override {return (void*)mNativeMenu;}
  nsMenuObjectTypeX MenuObjectType() override {return eMenuBarObjectType;}

  
  nsresult          Create(nsIWidget* aParent, nsIContent* aContent);
  void              SetParent(nsIWidget* aParent);
  uint32_t          GetMenuCount();
  bool              MenuContainsAppMenu();
  nsMenuX*          GetMenuAt(uint32_t aIndex);
  nsMenuX*          GetXULHelpMenu();
  void              SetSystemHelpMenu();
  nsresult          Paint(bool aDelayed = false);
  void              PaintMenuBarAfterDelay();
  void              ResetAwaitingDelayedPaint() { mAwaitingDelayedPaint = false; }
  void              ForceUpdateNativeMenuAt(const nsAString& indexString);
  void              ForceNativeMenuReload(); 
  static char       GetLocalizedAccelKey(const char *shortcutID);

protected:
  void              ConstructNativeMenus();
  void              ConstructFallbackNativeMenus();
  nsresult          InsertMenuAtIndex(nsMenuX* aMenu, uint32_t aIndex);
  void              RemoveMenuAtIndex(uint32_t aIndex);
  void              HideItem(nsIDOMDocument* inDoc, const nsAString & inID, nsIContent** outHiddenNode);
  void              AquifyMenuBar();
  NSMenuItem*       CreateNativeAppMenuItem(nsMenuX* inMenu, const nsAString& nodeID, SEL action,
                                            int tag, NativeMenuItemTarget* target);
  nsresult          CreateApplicationMenu(nsMenuX* inMenu);

  nsTArray< nsAutoPtr<nsMenuX> > mMenuArray;
  nsIWidget*         mParentWindow;        
  GeckoNSMenu*       mNativeMenu;            

  bool               mAwaitingDelayedPaint;
};

#endif 

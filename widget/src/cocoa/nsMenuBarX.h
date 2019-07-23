





































#ifndef nsMenuBarX_h_
#define nsMenuBarX_h_

#import <Cocoa/Cocoa.h>

#include "nsMenuBaseX.h"
#include "nsIMutationObserver.h"
#include "nsHashtable.h"
#include "nsINativeMenuService.h"
#include "nsAutoPtr.h"
#include "nsString.h"

class nsMenuX;
class nsMenuItemX;
class nsChangeObserver;
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



class nsMenuBarX : public nsMenuObjectX,
                   public nsIMutationObserver
{
public:
  nsMenuBarX();
  virtual ~nsMenuBarX();

  static NativeMenuItemTarget* sNativeEventTarget;
  static nsMenuBarX*           sLastGeckoMenuBarPainted;

  
  
  nsCOMPtr<nsIContent> mAboutItemContent;
  nsCOMPtr<nsIContent> mPrefItemContent;
  nsCOMPtr<nsIContent> mQuitItemContent;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIMUTATIONOBSERVER

  
  void*             NativeData()     {return (void*)mNativeMenu;}
  nsMenuObjectTypeX MenuObjectType() {return eMenuBarObjectType;}

  
  nsresult          Create(nsIWidget* aParent, nsIContent* aContent);
  void              SetParent(nsIWidget* aParent);
  void              RegisterForContentChanges(nsIContent* aContent, nsChangeObserver* aMenuObject);
  void              UnregisterForContentChanges(nsIContent* aContent);
  PRUint32          RegisterForCommand(nsMenuItemX* aItem);
  void              UnregisterCommand(PRUint32 aCommandID);
  PRUint32          GetMenuCount();
  bool              MenuContainsAppMenu();
  nsMenuX*          GetMenuAt(PRUint32 aIndex);
  nsMenuItemX*      GetMenuItemForCommandID(PRUint32 inCommandID);
  nsresult          Paint();
  void              ForceUpdateNativeMenuAt(const nsAString& indexString);
  void              ForceNativeMenuReload(); 
  static char       GetLocalizedAccelKey(const char *shortcutID);

protected:
  void              ConstructNativeMenus();
  nsresult          InsertMenuAtIndex(nsMenuX* aMenu, PRUint32 aIndex);
  void              RemoveMenuAtIndex(PRUint32 aIndex);
  nsChangeObserver* LookupContentChangeObserver(nsIContent* aContent);
  void              HideItem(nsIDOMDocument* inDoc, const nsAString & inID, nsIContent** outHiddenNode);
  void              AquifyMenuBar();
  NSMenuItem*       CreateNativeAppMenuItem(nsMenuX* inMenu, const nsAString& nodeID, SEL action,
                                            int tag, NativeMenuItemTarget* target);
  nsresult          CreateApplicationMenu(nsMenuX* inMenu);

  nsTArray< nsAutoPtr<nsMenuX> > mMenuArray;
  nsIWidget*         mParentWindow;        
  PRUint32           mCurrentCommandID;    
  nsIDocument*       mDocument;            
  GeckoNSMenu*       mNativeMenu;            
  nsHashtable        mObserverTable;       
};

#endif 

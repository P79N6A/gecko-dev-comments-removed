





































#ifndef nsMenuX_h_
#define nsMenuX_h_

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIMenu.h"
#include "nsChangeObserver.h"
#include "nsMenuBarX.h"

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>


class nsIMenuBar;
class nsMenuX;
class nsMenuItemIconX;




@interface MenuDelegate : NSObject
{
  nsMenuX* mGeckoMenu; 
  EventHandlerRef mEventHandler;
  BOOL mHaveInstalledCarbonEvents;
}
- (id)initWithGeckoMenu:(nsMenuX*)geckoMenu;
@end


class nsMenuX : public nsIMenu,
                public nsChangeObserver
{
public:
    nsMenuX();
    virtual ~nsMenuX();

    NS_DECL_ISUPPORTS
    NS_DECL_CHANGEOBSERVER

    id GetNativeMenuItem();

    
    NS_IMETHOD Create(nsISupports * aParent, const nsAString &aLabel, const nsAString &aAccessKey, 
                      nsMenuBarX* aMenuBar, nsIContent* aNode);
    NS_IMETHOD GetParent(nsISupports *&aParent);
    NS_IMETHOD GetLabel(nsString &aText);
    NS_IMETHOD SetLabel(const nsAString &aText);
    NS_IMETHOD GetAccessKey(nsString &aText);
    NS_IMETHOD SetAccessKey(const nsAString &aText);
    NS_IMETHOD AddItem(nsISupports* aText);
    NS_IMETHOD GetItemCount(PRUint32 &aCount);
    NS_IMETHOD GetItemAt(const PRUint32 aPos, nsISupports *& aMenuItem);
    NS_IMETHOD GetVisibleItemCount(PRUint32 &aCount);
    NS_IMETHOD GetVisibleItemAt(const PRUint32 aPos, nsISupports *& aMenuItem);
    NS_IMETHOD InsertItemAt(const PRUint32 aPos, nsISupports * aMenuItem);
    NS_IMETHOD RemoveItem(const PRUint32 aPos);
    NS_IMETHOD RemoveAll();
    NS_IMETHOD GetNativeData(void** aData);
    NS_IMETHOD SetNativeData(void* aData);
    NS_IMETHOD GetMenuContent(nsIContent ** aMenuNode);
    NS_IMETHOD SetEnabled(PRBool aIsEnabled);
    NS_IMETHOD GetEnabled(PRBool* aIsEnabled);

    NS_IMETHOD ChangeNativeEnabledStatusForMenuItem(nsIMenuItem* aMenuItem, PRBool aEnabled);
    NS_IMETHOD GetMenuRefAndItemIndexForMenuItem(nsISupports* aMenuItem,
                                                 void**       aMenuRef,
                                                 PRUint16*    aMenuItemIndex);
    NS_IMETHOD SetupIcon();
    nsEventStatus MenuSelected(const nsMenuEvent & aMenuEvent); 
    void MenuDeselected(const nsMenuEvent & aMenuEvent); 
    void MenuConstruct(const nsMenuEvent & aMenuEvent, nsIWidget * aParentWindow, void * aMenuNode);
    void MenuDestruct(const nsMenuEvent & aMenuEvent);
    void SetRebuild(PRBool aMenuEvent);

protected:
    
    
    nsresult CountVisibleBefore(PRUint32* outVisibleBefore);

    
    void GetMenuPopupContent(nsIContent** aResult);
    
    
    PRBool OnDestroy();
    PRBool OnCreate();
    PRBool OnDestroyed();
    PRBool OnCreated();

    nsresult AddMenuItem(nsIMenuItem * aMenuItem);
    nsresult AddMenu(nsIMenu * aMenu);

    void LoadMenuItem(nsIContent* inMenuItemContent);  
    void LoadSubMenu(nsIContent* inMenuContent);

    NSMenu* CreateMenuWithGeckoString(nsString& menuTitle);

protected:
    nsString                    mLabel;
    nsCOMArray<nsISupports>     mMenuItemsArray;
    PRUint32                    mVisibleItemsCount;     

    nsISupports*                mParent;                
    nsMenuBarX*                 mMenuBar;               
    nsCOMPtr<nsIContent>        mMenuContent;           
    nsRefPtr<nsMenuItemIconX>   mIcon;

    
    PRInt16                     mMacMenuID;
    NSMenu*                     mMacMenu;               
    MenuDelegate*               mMenuDelegate;          
    NSMenuItem*                 mNativeMenuItem;        
    PRPackedBool                mIsEnabled;
    PRPackedBool                mDestroyHandlerCalled;
    PRPackedBool                mNeedsRebuild;
    PRPackedBool                mConstructed;
    PRPackedBool                mVisible;               
    PRPackedBool                mXBLAttached;
};

#endif 







































#ifndef nsMenuBarX_h_
#define nsMenuBarX_h_

#include "nsIMenuBar.h"
#include "nsIMutationObserver.h"
#include "nsCOMArray.h"
#include "nsHashtable.h"
#include "nsWeakReference.h"
#include "nsIContent.h"

#import  <Carbon/Carbon.h>
#import  <Cocoa/Cocoa.h>

class nsIWidget;
class nsIDocument;
class nsIDOMNode;
class nsChangeObserver;

extern "C" MenuRef _NSGetCarbonMenu(NSMenu* aMenu);

PRBool NodeIsHiddenOrCollapsed(nsIContent* inContent);

namespace MenuHelpersX
{
  nsEventStatus DispatchCommandTo(nsIContent* aTargetContent);
  NSString* CreateTruncatedCocoaLabel(const nsString& itemLabel);
  PRUint8 GeckoModifiersForNodeAttribute(const nsString& modifiersAttribute);
  unsigned int MacModifiersForGeckoModifiers(PRUint8 geckoModifiers);
}



@interface NativeMenuItemTarget : NSObject
{
}
-(IBAction)menuItemHit:(id)sender;
@end






class nsMenuBarX : public nsIMenuBar,
                   public nsIMutationObserver,
                   public nsSupportsWeakReference
{
public:
    nsMenuBarX();
    virtual ~nsMenuBarX();

    
    static NativeMenuItemTarget* sNativeEventTarget;
    
    static NSWindow* sEventTargetWindow;
    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIMUTATIONOBSERVER

    
    NS_IMETHOD Create(nsIWidget * aParent);
    NS_IMETHOD GetParent(nsIWidget *&aParent);
    NS_IMETHOD SetParent(nsIWidget * aParent);
    NS_IMETHOD AddMenu(nsIMenu * aMenu);
    NS_IMETHOD GetMenuCount(PRUint32 &aCount);
    NS_IMETHOD GetMenuAt(const PRUint32 aCount, nsIMenu *& aMenu);
    NS_IMETHOD InsertMenuAt(const PRUint32 aCount, nsIMenu *& aMenu);
    NS_IMETHOD RemoveMenu(const PRUint32 aCount);
    NS_IMETHOD RemoveAll();
    NS_IMETHOD GetNativeData(void*& aData);
    NS_IMETHOD Paint();
    NS_IMETHOD SetNativeData(void* aData);
    NS_IMETHOD MenuConstruct(const nsMenuEvent & aMenuEvent, nsIWidget * aParentWindow, void * aMenuNode);

    PRUint32 RegisterForCommand(nsIMenuItem* aItem);
    void UnregisterCommand(PRUint32 aCommandID);

    void RegisterForContentChanges(nsIContent* aContent, nsChangeObserver* aMenuObject);
    void UnregisterForContentChanges(nsIContent* aContent);
    nsChangeObserver* LookupContentChangeObserver(nsIContent* aContent);

protected:
    
    void AquifyMenuBar();
    void HideItem(nsIDOMDocument* inDoc, const nsAString & inID, nsIContent** outHiddenNode);
    OSStatus InstallCommandEventHandler();

    
    pascal static OSStatus CommandEventHandler(EventHandlerCallRef inHandlerChain, 
                                               EventRef inEvent, void* userData);
    nsEventStatus ExecuteCommand(nsIContent* inDispatchTo);
    
    
    NSMenuItem* CreateNativeAppMenuItem(nsIMenu* inMenu, const nsAString& nodeID, SEL action,
                                                    int tag, NativeMenuItemTarget* target);
    nsresult CreateApplicationMenu(nsIMenu* inMenu);

    nsHashtable             mObserverTable;       

    nsCOMArray<nsIMenu>     mMenusArray;          
    nsCOMPtr<nsIContent>    mMenuBarContent;      
    nsCOMPtr<nsIContent>    mAboutItemContent;    
                                                  
    nsCOMPtr<nsIContent>    mPrefItemContent;     
    nsCOMPtr<nsIContent>    mQuitItemContent;     
    nsIWidget*              mParent;              
    PRBool                  mIsMenuBarAdded;
    PRUint32                mCurrentCommandID;    

    nsIDocument*            mDocument;            

    NSMenu*                 mRootMenu;            
 
    static EventHandlerUPP  sCommandEventHandler; 
};

#endif 

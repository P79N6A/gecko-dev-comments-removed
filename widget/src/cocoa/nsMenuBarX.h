





































#ifndef nsMenuBarX_h_
#define nsMenuBarX_h_

#include "nsIMenuBar.h"
#include "nsIMenuListener.h"
#include "nsIMutationObserver.h"
#include "nsIChangeManager.h"
#include "nsIMenuCommandDispatcher.h"
#include "nsCOMArray.h"
#include "nsHashtable.h"
#include "nsWeakReference.h"
#include "nsIContent.h"

#import  <Carbon/Carbon.h>
#import  <Cocoa/Cocoa.h>

class nsIWidget;
class nsIDocument;
class nsIDOMNode;

extern "C" MenuRef _NSGetCarbonMenu(NSMenu* aMenu);

PRBool NodeIsHiddenOrCollapsed(nsIContent* inContent);

namespace MenuHelpersX
{
  nsEventStatus DispatchCommandTo(nsIWeakReference* aDocShellWeakRef,
                                  nsIContent* aTargetContent);
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
                   public nsIMenuListener,
                   public nsIMutationObserver,
                   public nsIChangeManager,
                   public nsIMenuCommandDispatcher,
                   public nsSupportsWeakReference
{
public:
    nsMenuBarX();
    virtual ~nsMenuBarX();
    
    enum {kApplicationMenuID = 1};
    
    
    static NativeMenuItemTarget* sNativeEventTarget;
    
    static NSWindow* sEventTargetWindow;
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSICHANGEMANAGER
    NS_DECL_NSIMENUCOMMANDDISPATCHER

    
    nsEventStatus MenuItemSelected(const nsMenuEvent & aMenuEvent);
    nsEventStatus MenuSelected(const nsMenuEvent & aMenuEvent);
    nsEventStatus MenuDeselected(const nsMenuEvent & aMenuEvent);
    nsEventStatus MenuConstruct(const nsMenuEvent & aMenuEvent, nsIWidget * aParentWindow, 
                                void * menuNode, void * aDocShell);
    nsEventStatus MenuDestruct(const nsMenuEvent & aMenuEvent);
    nsEventStatus CheckRebuild(PRBool & aMenuEvent);
    nsEventStatus SetRebuild(PRBool aMenuEvent);

    
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
    
protected:

    void GetDocument(nsIDocShell* inDocShell, nsIDocument** outDocument) ;
    void RegisterAsDocumentObserver(nsIDocShell* inDocShell);
    
    
    void AquifyMenuBar();
    void HideItem(nsIDOMDocument* inDoc, const nsAString & inID, nsIContent** outHiddenNode);
    OSStatus InstallCommandEventHandler();

    
    pascal static OSStatus CommandEventHandler(EventHandlerCallRef inHandlerChain, 
                                               EventRef inEvent, void* userData);
    nsEventStatus ExecuteCommand(nsIContent* inDispatchTo);
    
    
    NSMenuItem* nsMenuBarX::CreateNativeAppMenuItem(nsIMenu* inMenu, const nsAString& nodeID, SEL action,
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


    nsWeakPtr               mDocShellWeakRef;     
    nsIDocument*            mDocument;            

    NSMenu*                 mRootMenu;            
 
    static EventHandlerUPP  sCommandEventHandler; 
};

#endif 

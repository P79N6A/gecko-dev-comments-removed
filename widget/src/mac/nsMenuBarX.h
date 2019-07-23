




































#ifndef nsMenuBarX_h__
#define nsMenuBarX_h__

#include "nsIMenuBar.h"
#include "nsIMenuListener.h"
#include "nsIMutationObserver.h"
#include "nsIChangeManager.h"
#include "nsIMenuCommandDispatcher.h"
#include "nsPresContext.h"
#include "nsSupportsArray.h"
#include "nsHashtable.h"
#include "nsWeakReference.h"
#include "nsIContent.h"

#include <Carbon/Carbon.h>

class nsIWidget;
class nsIDocument;
class nsIDOMNode;

namespace MenuHelpersX
{
    
  nsresult DocShellToPresContext ( nsIDocShell* inDocShell, nsPresContext** outContext ) ;
  nsEventStatus DispatchCommandTo(nsIWeakReference* aDocShellWeakRef,
                                  nsIContent* aTargetContent);

}






class nsMenuBarX :  public nsIMenuBar,
                    public nsIMenuListener,
                    public nsIMutationObserver,
                    public nsIChangeManager,
                    public nsIMenuCommandDispatcher,
                    public nsSupportsWeakReference
{
public:
    nsMenuBarX();
    virtual ~nsMenuBarX();

    enum { kAppleMenuID = 1 } ;
    
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

    void GetDocument ( nsIDocShell* inDocShell, nsIDocument** outDocument ) ;
    void RegisterAsDocumentObserver ( nsIDocShell* inDocShell ) ;
    
      
    void AquifyMenuBar ( ) ;
    void HideItem ( nsIDOMDocument* inDoc, const nsAString & inID, nsIContent** outHiddenNode ) ;
    OSStatus InstallCommandEventHandler ( ) ;

      
    pascal static OSStatus CommandEventHandler ( EventHandlerCallRef inHandlerChain, 
                                                  EventRef inEvent, void* userData ) ;
    nsEventStatus ExecuteCommand ( nsIContent* inDispatchTo ) ;
    
      
    nsresult CreateAppleMenu ( nsIMenu* inMenu ) ;

    nsHashtable             mObserverTable;     

    PRUint32                mNumMenus;
    nsSupportsArray         mMenusArray;        
    nsCOMPtr<nsIContent>    mMenuBarContent;    
    nsCOMPtr<nsIContent>    mPrefItemContent;   
                                                
    nsCOMPtr<nsIContent>    mQuitItemContent;   
    nsIWidget*              mParent;            
    PRBool                  mIsMenuBarAdded;
    PRUint32                mCurrentCommandID;  


    nsWeakPtr               mDocShellWeakRef;   
    nsIDocument*            mDocument;          

    MenuRef                 mRootMenu;          
    
    static MenuRef          sAppleMenu;         
 
    static EventHandlerUPP  sCommandEventHandler;   
};

#endif 

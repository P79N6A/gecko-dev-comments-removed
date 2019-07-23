




































#ifndef nsMenuX_h__
#define nsMenuX_h__

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIMenu.h"
#include "nsSupportsArray.h"
#include "nsIMenuListener.h"
#include "nsIChangeManager.h"
#include "nsWeakReference.h"

#include <Carbon/Carbon.h>

class nsIMenuBar;
class nsIMenuListener;
class nsMenuItemIcon;

extern PRInt16 mMacMenuIDCount;

#if DEBUG

class nsInstanceCounter
{
public:
        nsInstanceCounter(const char* inDesc)
        : mInstanceCount(0)
        , mDescription(inDesc)
        {
        }
        
        ~nsInstanceCounter()
        {
          printf("%s %d\n", mDescription, mInstanceCount);
        }

        nsInstanceCounter& operator ++()          
        {
          ++ mInstanceCount;
          return *this;                
        }

        nsInstanceCounter& operator -- ()        
        {
          -- mInstanceCount;
          return *this;
        }

protected:

  PRInt32     mInstanceCount;
  const char* mDescription;

};
#endif

class nsMenuX : public nsIMenu,
                public nsIMenuListener,
                public nsIChangeObserver,
                public nsSupportsWeakReference
{

public:
    nsMenuX();
    virtual ~nsMenuX();

    NS_DECL_ISUPPORTS
    NS_DECL_NSICHANGEOBSERVER

    
    nsEventStatus MenuItemSelected(const nsMenuEvent & aMenuEvent); 
    nsEventStatus MenuSelected(const nsMenuEvent & aMenuEvent); 
    nsEventStatus MenuDeselected(const nsMenuEvent & aMenuEvent); 
    nsEventStatus MenuConstruct( const nsMenuEvent & aMenuEvent, nsIWidget * aParentWindow, 
                                void * menuNode, void * aDocShell);
    nsEventStatus MenuDestruct(const nsMenuEvent & aMenuEvent);
    nsEventStatus CheckRebuild(PRBool & aMenuEvent);
    nsEventStatus SetRebuild(PRBool aMenuEvent);

    
    NS_IMETHOD Create ( nsISupports * aParent, const nsAString &aLabel, const nsAString &aAccessKey, 
                        nsIChangeManager* aManager, nsIDocShell* aShell, nsIContent* aNode ) ;
    NS_IMETHOD GetParent(nsISupports *&aParent);
    NS_IMETHOD GetLabel(nsString &aText);
    NS_IMETHOD SetLabel(const nsAString &aText);
    NS_IMETHOD GetAccessKey(nsString &aText);
    NS_IMETHOD SetAccessKey(const nsAString &aText);
    NS_IMETHOD AddItem(nsISupports* aText);
    NS_IMETHOD AddSeparator();
    NS_IMETHOD GetItemCount(PRUint32 &aCount);
    NS_IMETHOD GetItemAt(const PRUint32 aPos, nsISupports *& aMenuItem);
    NS_IMETHOD InsertItemAt(const PRUint32 aPos, nsISupports * aMenuItem);
    NS_IMETHOD RemoveItem(const PRUint32 aPos);
    NS_IMETHOD RemoveAll();
    NS_IMETHOD GetNativeData(void** aData);
    NS_IMETHOD SetNativeData(void* aData);
    NS_IMETHOD AddMenuListener(nsIMenuListener * aMenuListener);
    NS_IMETHOD RemoveMenuListener(nsIMenuListener * aMenuListener);
    NS_IMETHOD GetMenuContent(nsIContent ** aMenuNode);
    NS_IMETHOD SetEnabled(PRBool aIsEnabled);
    NS_IMETHOD GetEnabled(PRBool* aIsEnabled);

    NS_IMETHOD AddMenuItem(nsIMenuItem * aMenuItem);
    NS_IMETHOD AddMenu(nsIMenu * aMenu);
    NS_IMETHOD ChangeNativeEnabledStatusForMenuItem(nsIMenuItem* aMenuItem, PRBool aEnabled);
    NS_IMETHOD GetMenuRefAndItemIndexForMenuItem(nsISupports* aMenuItem,
                                                 void**       aMenuRef,
                                                 PRUint16*    aMenuItemIndex);
    NS_IMETHOD SetupIcon();

protected:
      
      
    nsresult CountVisibleBefore ( PRUint32* outVisibleBefore ) ;

    
    void GetMenuPopupContent ( nsIContent** aResult ) ;

      
      
    void InsertMenuItemWithTruncation ( nsAutoString & inItemLabel, 
                                          PRUint32 inItemIndex ) ;
    
    
    PRBool OnDestroy() ;
    PRBool OnCreate() ;
    PRBool OnDestroyed() ;
    PRBool OnCreated() ;

    void LoadMenuItem ( nsIMenu* pParentMenu, nsIContent* menuitemContent );  
    void LoadSubMenu ( nsIMenu * pParentMenu, nsIContent* menuitemContent );
    void LoadSeparator ( nsIContent* menuitemContent );

    MenuHandle NSStringNewMenu(short menuID, nsString& menuTitle);

protected:
    nsString                    mLabel;
    PRUint32                    mNumMenuItems;
    nsSupportsArray             mMenuItemsArray;        

    nsISupports*                mParent;                
    nsIChangeManager*           mManager;               
    nsWeakPtr                   mDocShellWeakRef;       
    nsCOMPtr<nsIContent>        mMenuContent;           
    nsCOMPtr<nsIMenuListener>   mListener;              
    nsRefPtr<nsMenuItemIcon>    mIcon;

    
    PRInt16                     mMacMenuID;
    MenuHandle                  mMacMenuHandle;
    PRPackedBool                mIsEnabled;
    PRPackedBool                mDestroyHandlerCalled;
    PRPackedBool                mNeedsRebuild;
    PRPackedBool                mConstructed;
    PRPackedBool                mVisible;               
    
    EventHandlerRef             mHandler;               
};

#endif 

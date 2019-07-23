




































#ifndef nsMenuItemX_h__
#define nsMenuItemX_h__


#include "nsIMenuItem.h"
#include "nsString.h"
#include "nsIMenuListener.h"
#include "nsIChangeManager.h"
#include "nsWeakReference.h"
#include "nsIWidget.h"
#include "nsAutoPtr.h"

class nsIMenu;
class nsMenuItemIcon;





class nsMenuItemX : public nsIMenuItem,
                    public nsIMenuListener,
                    public nsIChangeObserver,
                    public nsSupportsWeakReference
{
public:
  nsMenuItemX();
  virtual ~nsMenuItemX();

  
  NS_DECL_ISUPPORTS
  NS_DECL_NSICHANGEOBSERVER

  
  NS_IMETHOD Create(nsIMenu* aParent, const nsString & aLabel, PRBool aIsSeparator,
                    EMenuItemType aItemType, nsIChangeManager* aManager,
                    nsIDocShell* aShell, nsIContent* aNode);
  NS_IMETHOD GetLabel(nsString &aText);
  NS_IMETHOD SetShortcutChar(const nsString &aText);
  NS_IMETHOD GetShortcutChar(nsString &aText);
  NS_IMETHOD GetEnabled(PRBool *aIsEnabled);
  NS_IMETHOD SetChecked(PRBool aIsEnabled);
  NS_IMETHOD GetChecked(PRBool *aIsEnabled);
  NS_IMETHOD GetMenuItemType(EMenuItemType *aIsCheckbox);
  NS_IMETHOD GetNativeData(void*& aData);
  NS_IMETHOD AddMenuListener(nsIMenuListener * aMenuListener);
  NS_IMETHOD RemoveMenuListener(nsIMenuListener * aMenuListener);
  NS_IMETHOD IsSeparator(PRBool & aIsSep);

  NS_IMETHOD DoCommand();
  NS_IMETHOD DispatchDOMEvent(const nsString &eventName, PRBool *preventDefaultCalled);
  NS_IMETHOD SetModifiers(PRUint8 aModifiers);
  NS_IMETHOD GetModifiers(PRUint8 * aModifiers);
  NS_IMETHOD SetupIcon();
    
  
  nsEventStatus MenuItemSelected(const nsMenuEvent & aMenuEvent);
  nsEventStatus MenuSelected(const nsMenuEvent & aMenuEvent);
  nsEventStatus MenuDeselected(const nsMenuEvent & aMenuEvent);
  nsEventStatus MenuConstruct(const nsMenuEvent & aMenuEvent, nsIWidget * aParentWindow, 
                                void * menuNode, void * aDocShell);
  nsEventStatus MenuDestruct(const nsMenuEvent & aMenuEvent);
  nsEventStatus CheckRebuild(PRBool & aMenuEvent);
  nsEventStatus SetRebuild(PRBool aMenuEvent);

protected:

  void UncheckRadioSiblings ( nsIContent* inCheckedElement ) ;

  nsString        mLabel;
  nsString        mKeyEquivalent;

  nsIMenu*                  mMenuParent;          
  nsIChangeManager*         mManager;             

  nsCOMPtr<nsIMenuListener> mXULCommandListener;
  
  nsWeakPtr                 mDocShellWeakRef;     
  nsCOMPtr<nsIContent>      mContent;
  nsCOMPtr<nsIContent>      mCommandContent;
  nsRefPtr<nsMenuItemIcon>  mIcon;
  
  PRUint8           mModifiers;
  PRPackedBool      mIsSeparator;
  PRPackedBool      mEnabled;
  PRPackedBool      mIsChecked;
  EMenuItemType     mMenuType;
};

#endif 







































#ifndef nsMenuItemX_h_
#define nsMenuItemX_h_

#include "nsIMenuItem.h"
#include "nsString.h"
#include "nsChangeObserver.h"
#include "nsAutoPtr.h"

#import <Cocoa/Cocoa.h>

class nsIMenu;
class nsMenuItemIconX;





class nsMenuItemX : public nsIMenuItem,
                    public nsChangeObserver
{
public:
  nsMenuItemX();
  virtual ~nsMenuItemX();

  
  NS_DECL_ISUPPORTS
  NS_DECL_CHANGEOBSERVER

  
  NS_IMETHOD Create(nsIMenu* aParent, const nsString & aLabel, EMenuItemType aItemType,
                    nsMenuBarX* aMenuBar, nsIContent* aNode);
  NS_IMETHOD GetLabel(nsString &aText);
  NS_IMETHOD SetShortcutChar(const nsString &aText);
  NS_IMETHOD GetShortcutChar(nsString &aText);
  NS_IMETHOD GetEnabled(PRBool *aIsEnabled);
  NS_IMETHOD SetChecked(PRBool aIsEnabled);
  NS_IMETHOD GetChecked(PRBool *aIsEnabled);
  NS_IMETHOD GetMenuItemType(EMenuItemType *aIsCheckbox);
  NS_IMETHOD GetNativeData(void*& aData);
  NS_IMETHOD IsSeparator(PRBool & aIsSep);

  NS_IMETHOD DoCommand();
  NS_IMETHOD DispatchDOMEvent(const nsString &eventName, PRBool *preventDefaultCalled);
  NS_IMETHOD SetModifiers(PRUint8 aModifiers);
  NS_IMETHOD GetModifiers(PRUint8 * aModifiers);
  NS_IMETHOD SetupIcon();
  NS_IMETHOD GetMenuItemContent(nsIContent ** aMenuItemContent);

protected:

  void UncheckRadioSiblings(nsIContent* inCheckedElement);

  NSMenuItem*               mNativeMenuItem;       
  
  nsString                  mLabel;
  nsString                  mKeyEquivalent;

  nsIMenu*                  mMenuParent;          
  nsMenuBarX*               mMenuBar;             
  
  nsCOMPtr<nsIContent>      mContent;
  nsCOMPtr<nsIContent>      mCommandContent;
  nsRefPtr<nsMenuItemIconX> mIcon;
  
  PRUint8           mModifiers;
  PRPackedBool      mEnabled;
  PRPackedBool      mIsChecked;
  EMenuItemType     mType; 
};

#endif 

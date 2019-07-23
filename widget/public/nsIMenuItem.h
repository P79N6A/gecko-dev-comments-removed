




































#ifndef nsIMenuItem_h__
#define nsIMenuItem_h__

#include "prtypes.h"
#include "nsISupports.h"
#include "nsString.h"

#include "nsIDocShell.h"
#include "nsIDOMElement.h"


#define NS_IMENUITEM_IID \
{ 0xF9A30AA5, 0xD526, 0x4C19, \
  { 0x84, 0x18, 0xC2, 0x1B, 0xF6, 0xB3, 0x18, 0x37 } }

class nsIMenu;
class nsIWidget;
class nsIMenuListener;
class nsIChangeManager;
class nsIContent;

enum {
  knsMenuItemNoModifier      = 0,
  knsMenuItemShiftModifier   = (1 << 0),
  knsMenuItemAltModifier     = (1 << 1),
  knsMenuItemControlModifier = (1 << 2),
  knsMenuItemCommandModifier = (1 << 3)
};




class nsIMenuItem : public nsISupports {

  public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMENUITEM_IID)

    enum EMenuItemType { eRegular = 0, eCheckbox, eRadio } ;

   



    NS_IMETHOD Create(nsIMenu* aParent, const nsString & aLabel, PRBool isSeparator, 
                      EMenuItemType aItemType, nsIChangeManager* aManager,
                      nsIDocShell* aShell, nsIContent* aNode) = 0;
    
   



    NS_IMETHOD GetLabel(nsString &aText) = 0;

   



    NS_IMETHOD SetShortcutChar(const nsString &aText) = 0;
  
    



    NS_IMETHOD GetShortcutChar(nsString &aText) = 0;

   



    NS_IMETHOD GetEnabled(PRBool *aIsEnabled) = 0;

   



    NS_IMETHOD SetChecked(PRBool aIsEnabled) = 0;

   



    NS_IMETHOD GetChecked(PRBool *aIsEnabled) = 0;

   



    NS_IMETHOD GetMenuItemType(EMenuItemType *aType) = 0;
    
   



    NS_IMETHOD GetNativeData(void*& aData) = 0;

   



    NS_IMETHOD AddMenuListener(nsIMenuListener * aMenuListener) = 0;

   



    NS_IMETHOD RemoveMenuListener(nsIMenuListener * aMenuListener) = 0;

   



    NS_IMETHOD IsSeparator(PRBool & aIsSep) = 0;

   



    NS_IMETHOD DoCommand() = 0;
    
    



    NS_IMETHOD DispatchDOMEvent(const nsString &eventName, PRBool *preventDefaultCalled) = 0;

    


    NS_IMETHOD SetModifiers(PRUint8 aModifiers) = 0;
    NS_IMETHOD GetModifiers(PRUint8 * aModifiers) = 0;

   


    NS_IMETHOD SetupIcon() = 0;

    



    NS_IMETHOD GetMenuItemContent(nsIContent ** aMenuItemContent) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMenuItem, NS_IMENUITEM_IID)

#endif

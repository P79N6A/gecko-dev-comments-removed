




































#ifndef nsIMenuItem_h__
#define nsIMenuItem_h__

#include "prtypes.h"
#include "nsISupports.h"
#include "nsString.h"

#include "nsIDOMElement.h"


#define NS_IMENUITEM_IID \
{ 0x7DF81BE2, 0x51F4, 0x4CAA, \
  { 0x9F, 0xD7, 0x3F, 0x97, 0x4A, 0x7A, 0xEA, 0x51 } }

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

    enum EMenuItemType { eRegular = 0, eCheckbox, eRadio, eSeparator} ;

   



    NS_IMETHOD Create(nsIMenu* aParent, const nsString & aLabel, EMenuItemType aItemType,
                      nsIChangeManager* aManager, nsIContent* aNode) = 0;

   



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

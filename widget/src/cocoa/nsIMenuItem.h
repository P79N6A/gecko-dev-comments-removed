




































#ifndef nsIMenuItem_h__
#define nsIMenuItem_h__

#include "prtypes.h"
#include "nsISupports.h"
#include "nsString.h"

#include "nsIDOMElement.h"


#define NS_IMENUITEM_IID \
{ 0xCC986E81, 0x9F46, 0x4AA2, \
  { 0xB8, 0x09, 0xC5, 0x44, 0x78, 0x9E, 0x6F, 0x06 } }

class nsIMenu;
class nsIWidget;
class nsIContent;
class nsMenuBarX;

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
                      nsMenuBarX* aMenuBar, nsIContent* aNode) = 0;

   



    NS_IMETHOD GetLabel(nsString &aText) = 0;

   



    NS_IMETHOD SetShortcutChar(const nsString &aText) = 0;
  
    



    NS_IMETHOD GetShortcutChar(nsString &aText) = 0;

   



    NS_IMETHOD GetEnabled(PRBool *aIsEnabled) = 0;

   



    NS_IMETHOD SetChecked(PRBool aIsEnabled) = 0;

   



    NS_IMETHOD GetChecked(PRBool *aIsEnabled) = 0;

   



    NS_IMETHOD GetMenuItemType(EMenuItemType *aType) = 0;
    
   



    NS_IMETHOD GetNativeData(void*& aData) = 0;

   



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

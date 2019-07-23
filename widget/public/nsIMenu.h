




































#ifndef nsIMenu_h__
#define nsIMenu_h__

#include "nsISupports.h"
#include "nsStringFwd.h"

class nsIDocShell;
class nsIMenuBar;
class nsIMenu;
class nsIMenuItem;
class nsIMenuListener;
class nsIChangeManager;
class nsIContent;
class nsIMenuCommandDispatcher;



#define NS_IMENU_IID \
{ 0xFC5BCA9C, 0x4494, 0x4C0F, \
  { 0xBE, 0xFD, 0xCB, 0x31, 0xBE, 0xBA, 0x15, 0x31 } }





class nsIMenu : public nsISupports {

  public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMENU_IID)

  



    NS_IMETHOD Create ( nsISupports * aParent, const nsAString &aLabel, const nsAString &aAccessKey, 
                          nsIChangeManager* aManager, nsIDocShell* aShell, nsIContent* aNode ) = 0;

   



    NS_IMETHOD GetParent(nsISupports *&aParent) = 0;

   



    NS_IMETHOD GetLabel(nsString &aText) = 0;

   



    NS_IMETHOD SetLabel(const nsAString &aText) = 0;

	



	NS_IMETHOD GetAccessKey(nsString &aText) = 0;
   
	



	NS_IMETHOD SetAccessKey(const nsAString &aText) = 0;

	



	NS_IMETHOD SetEnabled(PRBool aIsEnabled) = 0;

	



	NS_IMETHOD GetEnabled(PRBool* aIsEnabled) = 0;
	
	



    NS_IMETHOD AddItem(nsISupports* aItem) = 0;

   



    NS_IMETHOD AddSeparator() = 0;

   



    NS_IMETHOD GetItemCount(PRUint32 &aCount) = 0;

   



    NS_IMETHOD GetItemAt(const PRUint32 aPos, nsISupports *& aMenuItem) = 0;

   



    NS_IMETHOD InsertItemAt(const PRUint32 aPos, nsISupports * aMenuItem) = 0;

   



    NS_IMETHOD RemoveItem(const PRUint32 aPos) = 0;

   



    NS_IMETHOD RemoveAll() = 0;

   



    NS_IMETHOD  GetNativeData(void** aData) = 0;

   



    NS_IMETHOD  SetNativeData(void* aData) = 0;
    
   



    NS_IMETHOD AddMenuListener(nsIMenuListener * aMenuListener) = 0;

   



    NS_IMETHOD RemoveMenuListener(nsIMenuListener * aMenuListener) = 0;

   



    NS_IMETHOD GetMenuContent(nsIContent ** aMenuContent) = 0;
    
   



    NS_IMETHOD ChangeNativeEnabledStatusForMenuItem(nsIMenuItem* aMenuItem,
                                                    PRBool aEnabled) = 0;

   



    NS_IMETHOD GetMenuRefAndItemIndexForMenuItem(nsISupports* aMenuItem,
                                                 void**       aMenuRef,
                                                 PRUint16*    aMenuItemIndex) = 0;

   


    NS_IMETHOD SetupIcon() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMenu, NS_IMENU_IID)

#endif

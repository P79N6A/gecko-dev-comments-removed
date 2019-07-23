




































#ifndef nsIMenu_h__
#define nsIMenu_h__

#include "nsISupports.h"
#include "nsStringFwd.h"
#include "nsEvent.h"

class nsIMenuBar;
class nsIMenu;
class nsIMenuItem;
class nsIContent;
class nsIWidget;
class nsMenuBarX;



#define NS_IMENU_IID \
{ 0x9225136B, 0x3F56, 0x4CA3, \
  { 0x92, 0xE0, 0x62, 0x3D, 0x5F, 0xB8, 0x35, 0x6B } }




class nsIMenu : public nsISupports {

  public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMENU_IID)

  



    NS_IMETHOD Create(nsISupports * aParent, const nsAString &aLabel, const nsAString &aAccessKey, 
                      nsMenuBarX* aMenuBar, nsIContent* aNode) = 0;

  



    NS_IMETHOD GetParent(nsISupports *&aParent) = 0;

  



    NS_IMETHOD GetLabel(nsString &aText) = 0;

  



    NS_IMETHOD SetLabel(const nsAString &aText) = 0;

  



  NS_IMETHOD GetAccessKey(nsString &aText) = 0;
   
  



  NS_IMETHOD SetAccessKey(const nsAString &aText) = 0;

  



  NS_IMETHOD SetEnabled(PRBool aIsEnabled) = 0;

  



  NS_IMETHOD GetEnabled(PRBool* aIsEnabled) = 0;
  
  




    NS_IMETHOD AddItem(nsISupports* aItem) = 0;

   




    NS_IMETHOD GetVisibleItemCount(PRUint32 &aCount) = 0;

   




    NS_IMETHOD GetVisibleItemAt(const PRUint32 aPos, nsISupports *& aMenuItem) = 0;

   




    NS_IMETHOD GetItemCount(PRUint32 &aCount) = 0;

   




    NS_IMETHOD GetItemAt(const PRUint32 aPos, nsISupports *& aMenuItem) = 0;

   



    NS_IMETHOD InsertItemAt(const PRUint32 aPos, nsISupports * aMenuItem) = 0;

   



    NS_IMETHOD RemoveItem(const PRUint32 aPos) = 0;

   



    NS_IMETHOD RemoveAll() = 0;

   



    NS_IMETHOD  GetNativeData(void** aData) = 0;

   



    NS_IMETHOD  SetNativeData(void* aData) = 0;

   



    NS_IMETHOD GetMenuContent(nsIContent ** aMenuContent) = 0;
    
   



    NS_IMETHOD ChangeNativeEnabledStatusForMenuItem(nsIMenuItem* aMenuItem,
                                                    PRBool aEnabled) = 0;

   



    NS_IMETHOD GetMenuRefAndItemIndexForMenuItem(nsISupports* aMenuItem,
                                                 void**       aMenuRef,
                                                 PRUint16*    aMenuItemIndex) = 0;

   



    NS_IMETHOD SetupIcon() = 0;
    
   



    virtual nsEventStatus MenuSelected(const nsMenuEvent & aMenuEvent) = 0;
    
   



    virtual void MenuDeselected(const nsMenuEvent & aMenuEvent) = 0;

   



    virtual void MenuConstruct(const nsMenuEvent & aMenuEvent, nsIWidget * aParentWindow, void * aMenuNode) = 0;

   



    virtual void MenuDestruct(const nsMenuEvent & aMenuEvent) = 0;
    
   



    virtual void SetRebuild(PRBool aMenuEvent) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMenu, NS_IMENU_IID)

#endif

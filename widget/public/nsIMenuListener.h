




































#ifndef nsIMenuListener_h__
#define nsIMenuListener_h__

#include "nsISupports.h"
#include "nsEvent.h"

class nsIWidget;


#define NS_IMENULISTENER_IID      \
{ 0xF463E22A, 0xC5A9, 0x4443, \
  { 0x94, 0x07, 0x2A, 0x7C, 0xD6, 0x63, 0x4A, 0xE1 } }








class nsIMenuListener : public nsISupports {

  public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMENULISTENER_IID)

	 




    virtual nsEventStatus MenuItemSelected(const nsMenuEvent & aMenuEvent) = 0;

    




    virtual nsEventStatus MenuSelected(const nsMenuEvent & aMenuEvent) = 0;

    




    virtual nsEventStatus MenuDeselected(const nsMenuEvent & aMenuEvent) = 0;

    virtual nsEventStatus MenuConstruct(const nsMenuEvent & aMenuEvent,
                                        nsIWidget* aParentWindow, void* aNode) = 0;

    virtual nsEventStatus MenuDestruct(const nsMenuEvent & aMenuEvent) = 0;
    
    virtual nsEventStatus CheckRebuild(PRBool & aMenuEvent) = 0;
    virtual nsEventStatus SetRebuild(PRBool aMenuEvent) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMenuListener, NS_IMENULISTENER_IID)

#endif 






































#ifndef nsIMenuListener_h__
#define nsIMenuListener_h__

#include "nsISupports.h"
#include "nsEvent.h"

class nsIWidget;


#define NS_IMENULISTENER_IID      \
{ 0xf2e79602, 0x1700, 0x11d5, \
  { 0xbb, 0x6f, 0x90, 0xf2, 0x40, 0xfe, 0x49, 0x3c } }








class nsIMenuListener : public nsISupports {

  public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMENULISTENER_IID)

	 




    virtual nsEventStatus MenuItemSelected(const nsMenuEvent & aMenuEvent) = 0;

    




    virtual nsEventStatus MenuSelected(const nsMenuEvent & aMenuEvent) = 0;

    




    virtual nsEventStatus MenuDeselected(const nsMenuEvent & aMenuEvent) = 0;

    virtual nsEventStatus MenuConstruct( const nsMenuEvent & aMenuEvent,
                                          nsIWidget* aParentWindow, void* aNode,
                                          void* aDocShell) = 0;

    virtual nsEventStatus MenuDestruct(const nsMenuEvent & aMenuEvent) = 0;
    
    virtual nsEventStatus CheckRebuild(PRBool & aMenuEvent) = 0;
    virtual nsEventStatus SetRebuild(PRBool aMenuEvent) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMenuListener, NS_IMENULISTENER_IID)

#endif 

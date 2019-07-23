





































#ifndef nsIMouseListener_h__
#define nsIMouseListener_h__

#include "nsEvent.h"
#include "nsISupports.h"








#define NS_IMOUSELISTENER_IID \
{ 0xc83f6b81, 0xd7ce, 0x11d2, { 0x83, 0x60, 0xc4, 0xc8, 0x94, 0xc4, 0x91, 0x7c } }

class nsIMouseListener : public nsISupports {
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMOUSELISTENER_IID)

    




    virtual nsEventStatus MousePressed(const nsGUIEvent & aMouseEvent) = 0;

    




    virtual nsEventStatus MouseReleased(const nsGUIEvent & aMouseEvent) = 0;

    





    virtual nsEventStatus MouseClicked(const nsGUIEvent & aMouseEvent) = 0;

    




    virtual nsEventStatus MouseMoved(const nsGUIEvent & aMouseEvent) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMouseListener, NS_IMOUSELISTENER_IID)

#endif 

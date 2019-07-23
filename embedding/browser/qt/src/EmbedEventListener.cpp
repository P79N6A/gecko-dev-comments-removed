





































#include "EmbedEventListener.h"

#include <nsCOMPtr.h>
#include <nsIDOMMouseEvent.h>

#include "nsIDOMKeyEvent.h"
#include "nsIDOMUIEvent.h"

#include "EmbedEventListener.h"
#include "qgeckoembed.h"

EmbedEventListener::EmbedEventListener(QGeckoEmbed *aOwner)
{
    mOwner = aOwner;
}

EmbedEventListener::~EmbedEventListener()
{

}

NS_IMPL_ADDREF(EmbedEventListener)
NS_IMPL_RELEASE(EmbedEventListener)
NS_INTERFACE_MAP_BEGIN(EmbedEventListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMKeyListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEventListener, nsIDOMKeyListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMKeyListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMUIListener)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
EmbedEventListener::HandleEvent(nsIDOMEvent* aEvent)
{
    return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::KeyDown(nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr <nsIDOMKeyEvent> keyEvent;
    keyEvent = do_QueryInterface(aDOMEvent);
    if (!keyEvent)
        return NS_OK;

    
    
    bool returnVal = mOwner->domKeyDownEvent(keyEvent);

    if (returnVal) {
        aDOMEvent->StopPropagation();
        aDOMEvent->PreventDefault();
    }
    return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::KeyUp(nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr <nsIDOMKeyEvent> keyEvent;
    keyEvent = do_QueryInterface(aDOMEvent);
    if (!keyEvent)
        return NS_OK;
    
    

    bool returnVal = mOwner->domKeyUpEvent(keyEvent);
    if (returnVal) {
        aDOMEvent->StopPropagation();
        aDOMEvent->PreventDefault();
    }

    return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::KeyPress(nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr <nsIDOMKeyEvent> keyEvent;
    keyEvent = do_QueryInterface(aDOMEvent);
    if (!keyEvent)
        return NS_OK;

    
    
    bool returnVal = mOwner->domKeyPressEvent(keyEvent);
    if (returnVal) {
        aDOMEvent->StopPropagation();
        aDOMEvent->PreventDefault();
    }
    return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::MouseDown(nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr <nsIDOMMouseEvent> mouseEvent;
    mouseEvent = do_QueryInterface(aDOMEvent);
    if (!mouseEvent)
        return NS_OK;
    
    

    bool returnVal = mOwner->domMouseDownEvent(mouseEvent);
    if (returnVal) {
        aDOMEvent->StopPropagation();
        aDOMEvent->PreventDefault();
    }

    return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::MouseUp(nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr <nsIDOMMouseEvent> mouseEvent;
    mouseEvent = do_QueryInterface(aDOMEvent);
    if (!mouseEvent)
        return NS_OK;
    
    

    bool returnVal = mOwner->domMouseUpEvent(mouseEvent);
    if (returnVal) {
        aDOMEvent->StopPropagation();
        aDOMEvent->PreventDefault();
    }
    return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::MouseClick(nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr <nsIDOMMouseEvent> mouseEvent;
    mouseEvent = do_QueryInterface(aDOMEvent);
    if (!mouseEvent)
        return NS_OK;
    
    
    bool returnVal = mOwner->domMouseClickEvent(mouseEvent);
    if (returnVal) {
        aDOMEvent->StopPropagation();
        aDOMEvent->PreventDefault();
    }
    return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::MouseDblClick(nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr <nsIDOMMouseEvent> mouseEvent;
    mouseEvent = do_QueryInterface(aDOMEvent);
    if (!mouseEvent)
        return NS_OK;
    
    
    bool returnVal = mOwner->domMouseDblClickEvent(mouseEvent);
    if (returnVal) {
        aDOMEvent->StopPropagation();
        aDOMEvent->PreventDefault();
    }
    return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::MouseOver(nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr <nsIDOMMouseEvent> mouseEvent;
    mouseEvent = do_QueryInterface(aDOMEvent);
    if (!mouseEvent)
        return NS_OK;
    
    
    bool returnVal = mOwner->domMouseOverEvent(mouseEvent);
    if (returnVal) {
        aDOMEvent->StopPropagation();
        aDOMEvent->PreventDefault();
    }
    return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::MouseOut(nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr <nsIDOMMouseEvent> mouseEvent;
    mouseEvent = do_QueryInterface(aDOMEvent);
    if (!mouseEvent)
        return NS_OK;
    
    
    bool returnVal = mOwner->domMouseOutEvent(mouseEvent);
    if (returnVal) {
        aDOMEvent->StopPropagation();
        aDOMEvent->PreventDefault();
    }
    return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::Activate(nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr <nsIDOMUIEvent> uiEvent = do_QueryInterface(aDOMEvent);
    if (!uiEvent)
        return NS_OK;
    
    

    return mOwner->domActivateEvent(uiEvent);
}

NS_IMETHODIMP
EmbedEventListener::FocusIn(nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr <nsIDOMUIEvent> uiEvent = do_QueryInterface(aDOMEvent);
    if (!uiEvent)
        return NS_OK;
    
    

    return mOwner->domFocusInEvent(uiEvent);
}

NS_IMETHODIMP
EmbedEventListener::FocusOut(nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr <nsIDOMUIEvent> uiEvent = do_QueryInterface(aDOMEvent);
    if (!uiEvent)
        return NS_OK;
    
    

    return mOwner->domFocusOutEvent(uiEvent);
}

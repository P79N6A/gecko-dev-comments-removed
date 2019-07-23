




































#include "nsCOMPtr.h"
#include "nsIDOMMouseEvent.h"

#include "nsIDOMKeyEvent.h"
#include "nsIDOMUIEvent.h"

#include "EmbedEventListener.h"
#include "EmbedPrivate.h"

EmbedEventListener::EmbedEventListener(void)
{
  mOwner = nsnull;
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

nsresult
EmbedEventListener::Init(EmbedPrivate *aOwner)
{
  mOwner = aOwner;
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::HandleEvent(nsIDOMEvent* aDOMEvent)
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
  
  
  gint return_val = FALSE;
  g_signal_emit(G_OBJECT(mOwner->mOwningWidget),
                moz_embed_signals[DOM_KEY_DOWN], 0,
                (void *)keyEvent, &return_val);
  if (return_val) {
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
  
  
  gint return_val = FALSE;
  g_signal_emit(G_OBJECT(mOwner->mOwningWidget),
                moz_embed_signals[DOM_KEY_UP], 0,
                (void *)keyEvent, &return_val);
  if (return_val) {
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
  
  gint return_val = FALSE;
  g_signal_emit(G_OBJECT(mOwner->mOwningWidget),
                moz_embed_signals[DOM_KEY_PRESS], 0,
                (void *)keyEvent, &return_val);
  if (return_val) {
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
  
  gint return_val = FALSE;
  g_signal_emit(G_OBJECT(mOwner->mOwningWidget),
                moz_embed_signals[DOM_MOUSE_DOWN], 0,
                (void *)mouseEvent, &return_val);
  if (return_val) {
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
  
  gint return_val = FALSE;
  g_signal_emit(G_OBJECT(mOwner->mOwningWidget),
                moz_embed_signals[DOM_MOUSE_UP], 0,
                (void *)mouseEvent, &return_val);
  if (return_val) {
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
  
  gint return_val = FALSE;
  g_signal_emit(G_OBJECT(mOwner->mOwningWidget),
                moz_embed_signals[DOM_MOUSE_CLICK], 0,
                (void *)mouseEvent, &return_val);
  if (return_val) {
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
  
  gint return_val = FALSE;
  g_signal_emit(G_OBJECT(mOwner->mOwningWidget),
                moz_embed_signals[DOM_MOUSE_DBL_CLICK], 0,
                (void *)mouseEvent, &return_val);
  if (return_val) {
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
  
  gint return_val = FALSE;
  g_signal_emit(G_OBJECT(mOwner->mOwningWidget),
                moz_embed_signals[DOM_MOUSE_OVER], 0,
                (void *)mouseEvent, &return_val);
  if (return_val) {
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
  
  gint return_val = FALSE;
  g_signal_emit(G_OBJECT(mOwner->mOwningWidget),
                moz_embed_signals[DOM_MOUSE_OUT], 0,
                (void *)mouseEvent, &return_val);
  if (return_val) {
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
  
  gint return_val = FALSE;
  g_signal_emit(G_OBJECT(mOwner->mOwningWidget),
                moz_embed_signals[DOM_ACTIVATE], 0,
                (void *)uiEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::FocusIn(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMUIEvent> uiEvent = do_QueryInterface(aDOMEvent);
  if (!uiEvent)
    return NS_OK;
  
  gint return_val = FALSE;
  g_signal_emit(G_OBJECT(mOwner->mOwningWidget),
                moz_embed_signals[DOM_FOCUS_IN], 0,
                (void *)uiEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::FocusOut(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMUIEvent> uiEvent = do_QueryInterface(aDOMEvent);
  if (!uiEvent)
    return NS_OK;
  
  gint return_val = FALSE;
  g_signal_emit(G_OBJECT(mOwner->mOwningWidget),
                moz_embed_signals[DOM_FOCUS_OUT], 0,
                (void *)uiEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  }
  return NS_OK;
}

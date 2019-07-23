




































#include "nsIDocShell.h"
#include "nsPresContext.h"
#include "nsDOMClassInfo.h"
#include "nsDOMError.h"
#include "nsIDOMNSEvent.h"

#include "nsDOMWindowUtils.h"
#include "nsGlobalWindow.h"
#include "nsIDocument.h"
#include "nsIFocusController.h"
#include "nsIEventStateManager.h"

#include "nsContentUtils.h"

#include "nsIFrame.h"
#include "nsIWidget.h"
#include "nsGUIEvent.h"
#include "nsIParser.h"
#include "nsJSEnvironment.h"

#if defined(MOZ_X11) && defined(MOZ_WIDGET_GTK2)
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#endif

NS_INTERFACE_MAP_BEGIN(nsDOMWindowUtils)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMWindowUtils)
  NS_INTERFACE_MAP_ENTRY(nsIDOMWindowUtils)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(WindowUtils)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsDOMWindowUtils)
NS_IMPL_RELEASE(nsDOMWindowUtils)

nsDOMWindowUtils::nsDOMWindowUtils(nsGlobalWindow *aWindow)
  : mWindow(aWindow)
{
}

nsDOMWindowUtils::~nsDOMWindowUtils()
{
}

NS_IMETHODIMP
nsDOMWindowUtils::GetImageAnimationMode(PRUint16 *aMode)
{
  NS_ENSURE_ARG_POINTER(aMode);
  *aMode = 0;
  if (mWindow) {
    nsIDocShell *docShell = mWindow->GetDocShell();
    if (docShell) {
      nsCOMPtr<nsPresContext> presContext;
      docShell->GetPresContext(getter_AddRefs(presContext));
      if (presContext) {
        *aMode = presContext->ImageAnimationMode();
        return NS_OK;
      }
    }
  }
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsDOMWindowUtils::SetImageAnimationMode(PRUint16 aMode)
{
  if (mWindow) {
    nsIDocShell *docShell = mWindow->GetDocShell();
    if (docShell) {
      nsCOMPtr<nsPresContext> presContext;
      docShell->GetPresContext(getter_AddRefs(presContext));
      if (presContext) {
        presContext->SetImageAnimationMode(aMode);
        return NS_OK;
      }
    }
  }
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsDOMWindowUtils::GetDocCharsetIsForced(PRBool *aIsForced)
{
  *aIsForced = PR_FALSE;

  PRBool hasCap = PR_FALSE;
  if (NS_FAILED(nsContentUtils::GetSecurityManager()->IsCapabilityEnabled("UniversalXPConnect", &hasCap)) || !hasCap)
    return NS_ERROR_DOM_SECURITY_ERR;

  if (mWindow) {
    nsCOMPtr<nsIDocument> doc(do_QueryInterface(mWindow->GetExtantDocument()));
    *aIsForced = doc &&
      doc->GetDocumentCharacterSetSource() >= kCharsetFromParentForced;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWindowUtils::GetDocumentMetadata(const nsAString& aName,
                                      nsAString& aValue)
{
  PRBool hasCap = PR_FALSE;
  if (NS_FAILED(nsContentUtils::GetSecurityManager()->IsCapabilityEnabled("UniversalXPConnect", &hasCap))
      || !hasCap)
    return NS_ERROR_DOM_SECURITY_ERR;

  if (mWindow) {
    nsCOMPtr<nsIDocument> doc(do_QueryInterface(mWindow->GetExtantDocument()));
    if (doc) {
      nsCOMPtr<nsIAtom> name = do_GetAtom(aName);
      doc->GetHeaderData(name, aValue);
      return NS_OK;
    }
  }
  
  aValue.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWindowUtils::Redraw(PRUint32 aCount, PRUint32 *aDurationOut)
{
  nsresult rv;

  if (aCount == 0)
    aCount = 1;

  nsCOMPtr<nsIDocShell> docShell = mWindow->GetDocShell();
  if (docShell) {
    nsCOMPtr<nsIPresShell> presShell;

    rv = docShell->GetPresShell(getter_AddRefs(presShell));
    if (NS_SUCCEEDED(rv) && presShell) {
      nsIFrame *rootFrame = presShell->GetRootFrame();

      if (rootFrame) {
        nsRect r(nsPoint(0, 0), rootFrame->GetSize());

        PRIntervalTime iStart = PR_IntervalNow();

        for (PRUint32 i = 0; i < aCount; i++)
          rootFrame->Invalidate(r, PR_TRUE);

#if defined(MOZ_X11) && defined(MOZ_WIDGET_GTK2)
        XSync(GDK_DISPLAY(), False);
#endif

        *aDurationOut = PR_IntervalToMilliseconds(PR_IntervalNow() - iStart);

        return NS_OK;
      }
    }
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDOMWindowUtils::SendMouseEvent(const nsAString& aType,
                                 PRInt32 aX,
                                 PRInt32 aY,
                                 PRInt32 aButton,
                                 PRInt32 aClickCount,
                                 PRInt32 aModifiers)
{
  PRBool hasCap = PR_FALSE;
  if (NS_FAILED(nsContentUtils::GetSecurityManager()->IsCapabilityEnabled("UniversalXPConnect", &hasCap))
      || !hasCap)
    return NS_ERROR_DOM_SECURITY_ERR;

  
  nsCOMPtr<nsIWidget> widget = GetWidget();
  if (!widget)
    return NS_ERROR_FAILURE;

  PRInt32 msg;
  PRBool contextMenuKey = PR_FALSE;
  if (aType.EqualsLiteral("mousedown"))
    msg = NS_MOUSE_BUTTON_DOWN;
  else if (aType.EqualsLiteral("mouseup"))
    msg = NS_MOUSE_BUTTON_UP;
  else if (aType.EqualsLiteral("mousemove"))
    msg = NS_MOUSE_MOVE;
  else if (aType.EqualsLiteral("mouseover"))
    msg = NS_MOUSE_ENTER;
  else if (aType.EqualsLiteral("mouseout"))
    msg = NS_MOUSE_EXIT;
  else if (aType.EqualsLiteral("contextmenu")) {
    msg = NS_CONTEXTMENU;
    contextMenuKey = (aButton == 0);
  } else
    return NS_ERROR_FAILURE;

  nsMouseEvent event(PR_TRUE, msg, widget, nsMouseEvent::eReal,
                     contextMenuKey ?
                       nsMouseEvent::eContextMenuKey : nsMouseEvent::eNormal);
  event.isShift = (aModifiers & nsIDOMNSEvent::SHIFT_MASK) ? PR_TRUE : PR_FALSE;
  event.isControl = (aModifiers & nsIDOMNSEvent::CONTROL_MASK) ? PR_TRUE : PR_FALSE;
  event.isAlt = (aModifiers & nsIDOMNSEvent::ALT_MASK) ? PR_TRUE : PR_FALSE;
  event.isMeta = (aModifiers & nsIDOMNSEvent::META_MASK) ? PR_TRUE : PR_FALSE;
  event.button = aButton;
  event.widget = widget;

  event.clickCount = aClickCount;
  event.time = PR_IntervalNow();
  event.refPoint.x = aX;
  event.refPoint.y = aY;

  nsEventStatus status;
  return widget->DispatchEvent(&event, status);
}

NS_IMETHODIMP
nsDOMWindowUtils::SendKeyEvent(const nsAString& aType,
                               PRInt32 aKeyCode,
                               PRInt32 aCharCode,
                               PRInt32 aModifiers)
{
  PRBool hasCap = PR_FALSE;
  if (NS_FAILED(nsContentUtils::GetSecurityManager()->IsCapabilityEnabled("UniversalXPConnect", &hasCap))
      || !hasCap)
    return NS_ERROR_DOM_SECURITY_ERR;

  
  nsCOMPtr<nsIWidget> widget = GetWidget();
  if (!widget)
    return NS_ERROR_FAILURE;

  PRInt32 msg;
  if (aType.EqualsLiteral("keydown"))
    msg = NS_KEY_DOWN;
  else if (aType.EqualsLiteral("keyup"))
    msg = NS_KEY_UP;
  else if (aType.EqualsLiteral("keypress"))
    msg = NS_KEY_PRESS;
  else
    return NS_ERROR_FAILURE;

  nsKeyEvent event(PR_TRUE, msg, widget);
  event.isShift = (aModifiers & nsIDOMNSEvent::SHIFT_MASK) ? PR_TRUE : PR_FALSE;
  event.isControl = (aModifiers & nsIDOMNSEvent::CONTROL_MASK) ? PR_TRUE : PR_FALSE;
  event.isAlt = (aModifiers & nsIDOMNSEvent::ALT_MASK) ? PR_TRUE : PR_FALSE;
  event.isMeta = (aModifiers & nsIDOMNSEvent::META_MASK) ? PR_TRUE : PR_FALSE;

  event.keyCode = aKeyCode;
  event.charCode = aCharCode;
  event.refPoint.x = event.refPoint.y = 0;
  event.time = PR_IntervalNow();

  nsEventStatus status;
  return widget->DispatchEvent(&event, status);
}

NS_IMETHODIMP
nsDOMWindowUtils::SendNativeKeyEvent(PRInt32 aNativeKeyboardLayout,
                                     PRInt32 aNativeKeyCode,
                                     PRInt32 aModifiers,
                                     const nsAString& aCharacters,
                                     const nsAString& aUnmodifiedCharacters)
{
  PRBool hasCap = PR_FALSE;
  if (NS_FAILED(nsContentUtils::GetSecurityManager()->IsCapabilityEnabled("UniversalXPConnect", &hasCap))
      || !hasCap)
    return NS_ERROR_DOM_SECURITY_ERR;

  
  nsCOMPtr<nsIWidget> widget = GetWidget();
  if (!widget)
    return NS_ERROR_FAILURE;

  return widget->SynthesizeNativeKeyEvent(aNativeKeyboardLayout, aNativeKeyCode,
                                          aModifiers, aCharacters, aUnmodifiedCharacters);
}

NS_IMETHODIMP
nsDOMWindowUtils::ActivateNativeMenuItemAt(const nsAString& indexString)
{
  PRBool hasCap = PR_FALSE;
  if (NS_FAILED(nsContentUtils::GetSecurityManager()->IsCapabilityEnabled("UniversalXPConnect", &hasCap))
      || !hasCap)
    return NS_ERROR_DOM_SECURITY_ERR;

  
  nsCOMPtr<nsIWidget> widget = GetWidget();
  if (!widget)
    return NS_ERROR_FAILURE;

  return widget->ActivateNativeMenuItemAt(indexString);
}


NS_IMETHODIMP
nsDOMWindowUtils::ForceNativeMenuReload()
{
  PRBool hasCap = PR_FALSE;
  if (NS_FAILED(nsContentUtils::GetSecurityManager()->IsCapabilityEnabled("UniversalXPConnect", &hasCap))
      || !hasCap)
    return NS_ERROR_DOM_SECURITY_ERR;

  
  nsCOMPtr<nsIWidget> widget = GetWidget();
  if (!widget)
    return NS_ERROR_FAILURE;

  return widget->ForceNativeMenuReload();
}


nsIWidget*
nsDOMWindowUtils::GetWidget()
{
  if (mWindow) {
    nsIDocShell *docShell = mWindow->GetDocShell();
    if (docShell) {
      nsCOMPtr<nsIPresShell> presShell;
      docShell->GetPresShell(getter_AddRefs(presShell));
      if (presShell) {
        nsIFrame* frame = presShell->GetRootFrame();
        if (frame)
          return frame->GetWindow();
      }
    }
  }

  return nsnull;
}

NS_IMETHODIMP
nsDOMWindowUtils::Focus(nsIDOMElement* aElement)
{
  PRBool hasCap = PR_FALSE;
  if (NS_FAILED(nsContentUtils::GetSecurityManager()->IsCapabilityEnabled(
    "UniversalXPConnect", &hasCap)) || !hasCap)
    return NS_ERROR_DOM_SECURITY_ERR;

  if (mWindow) {
    nsCOMPtr<nsIContent> content = do_QueryInterface(aElement);
    if (content) {
      nsCOMPtr<nsIDocument> doc(do_QueryInterface(mWindow->GetExtantDocument()));
      if (!doc || content->GetCurrentDoc() != doc)
        return NS_ERROR_FAILURE;
    }

    nsIDocShell *docShell = mWindow->GetDocShell();
    if (docShell) {
      nsCOMPtr<nsIPresShell> presShell;
      docShell->GetPresShell(getter_AddRefs(presShell));
      if (presShell) {
        nsPresContext *pc = presShell->GetPresContext();
        if (pc) {
          pc->EventStateManager()->ChangeFocusWith(content,
              nsIEventStateManager::eEventFocusedByApplication);
        }
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWindowUtils::GarbageCollect()
{
  
  
#ifndef DEBUG
  PRBool hasCap = PR_FALSE;
  if (NS_FAILED(nsContentUtils::GetSecurityManager()->
                  IsCapabilityEnabled("UniversalXPConnect", &hasCap))
      || !hasCap)
    return NS_ERROR_DOM_SECURITY_ERR;
#endif

  nsJSContext::CC();
  nsJSContext::CC();

  return NS_OK;
}

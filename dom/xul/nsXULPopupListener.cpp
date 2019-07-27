









#include "nsXULPopupListener.h"
#include "nsCOMPtr.h"
#include "nsGkAtoms.h"
#include "nsIDOMElement.h"
#include "nsIDOMXULElement.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentXBL.h"
#include "nsContentCID.h"
#include "nsContentUtils.h"
#include "nsXULPopupManager.h"
#include "nsIScriptContext.h"
#include "nsIDOMWindow.h"
#include "nsIDOMXULDocument.h"
#include "nsIDocument.h"
#include "nsServiceManagerUtils.h"
#include "nsIPrincipal.h"
#include "nsIScriptSecurityManager.h"
#include "nsLayoutUtils.h"
#include "nsHTMLReflowState.h"
#include "nsIObjectLoadingContent.h"
#include "mozilla/EventStateManager.h"
#include "mozilla/EventStates.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/Event.h" 
#include "mozilla/dom/EventTarget.h"
#include "mozilla/dom/FragmentOrElement.h"


#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsFocusManager.h"
#include "nsPIDOMWindow.h"
#include "nsViewManager.h"
#include "nsError.h"
#include "nsMenuFrame.h"

using namespace mozilla;
using namespace mozilla::dom;



#if defined(XP_WIN)
#define NS_CONTEXT_MENU_IS_MOUSEUP 1
#endif

nsXULPopupListener::nsXULPopupListener(mozilla::dom::Element* aElement,
                                       bool aIsContext)
  : mElement(aElement), mPopupContent(nullptr), mIsContext(aIsContext)
{
}

nsXULPopupListener::~nsXULPopupListener(void)
{
  ClosePopup();
}

NS_IMPL_CYCLE_COLLECTION(nsXULPopupListener, mElement, mPopupContent)
NS_IMPL_CYCLE_COLLECTING_ADDREF(nsXULPopupListener)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsXULPopupListener)

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_BEGIN(nsXULPopupListener)
  
  if (tmp->mElement) {
    return mozilla::dom::FragmentOrElement::CanSkip(tmp->mElement, true);
  }
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_BEGIN(nsXULPopupListener)
  if (tmp->mElement) {
    return mozilla::dom::FragmentOrElement::CanSkipInCC(tmp->mElement);
  }
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_BEGIN(nsXULPopupListener)
  if (tmp->mElement) {
    return mozilla::dom::FragmentOrElement::CanSkipThis(tmp->mElement);
  }
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsXULPopupListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END




nsresult
nsXULPopupListener::HandleEvent(nsIDOMEvent* aEvent)
{
  nsAutoString eventType;
  aEvent->GetType(eventType);

  if(!((eventType.EqualsLiteral("mousedown") && !mIsContext) ||
       (eventType.EqualsLiteral("contextmenu") && mIsContext)))
    return NS_OK;

  int16_t button;

  nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aEvent);
  if (!mouseEvent) {
    
    return NS_OK;
  }

  
  EventTarget* target = mouseEvent->InternalDOMEvent()->GetTarget();
  nsCOMPtr<nsIDOMNode> targetNode = do_QueryInterface(target);

  if (!targetNode && mIsContext) {
    
    nsCOMPtr<nsIDOMWindow> domWin = do_QueryInterface(target);
    if (!domWin) {
      return NS_ERROR_DOM_WRONG_TYPE_ERR;
    }
    
    nsCOMPtr<nsIDOMDocument> domDoc;
    domWin->GetDocument(getter_AddRefs(domDoc));

    nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
    if (doc)
      targetNode = do_QueryInterface(doc->GetRootElement());
    if (!targetNode) {
      return NS_ERROR_FAILURE;
    }
  }

  nsCOMPtr<nsIContent> targetContent = do_QueryInterface(target);
  if (!targetContent) {
    return NS_OK;
  }
  if (targetContent->Tag() == nsGkAtoms::browser &&
      targetContent->IsXUL() &&
      EventStateManager::IsRemoteTarget(targetContent)) {
    return NS_OK;
  }

  bool preventDefault;
  mouseEvent->GetDefaultPrevented(&preventDefault);
  if (preventDefault && targetNode && mIsContext) {
    
    
    bool eventEnabled =
      Preferences::GetBool("dom.event.contextmenu.enabled", true);
    if (!eventEnabled) {
      
      
      nsCOMPtr<nsIObjectLoadingContent> olc = do_QueryInterface(targetNode);
      uint32_t type;
      if (olc && NS_SUCCEEDED(olc->GetDisplayedType(&type)) &&
          type == nsIObjectLoadingContent::TYPE_PLUGIN) {
        return NS_OK;
      }

      
      
      
      nsCOMPtr<nsINode> node = do_QueryInterface(targetNode);
      if (node) {
        nsCOMPtr<nsIPrincipal> system;
        nsContentUtils::GetSecurityManager()->
          GetSystemPrincipal(getter_AddRefs(system));
        if (node->NodePrincipal() != system) {
          
          
          preventDefault = false;
        }
      }
    }
  }

  if (preventDefault) {
    
    return NS_OK;
  }

  
  
  
  
  
  if (!mIsContext) {
    nsIAtom *tag = targetContent ? targetContent->Tag() : nullptr;
    if (tag == nsGkAtoms::menu || tag == nsGkAtoms::menuitem)
      return NS_OK;
  }

  if (mIsContext) {
#ifndef NS_CONTEXT_MENU_IS_MOUSEUP
    
    
    FireFocusOnTargetContent(targetNode);
#endif
  }
  else {
    
    mouseEvent->GetButton(&button);
    if (button != 0)
      return NS_OK;
  }

  
  
  LaunchPopup(aEvent, targetContent);

  return NS_OK;
}

#ifndef NS_CONTEXT_MENU_IS_MOUSEUP
nsresult
nsXULPopupListener::FireFocusOnTargetContent(nsIDOMNode* aTargetNode)
{
  nsresult rv;
  nsCOMPtr<nsIDOMDocument> domDoc;
  rv = aTargetNode->GetOwnerDocument(getter_AddRefs(domDoc));
  if(NS_SUCCEEDED(rv) && domDoc)
  {
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);

    
    nsIPresShell *shell = doc->GetShell();
    if (!shell)
      return NS_ERROR_FAILURE;

    
    
    nsRefPtr<nsPresContext> context = shell->GetPresContext();
 
    nsCOMPtr<nsIContent> content = do_QueryInterface(aTargetNode);
    nsIFrame* targetFrame = content->GetPrimaryFrame();
    if (!targetFrame) return NS_ERROR_FAILURE;

    const nsStyleUserInterface* ui = targetFrame->StyleUserInterface();
    bool suppressBlur = (ui->mUserFocus == NS_STYLE_USER_FOCUS_IGNORE);

    nsCOMPtr<nsIDOMElement> element;
    nsCOMPtr<nsIContent> newFocus = do_QueryInterface(content);

    nsIFrame* currFrame = targetFrame;
    
    while (currFrame) {
        int32_t tabIndexUnused;
        if (currFrame->IsFocusable(&tabIndexUnused, true)) {
          newFocus = currFrame->GetContent();
          nsCOMPtr<nsIDOMElement> domElement(do_QueryInterface(newFocus));
          if (domElement) {
            element = domElement;
            break;
          }
        }
        currFrame = currFrame->GetParent();
    } 

    nsIFocusManager* fm = nsFocusManager::GetFocusManager();
    if (fm) {
      if (element) {
        fm->SetFocus(element, nsIFocusManager::FLAG_BYMOUSE |
                              nsIFocusManager::FLAG_NOSCROLL);
      } else if (!suppressBlur) {
        nsPIDOMWindow *window = doc->GetWindow();
        fm->ClearFocus(window);
      }
    }

    EventStateManager* esm = context->EventStateManager();
    nsCOMPtr<nsIContent> focusableContent = do_QueryInterface(element);
    esm->SetContentState(focusableContent, NS_EVENT_STATE_ACTIVE);
  }
  return rv;
}
#endif







void
nsXULPopupListener::ClosePopup()
{
  if (mPopupContent) {
    
    
    
    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (pm)
      pm->HidePopup(mPopupContent, false, true, true, false);
    mPopupContent = nullptr;  
  }
} 

static already_AddRefed<nsIContent>
GetImmediateChild(nsIContent* aContent, nsIAtom *aTag) 
{
  for (nsIContent* child = aContent->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    if (child->Tag() == aTag) {
      nsCOMPtr<nsIContent> ret = child;
      return ret.forget();
    }
  }

  return nullptr;
}
















nsresult
nsXULPopupListener::LaunchPopup(nsIDOMEvent* aEvent, nsIContent* aTargetContent)
{
  nsresult rv = NS_OK;

  nsAutoString identifier;
  nsIAtom* type = mIsContext ? nsGkAtoms::context : nsGkAtoms::popup;
  bool hasPopupAttr = mElement->GetAttr(kNameSpaceID_None, type, identifier);

  if (identifier.IsEmpty()) {
    hasPopupAttr = mElement->GetAttr(kNameSpaceID_None,
                          mIsContext ? nsGkAtoms::contextmenu : nsGkAtoms::menu,
                          identifier) || hasPopupAttr;
  }

  if (hasPopupAttr) {
    aEvent->StopPropagation();
    aEvent->PreventDefault();
  }

  if (identifier.IsEmpty())
    return rv;

  
  nsCOMPtr<nsIDocument> document = mElement->GetDocument();
  if (!document) {
    NS_WARNING("No document!");
    return NS_ERROR_FAILURE;
  }

  
  nsCOMPtr<nsIContent> popup;
  if (identifier.EqualsLiteral("_child")) {
    popup = GetImmediateChild(mElement, nsGkAtoms::menupopup);
    if (!popup) {
      nsCOMPtr<nsIDOMDocumentXBL> nsDoc(do_QueryInterface(document));
      nsCOMPtr<nsIDOMNodeList> list;
      nsCOMPtr<nsIDOMElement> el = do_QueryInterface(mElement);
      nsDoc->GetAnonymousNodes(el, getter_AddRefs(list));
      if (list) {
        uint32_t ctr,listLength;
        nsCOMPtr<nsIDOMNode> node;
        list->GetLength(&listLength);
        for (ctr = 0; ctr < listLength; ctr++) {
          list->Item(ctr, getter_AddRefs(node));
          nsCOMPtr<nsIContent> childContent(do_QueryInterface(node));

          if (childContent->NodeInfo()->Equals(nsGkAtoms::menupopup,
                                               kNameSpaceID_XUL)) {
            popup.swap(childContent);
            break;
          }
        }
      }
    }
  } else if (!(popup = document->GetElementById(identifier))) {
    
    
    NS_ERROR("GetElementById had some kind of spasm.");
    return rv;
  }

  
  if (!popup || popup == mElement)
    return NS_OK;

  
  
  nsIContent* parent = popup->GetParent();
  if (parent) {
    nsMenuFrame* menu = do_QueryFrame(parent->GetPrimaryFrame());
    if (menu)
      return NS_OK;
  }

  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (!pm)
    return NS_OK;

  
  
  
  
  mPopupContent = popup;
  if (!mIsContext &&
      (mPopupContent->HasAttr(kNameSpaceID_None, nsGkAtoms::position) ||
       (mPopupContent->HasAttr(kNameSpaceID_None, nsGkAtoms::popupanchor) &&
        mPopupContent->HasAttr(kNameSpaceID_None, nsGkAtoms::popupalign)))) {
    pm->ShowPopup(mPopupContent, mElement, EmptyString(), 0, 0,
                  false, true, false, aEvent);
  }
  else {
    int32_t xPos = 0, yPos = 0;
    nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aEvent);
    mouseEvent->GetScreenX(&xPos);
    mouseEvent->GetScreenY(&yPos);

    pm->ShowPopupAtScreen(mPopupContent, xPos, yPos, mIsContext, aEvent);
  }

  return NS_OK;
}















































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
#include "nsEventStateManager.h"
#include "nsIScriptContext.h"
#include "nsIDOMWindow.h"
#include "nsIDOMXULDocument.h"
#include "nsIDocument.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMNSEvent.h"
#include "nsServiceManagerUtils.h"
#include "nsIPrincipal.h"
#include "nsIScriptSecurityManager.h"
#include "nsLayoutUtils.h"
#include "nsFrameManager.h"
#include "nsHTMLReflowState.h"
#include "nsIObjectLoadingContent.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/Element.h"


#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsFocusManager.h"
#include "nsPIDOMWindow.h"
#include "nsIViewManager.h"
#include "nsDOMError.h"

using namespace mozilla;



#if defined(XP_WIN) || defined(XP_OS2)
#define NS_CONTEXT_MENU_IS_MOUSEUP 1
#endif

nsXULPopupListener::nsXULPopupListener(nsIDOMElement *aElement, bool aIsContext)
  : mElement(aElement), mPopupContent(nsnull), mIsContext(aIsContext)
{
}

nsXULPopupListener::~nsXULPopupListener(void)
{
  ClosePopup();
}

NS_IMPL_CYCLE_COLLECTION_2(nsXULPopupListener, mElement, mPopupContent)
NS_IMPL_CYCLE_COLLECTING_ADDREF(nsXULPopupListener)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsXULPopupListener)

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

  PRUint16 button;

  nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aEvent);
  if (!mouseEvent) {
    
    return NS_OK;
  }

  
  nsCOMPtr<nsIDOMNSEvent> domNSEvent = do_QueryInterface(mouseEvent);
  if (!domNSEvent) {
    return NS_OK;
  }

  
  nsCOMPtr<nsIDOMEventTarget> target;
  mouseEvent->GetTarget(getter_AddRefs(target));
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

  bool preventDefault;
  domNSEvent->GetPreventDefault(&preventDefault);
  if (preventDefault && targetNode && mIsContext) {
    
    
    bool eventEnabled =
      Preferences::GetBool("dom.event.contextmenu.enabled", true);
    if (!eventEnabled) {
      
      
      nsCOMPtr<nsIObjectLoadingContent> olc = do_QueryInterface(targetNode);
      PRUint32 type;
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
          
          
          preventDefault = PR_FALSE;
        }
      }
    }
  }

  if (preventDefault) {
    
    return NS_OK;
  }

  
  
  
  
  
  nsCOMPtr<nsIContent> targetContent = do_QueryInterface(target);
  if (!mIsContext) {
    nsIAtom *tag = targetContent ? targetContent->Tag() : nsnull;
    if (tag == nsGkAtoms::menu || tag == nsGkAtoms::menuitem)
      return NS_OK;
  }

  nsCOMPtr<nsIDOMNSEvent> nsevent = do_QueryInterface(aEvent);

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
  aEvent->StopPropagation();
  aEvent->PreventDefault();

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

    const nsStyleUserInterface* ui = targetFrame->GetStyleUserInterface();
    bool suppressBlur = (ui->mUserFocus == NS_STYLE_USER_FOCUS_IGNORE);

    nsCOMPtr<nsIDOMElement> element;
    nsCOMPtr<nsIContent> newFocus = do_QueryInterface(content);

    nsIFrame* currFrame = targetFrame;
    
    while (currFrame) {
        PRInt32 tabIndexUnused;
        if (currFrame->IsFocusable(&tabIndexUnused, PR_TRUE)) {
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

    nsEventStateManager *esm = context->EventStateManager();
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
      pm->HidePopup(mPopupContent, PR_FALSE, PR_TRUE, PR_TRUE);
    mPopupContent = nsnull;  
  }
} 

static already_AddRefed<nsIContent>
GetImmediateChild(nsIContent* aContent, nsIAtom *aTag) 
{
  for (nsIContent* child = aContent->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    if (child->Tag() == aTag) {
      NS_ADDREF(child);
      return child;
    }
  }

  return nsnull;
}
















nsresult
nsXULPopupListener::LaunchPopup(nsIDOMEvent* aEvent, nsIContent* aTargetContent)
{
  nsresult rv = NS_OK;

  nsAutoString type(NS_LITERAL_STRING("popup"));
  if (mIsContext)
    type.AssignLiteral("context");

  nsAutoString identifier;
  mElement->GetAttribute(type, identifier);

  if (identifier.IsEmpty()) {
    if (type.EqualsLiteral("popup"))
      mElement->GetAttribute(NS_LITERAL_STRING("menu"), identifier);
    else if (type.EqualsLiteral("context"))
      mElement->GetAttribute(NS_LITERAL_STRING("contextmenu"), identifier);
    if (identifier.IsEmpty())
      return rv;
  }

  
  nsCOMPtr<nsIContent> content = do_QueryInterface(mElement);
  nsCOMPtr<nsIDocument> document = content->GetDocument();

  
  nsCOMPtr<nsIDOMDocument> domDocument = do_QueryInterface(document);
  if (!domDocument) {
    NS_ERROR("Popup attached to an element that isn't in XUL!");
    return NS_ERROR_FAILURE;
  }

  
  nsCOMPtr<nsIDOMElement> popupElement;

  if (identifier.EqualsLiteral("_child")) {
    nsCOMPtr<nsIContent> popup = GetImmediateChild(content, nsGkAtoms::menupopup);
    if (popup)
      popupElement = do_QueryInterface(popup);
    else {
      nsCOMPtr<nsIDOMDocumentXBL> nsDoc(do_QueryInterface(domDocument));
      nsCOMPtr<nsIDOMNodeList> list;
      nsDoc->GetAnonymousNodes(mElement, getter_AddRefs(list));
      if (list) {
        PRUint32 ctr,listLength;
        nsCOMPtr<nsIDOMNode> node;
        list->GetLength(&listLength);
        for (ctr = 0; ctr < listLength; ctr++) {
          list->Item(ctr, getter_AddRefs(node));
          nsCOMPtr<nsIContent> childContent(do_QueryInterface(node));

          if (childContent->NodeInfo()->Equals(nsGkAtoms::menupopup,
                                               kNameSpaceID_XUL)) {
            popupElement = do_QueryInterface(childContent);
            break;
          }
        }
      }
    }
  }
  else if (NS_FAILED(rv = domDocument->GetElementById(identifier,
                                              getter_AddRefs(popupElement)))) {
    
    
    NS_ERROR("GetElementById had some kind of spasm.");
    return rv;
  }

  
  if ( !popupElement || popupElement == mElement)
    return NS_OK;

  
  
  nsCOMPtr<nsIContent> popup = do_QueryInterface(popupElement);
  nsIContent* parent = popup->GetParent();
  if (parent) {
    nsIFrame* frame = parent->GetPrimaryFrame();
    if (frame && frame->GetType() == nsGkAtoms::menuFrame)
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
    pm->ShowPopup(mPopupContent, content, EmptyString(), 0, 0,
                  PR_FALSE, PR_TRUE, PR_FALSE, aEvent);
  }
  else {
    PRInt32 xPos = 0, yPos = 0;
    nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aEvent);
    mouseEvent->GetScreenX(&xPos);
    mouseEvent->GetScreenY(&yPos);

    pm->ShowPopupAtScreen(mPopupContent, xPos, yPos, mIsContext, aEvent);
  }

  return NS_OK;
}

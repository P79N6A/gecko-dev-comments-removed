













































#include "nsCOMPtr.h"
#include "nsGkAtoms.h"
#include "nsIDOMElement.h"
#include "nsIDOMXULElement.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentXBL.h"
#include "nsIXULPopupListener.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMContextMenuListener.h"
#include "nsContentCID.h"
#include "nsContentUtils.h"

#include "nsIScriptContext.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMXULDocument.h"
#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMNSEvent.h"
#include "nsServiceManagerUtils.h"
#include "nsIPrincipal.h"
#include "nsIScriptSecurityManager.h"

#include "nsIBoxObject.h"
#include "nsIPopupBoxObject.h"


#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIEventStateManager.h"
#include "nsIFocusController.h"
#include "nsPIDOMWindow.h"
#include "nsDOMError.h"

#include "nsIFrame.h"
#include "nsIMenuFrame.h"



#if !defined(XP_WIN) && !defined(XP_OS2)
#define NS_CONTEXT_MENU_IS_MOUSEUP 1
#endif







class XULPopupListenerImpl : public nsIXULPopupListener,
                             public nsIDOMMouseListener,
                             public nsIDOMContextMenuListener
{
public:
    XULPopupListenerImpl(void);
    virtual ~XULPopupListenerImpl(void);

public:
    
    NS_DECL_ISUPPORTS

    
    NS_IMETHOD Init(nsIDOMElement* aElement, const XULPopupType& popupType);

    
    NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent);
    NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent) { return NS_OK; }
    NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent) { return NS_OK; }
    NS_IMETHOD MouseDblClick(nsIDOMEvent* aMouseEvent) { return NS_OK; }
    NS_IMETHOD MouseOver(nsIDOMEvent* aMouseEvent) { return NS_OK; }
    NS_IMETHOD MouseOut(nsIDOMEvent* aMouseEvent) { return NS_OK; }

    
    NS_IMETHOD ContextMenu(nsIDOMEvent* aContextMenuEvent);

    
    NS_IMETHOD HandleEvent(nsIDOMEvent* anEvent) { return NS_OK; }

protected:

    virtual nsresult LaunchPopup(nsIDOMEvent* anEvent);
    virtual nsresult LaunchPopup(PRInt32 aClientX, PRInt32 aClientY) ;

private:

    nsresult PreLaunchPopup(nsIDOMEvent* aMouseEvent);
    nsresult FireFocusOnTargetContent(nsIDOMNode* aTargetNode);

    
    nsIDOMElement* mElement;               

    
    nsCOMPtr<nsIPopupBoxObject> mPopup;

    
    XULPopupType popupType;
    
};


      
XULPopupListenerImpl::XULPopupListenerImpl(void)
  : mElement(nsnull)
{
}

XULPopupListenerImpl::~XULPopupListenerImpl(void)
{
  if (mPopup) {
    mPopup->HidePopup();
  }
  
#ifdef DEBUG_REFS
    --gInstanceCount;
    fprintf(stdout, "%d - RDF: XULPopupListenerImpl\n", gInstanceCount);
#endif
}

NS_IMPL_ADDREF(XULPopupListenerImpl)
NS_IMPL_RELEASE(XULPopupListenerImpl)


NS_INTERFACE_MAP_BEGIN(XULPopupListenerImpl)
  NS_INTERFACE_MAP_ENTRY(nsIXULPopupListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMContextMenuListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEventListener, nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXULPopupListener)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
XULPopupListenerImpl::Init(nsIDOMElement* aElement, const XULPopupType& popup)
{
  mElement = aElement; 
  popupType = popup;
  return NS_OK;
}




nsresult
XULPopupListenerImpl::MouseDown(nsIDOMEvent* aMouseEvent)
{
  if(popupType != eXULPopupType_context)
    return PreLaunchPopup(aMouseEvent);
  else
    return NS_OK;
}

nsresult
XULPopupListenerImpl::ContextMenu(nsIDOMEvent* aMouseEvent)
{
  if(popupType == eXULPopupType_context)
    return PreLaunchPopup(aMouseEvent);
  else 
    return NS_OK;
}

nsresult
XULPopupListenerImpl::PreLaunchPopup(nsIDOMEvent* aMouseEvent)
{
  PRUint16 button;

  nsCOMPtr<nsIDOMMouseEvent> mouseEvent;
  mouseEvent = do_QueryInterface(aMouseEvent);
  if (!mouseEvent) {
    
    return NS_OK;
  }

  
  nsCOMPtr<nsIDOMNSUIEvent> nsUIEvent;
  nsUIEvent = do_QueryInterface(mouseEvent);
  if (!nsUIEvent) {
    return NS_OK;
  }

  
  nsCOMPtr<nsIDOMEventTarget> target;
  mouseEvent->GetTarget(getter_AddRefs(target));
  nsCOMPtr<nsIDOMNode> targetNode = do_QueryInterface(target);

  if (!targetNode && popupType == eXULPopupType_context) {
    
    nsCOMPtr<nsIDOMWindow> domWin = do_QueryInterface(target);
    if (!domWin) {
      return NS_ERROR_DOM_WRONG_TYPE_ERR;
    }
    
    nsCOMPtr<nsIDOMDocument> domDoc;
    domWin->GetDocument(getter_AddRefs(domDoc));

    nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
    if (doc)
      targetNode = do_QueryInterface(doc->GetRootContent());
    if (!targetNode) {
      return NS_ERROR_FAILURE;
    }
  }

  PRBool preventDefault;
  nsUIEvent->GetPreventDefault(&preventDefault);
  if (preventDefault && targetNode && popupType == eXULPopupType_context) {
    
    
    PRBool eventEnabled =
      nsContentUtils::GetBoolPref("dom.event.contextmenu.enabled", PR_TRUE);
    if (!eventEnabled) {
      
      
      
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

  
  
  
  
  
  if (popupType == eXULPopupType_popup) {
    nsCOMPtr<nsIContent> targetContent = do_QueryInterface(target);
    nsIAtom *tag = targetContent ? targetContent->Tag() : nsnull;
    if (tag == nsGkAtoms::menu || tag == nsGkAtoms::menuitem)
      return NS_OK;
  }

  
  nsCOMPtr<nsIContent> content = do_QueryInterface(mElement);

  
  nsCOMPtr<nsIDOMXULDocument> xulDocument = do_QueryInterface(content->GetDocument());
  if (!xulDocument) {
    NS_ERROR("Popup attached to an element that isn't in XUL!");
    return NS_ERROR_FAILURE;
  }

  
  
  xulDocument->SetPopupNode( targetNode );
  xulDocument->SetTrustedPopupEvent( aMouseEvent );

  nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aMouseEvent));

  switch (popupType) {
    case eXULPopupType_popup:
      
      mouseEvent->GetButton(&button);
      if (button == 0) {
        
        LaunchPopup(aMouseEvent);
        aMouseEvent->StopPropagation();
        aMouseEvent->PreventDefault();
      }
      break;
    case eXULPopupType_context:

      
#ifndef NS_CONTEXT_MENU_IS_MOUSEUP
      
      
      FireFocusOnTargetContent(targetNode);
#endif
      LaunchPopup(aMouseEvent);
      aMouseEvent->StopPropagation();
      aMouseEvent->PreventDefault();
      break;
  }
  xulDocument->SetTrustedPopupEvent(nsnull);
  return NS_OK;
}

nsresult
XULPopupListenerImpl::FireFocusOnTargetContent(nsIDOMNode* aTargetNode)
{
  nsresult rv;
  nsCOMPtr<nsIDOMDocument> domDoc;
  rv = aTargetNode->GetOwnerDocument(getter_AddRefs(domDoc));
  if(NS_SUCCEEDED(rv) && domDoc)
  {
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);

    
    nsIPresShell *shell = doc->GetPrimaryShell();
    if (!shell)
      return NS_ERROR_FAILURE;

    
    nsCOMPtr<nsPresContext> context = shell->GetPresContext();
 
    nsCOMPtr<nsIContent> content = do_QueryInterface(aTargetNode);
    nsIFrame* targetFrame = shell->GetPrimaryFrameFor(content);
    if (!targetFrame) return NS_ERROR_FAILURE;
      
    PRBool suppressBlur = PR_FALSE;
    const nsStyleUserInterface* ui = targetFrame->GetStyleUserInterface();
    suppressBlur = (ui->mUserFocus == NS_STYLE_USER_FOCUS_IGNORE);

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
    nsCOMPtr<nsIContent> focusableContent = do_QueryInterface(element);
    nsIEventStateManager *esm = context->EventStateManager();

    if (focusableContent) {
      
      nsCOMPtr<nsIFocusController> focusController = nsnull;
      PRBool isAlreadySuppressed = PR_FALSE;
      nsPIDOMWindow *ourWindow = doc->GetWindow();
      if (ourWindow) {
        focusController = ourWindow->GetRootFocusController();
        if (focusController) {
          focusController->GetSuppressFocusScroll(&isAlreadySuppressed);
          if (!isAlreadySuppressed)
            focusController->SetSuppressFocusScroll(PR_TRUE);
        }
      }

      focusableContent->SetFocus(context);

      
      if (focusController && !isAlreadySuppressed)
        focusController->SetSuppressFocusScroll(PR_FALSE);
    } else if (!suppressBlur)
      esm->SetContentState(nsnull, NS_EVENT_STATE_FOCUS);

    esm->SetContentState(focusableContent, NS_EVENT_STATE_ACTIVE);
  }
  return rv;
}




nsresult
XULPopupListenerImpl::LaunchPopup ( nsIDOMEvent* anEvent )
{
  
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent ( do_QueryInterface(anEvent) );
  if (!mouseEvent) {
    
    return NS_OK;
  }

  PRInt32 xPos, yPos;
  mouseEvent->GetClientX(&xPos); 
  mouseEvent->GetClientY(&yPos); 

  return LaunchPopup(xPos, yPos);
}


static void
GetImmediateChild(nsIContent* aContent, nsIAtom *aTag, nsIContent** aResult) 
{
  *aResult = nsnull;
  PRInt32 childCount = aContent->GetChildCount();
  for (PRInt32 i = 0; i < childCount; i++) {
    nsIContent *child = aContent->GetChildAt(i);
    if (child->Tag() == aTag) {
      *aResult = child;
      NS_ADDREF(*aResult);
      return;
    }
  }

  return;
}

static void ConvertPosition(nsIDOMElement* aPopupElt, nsString& aAnchor, nsString& aAlign, PRInt32& aY)
{
  nsAutoString position;
  aPopupElt->GetAttribute(NS_LITERAL_STRING("position"), position);
  if (position.IsEmpty())
    return;

  if (position.EqualsLiteral("before_start")) {
    aAnchor.AssignLiteral("topleft");
    aAlign.AssignLiteral("bottomleft");
  }
  else if (position.EqualsLiteral("before_end")) {
    aAnchor.AssignLiteral("topright");
    aAlign.AssignLiteral("bottomright");
  }
  else if (position.EqualsLiteral("after_start")) {
    aAnchor.AssignLiteral("bottomleft");
    aAlign.AssignLiteral("topleft");
  }
  else if (position.EqualsLiteral("after_end")) {
    aAnchor.AssignLiteral("bottomright");
    aAlign.AssignLiteral("topright");
  }
  else if (position.EqualsLiteral("start_before")) {
    aAnchor.AssignLiteral("topleft");
    aAlign.AssignLiteral("topright");
  }
  else if (position.EqualsLiteral("start_after")) {
    aAnchor.AssignLiteral("bottomleft");
    aAlign.AssignLiteral("bottomright");
  }
  else if (position.EqualsLiteral("end_before")) {
    aAnchor.AssignLiteral("topright");
    aAlign.AssignLiteral("topleft");
  }
  else if (position.EqualsLiteral("end_after")) {
    aAnchor.AssignLiteral("bottomright");
    aAlign.AssignLiteral("bottomleft");
  }
  else if (position.EqualsLiteral("overlap")) {
    aAnchor.AssignLiteral("topleft");
    aAlign.AssignLiteral("topleft");
  }
  else if (position.EqualsLiteral("after_pointer"))
    aY += 21;
}












nsresult
XULPopupListenerImpl::LaunchPopup(PRInt32 aClientX, PRInt32 aClientY)
{
  nsresult rv = NS_OK;

  nsAutoString type(NS_LITERAL_STRING("popup"));
  if ( popupType == eXULPopupType_context ) {
    type.AssignLiteral("context");
    
    
    
    
    aClientX += 2;
    aClientY += 2;
  }

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

  
  nsCOMPtr<nsIDOMElement> popupContent;

  if (identifier.EqualsLiteral("_child")) {
    nsCOMPtr<nsIContent> popup;

    GetImmediateChild(content, nsGkAtoms::menupopup, getter_AddRefs(popup));
    if (popup)
      popupContent = do_QueryInterface(popup);
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
            popupContent = do_QueryInterface(childContent);
            break;
          }
        }
      }
    }
  }
  else if (NS_FAILED(rv = domDocument->GetElementById(identifier,
                                              getter_AddRefs(popupContent)))) {
    
    
    NS_ERROR("GetElementById had some kind of spasm.");
    return rv;
  }
  if ( !popupContent )
    return NS_OK;

  
  
  nsCOMPtr<nsIContent> popup = do_QueryInterface(popupContent);
  nsIContent* parent = popup->GetParent();
  if (parent) {
    nsIDocument* doc = parent->GetCurrentDoc();
    nsIPresShell* presShell = doc ? doc->GetPrimaryShell() : nsnull;
    nsIFrame* frame = presShell ? presShell->GetPrimaryFrameFor(parent) : nsnull;
    if (frame) {
      nsIMenuFrame* menu = nsnull;
      CallQueryInterface(frame, &menu);
      NS_ENSURE_FALSE(menu, NS_OK);
    }
  }

  
  nsPIDOMWindow *domWindow = document->GetWindow();

  if (domWindow) {
    
    nsAutoString anchorAlignment;
    popupContent->GetAttribute(NS_LITERAL_STRING("popupanchor"), anchorAlignment);

    nsAutoString popupAlignment;
    popupContent->GetAttribute(NS_LITERAL_STRING("popupalign"), popupAlignment);

    PRInt32 xPos = aClientX, yPos = aClientY;

    ConvertPosition(popupContent, anchorAlignment, popupAlignment, yPos);
    if (!anchorAlignment.IsEmpty() && !popupAlignment.IsEmpty())
      xPos = yPos = -1;

    nsCOMPtr<nsIBoxObject> popupBox;
    nsCOMPtr<nsIDOMXULElement> xulPopupElt(do_QueryInterface(popupContent));
    xulPopupElt->GetBoxObject(getter_AddRefs(popupBox));
    nsCOMPtr<nsIPopupBoxObject> popupBoxObject(do_QueryInterface(popupBox));
    if (popupBoxObject) {
      mPopup = popupBoxObject;
      popupBoxObject->ShowPopup(mElement, popupContent, xPos, yPos, 
                                type.get(), anchorAlignment.get(), 
                                popupAlignment.get());
    }
  }

  return NS_OK;
}


nsresult
NS_NewXULPopupListener(nsIXULPopupListener** pop)
{
    XULPopupListenerImpl* popup = new XULPopupListenerImpl();
    if (!popup)
      return NS_ERROR_OUT_OF_MEMORY;
    
    NS_ADDREF(popup);
    *pop = popup;
    return NS_OK;
}

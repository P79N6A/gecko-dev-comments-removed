





































#include "nsXMLElement.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIRefreshURI.h"
#include "nsPresContext.h"
#include "nsContentErrors.h"
#include "nsIDocument.h"

nsresult
NS_NewXMLElement(nsIContent** aInstancePtrResult, nsINodeInfo *aNodeInfo)
{
  nsXMLElement* it = new nsXMLElement(aNodeInfo);
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aInstancePtrResult = it);

  return NS_OK;
}

nsXMLElement::nsXMLElement(nsINodeInfo *aNodeInfo)
  : nsGenericElement(aNodeInfo)
{
}



NS_IMETHODIMP 
nsXMLElement::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_PRECONDITION(aInstancePtr, "null out param");

  nsresult rv = nsGenericElement::QueryInterface(aIID, aInstancePtr);

  if (NS_SUCCEEDED(rv))
    return rv;

  nsISupports *inst = nsnull;

  if (aIID.Equals(NS_GET_IID(nsIDOMNode))) {
    inst = static_cast<nsIDOMNode *>(this);
  } else if (aIID.Equals(NS_GET_IID(nsIDOMElement))) {
    inst = static_cast<nsIDOMElement *>(this);
  } else if (aIID.Equals(NS_GET_IID(nsIClassInfo))) {
    inst = NS_GetDOMClassInfoInstance(eDOMClassInfo_Element_id);
    NS_ENSURE_TRUE(inst, NS_ERROR_OUT_OF_MEMORY);
  } else {
    return PostQueryInterface(aIID, aInstancePtr);
  }

  NS_ADDREF(inst);

  *aInstancePtr = inst;

  return NS_OK;
}


NS_IMPL_ADDREF_INHERITED(nsXMLElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsXMLElement, nsGenericElement)


static nsresult
DocShellToPresContext(nsIDocShell *aShell, nsPresContext **aPresContext)
{
  *aPresContext = nsnull;

  nsresult rv;
  nsCOMPtr<nsIDocShell> ds = do_QueryInterface(aShell,&rv);
  if (NS_FAILED(rv))
    return rv;

  return ds->GetPresContext(aPresContext);
}

nsresult
nsXMLElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  nsresult rv = nsGenericElement::PreHandleEvent(aVisitor);
  NS_ENSURE_SUCCESS(rv, rv);

  return PreHandleEventForLinks(aVisitor);
}

nsresult
nsXMLElement::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  return PostHandleEventForLinks(aVisitor);
}

nsresult
nsXMLElement::MaybeTriggerAutoLink(nsIDocShell *aShell)
{
  NS_ENSURE_ARG_POINTER(aShell);

  
  
  if (!HasAttr(kNameSpaceID_XLink, nsGkAtoms::href) ||
      !AttrValueIs(kNameSpaceID_XLink, nsGkAtoms::type,
                   nsGkAtoms::simple, eCaseMatters) ||
      !AttrValueIs(kNameSpaceID_XLink, nsGkAtoms::actuate,
                   nsGkAtoms::onLoad, eCaseMatters)) {
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIDocShellTreeItem> docShellItem = do_QueryInterface(aShell);
  if (docShellItem) {
    nsCOMPtr<nsIDocShellTreeItem> rootItem;
    docShellItem->GetRootTreeItem(getter_AddRefs(rootItem));
    nsCOMPtr<nsIDocShell> docshell = do_QueryInterface(rootItem);
    if (docshell) {
      PRUint32 appType;
      if (NS_SUCCEEDED(docshell->GetAppType(&appType)) &&
          appType == nsIDocShell::APP_TYPE_MAIL) {
        return NS_OK;
      }
    }
  }

  
  nsCOMPtr<nsIURI> absURI;
  nsAutoString href;
  GetAttr(kNameSpaceID_XLink, nsGkAtoms::href, href);
  nsCOMPtr<nsIURI> baseURI = GetBaseURI();
  nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(absURI), href,
                                            GetOwnerDoc(), baseURI);
  if (!absURI) {
    return NS_OK;
  }

  
  
  PRBool isDocURI;
  absURI->Equals(GetOwnerDoc()->GetDocumentURI(), &isDocURI);
  if (isDocURI) {
    return NS_OK;
  }

  
  nsAutoString target;
  nsresult special_rv = GetLinkTargetAndAutoType(target);
  
  if (NS_FAILED(special_rv)) return NS_OK;

  
  nsCOMPtr<nsPresContext> pc;
  nsresult rv = DocShellToPresContext(aShell, getter_AddRefs(pc));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = TriggerLink(pc, absURI, target, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return special_rv; 
}

PRBool
nsXMLElement::IsFocusable(PRInt32 *aTabIndex)
{
  nsCOMPtr<nsIURI> absURI;
  if (IsLink(getter_AddRefs(absURI))) {
    if (aTabIndex) {
      *aTabIndex = ((sTabFocusModel & eTabFocus_linksMask) == 0 ? -1 : 0);
    }
    return PR_TRUE;
  }

  if (aTabIndex) {
    *aTabIndex = -1;
  }

  return PR_FALSE;
}

PRBool
nsXMLElement::IsLink(nsIURI** aURI) const
{
  NS_PRECONDITION(aURI, "Must provide aURI out param");

  
  
  
  
  
  
  
  
  
  

  static nsIContent::AttrValuesArray sShowVals[] =
    { &nsGkAtoms::_empty, &nsGkAtoms::_new, &nsGkAtoms::replace, nsnull };

  static nsIContent::AttrValuesArray sActuateVals[] =
    { &nsGkAtoms::_empty, &nsGkAtoms::onRequest, nsnull };

  
  const nsAttrValue* href = mAttrsAndChildren.GetAttr(nsGkAtoms::href,
                                                      kNameSpaceID_XLink);
  if (href &&
      AttrValueIs(kNameSpaceID_XLink, nsGkAtoms::type,
                  nsGkAtoms::simple, eCaseMatters) &&
      (HasAttr(kNameSpaceID_XLink, nsGkAtoms::_moz_target) ||
       FindAttrValueIn(kNameSpaceID_XLink, nsGkAtoms::show,
                       sShowVals, eCaseMatters) !=
                       nsIContent::ATTR_VALUE_NO_MATCH) &&
      FindAttrValueIn(kNameSpaceID_XLink, nsGkAtoms::actuate,
                      sActuateVals, eCaseMatters) !=
                      nsIContent::ATTR_VALUE_NO_MATCH) {
    
    nsCOMPtr<nsIURI> baseURI = GetBaseURI();
    nsContentUtils::NewURIWithDocumentCharset(aURI, href->GetStringValue(),
                                              GetOwnerDoc(), baseURI);
    return !!*aURI; 
  }

  *aURI = nsnull;
  return PR_FALSE;
}

void
nsXMLElement::GetLinkTarget(nsAString& aTarget)
{
  GetLinkTargetAndAutoType(aTarget);
}

nsresult
nsXMLElement::GetLinkTargetAndAutoType(nsAString& aTarget)
{
  
  if (GetAttr(kNameSpaceID_XLink, nsGkAtoms::_moz_target, aTarget)) {
    return aTarget.IsEmpty() ? NS_XML_AUTOLINK_REPLACE : NS_OK;
  }

  
  GetAttr(kNameSpaceID_XLink, nsGkAtoms::show, aTarget);
  if (aTarget.IsEmpty()) {
    return NS_XML_AUTOLINK_UNDEFINED;
  }
  if (aTarget.EqualsLiteral("new")) {
    aTarget.AssignLiteral("_blank");
    return NS_XML_AUTOLINK_NEW;
  }
  if (aTarget.EqualsLiteral("replace")) {
    aTarget.Truncate();
    return NS_XML_AUTOLINK_REPLACE;
  }
  

  aTarget.Truncate();
  return NS_ERROR_FAILURE;
}


NS_IMPL_ELEMENT_CLONE(nsXMLElement)







































#include "nsXMLElement.h"

nsresult
NS_NewXMLElement(nsIContent** aInstancePtrResult, already_AddRefed<nsINodeInfo> aNodeInfo)
{
  nsXMLElement* it = new nsXMLElement(aNodeInfo);
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aInstancePtrResult = it);

  return NS_OK;
}

DOMCI_NODE_DATA(Element, nsXMLElement)


NS_INTERFACE_TABLE_HEAD(nsXMLElement)
  NS_NODE_OFFSET_AND_INTERFACE_TABLE_BEGIN(nsXMLElement)
    NS_INTERFACE_TABLE_ENTRY(nsXMLElement, nsIDOMNode)
    NS_INTERFACE_TABLE_ENTRY(nsXMLElement, nsIDOMElement)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_ELEMENT_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Element)
NS_ELEMENT_INTERFACE_MAP_END

NS_IMPL_ADDREF_INHERITED(nsXMLElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsXMLElement, nsGenericElement)

NS_IMPL_ELEMENT_CLONE(nsXMLElement)

nsresult
nsXMLElement::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                        PRBool aNotify)
{
  nsAutoRemovableScriptBlocker scriptBlocker;
  PRBool isId = PR_FALSE;
  if (aAttribute == GetIDAttributeName() &&
      aNameSpaceID == kNameSpaceID_None) {
    
    RemoveFromIdTable();
    isId = PR_TRUE;
  }

  nsMutationGuard guard;

  nsresult rv = nsGenericElement::UnsetAttr(aNameSpaceID, aAttribute, aNotify);

  if (isId &&
      (!guard.Mutated(0) ||
       !mNodeInfo->GetIDAttributeAtom() ||
       !HasAttr(kNameSpaceID_None, GetIDAttributeName()))) {
    ClearHasID();
  }
  
  return rv;
}

nsIAtom *
nsXMLElement::GetIDAttributeName() const
{
  return mNodeInfo->GetIDAttributeAtom();
}

nsIAtom*
nsXMLElement::DoGetID() const
{
  NS_ASSERTION(HasID(), "Unexpected call");

  const nsAttrValue* attrVal = mAttrsAndChildren.GetAttr(GetIDAttributeName());
  return attrVal ? attrVal->GetAtomValue() : nsnull;
}

void
nsXMLElement::NodeInfoChanged(nsINodeInfo* aOldNodeInfo)
{
  NS_ASSERTION(!IsInDoc() ||
               aOldNodeInfo->GetDocument() == mNodeInfo->GetDocument(),
               "Can only change document if we're not inside one");
  nsIDocument* doc = GetCurrentDoc();

  if (HasID() && doc) {
    const nsAttrValue* attrVal =
      mAttrsAndChildren.GetAttr(aOldNodeInfo->GetIDAttributeAtom());
    if (attrVal) {
      doc->RemoveFromIdTable(this, attrVal->GetAtomValue());
    }
  }
  
  ClearHasID();

  nsIAtom* IDName = GetIDAttributeName();
  if (IDName) {
    const nsAttrValue* attrVal = mAttrsAndChildren.GetAttr(IDName);
    if (attrVal) {
      SetHasID();
      if (attrVal->Type() == nsAttrValue::eString) {
        nsString idVal(attrVal->GetStringValue());

        
        const_cast<nsAttrValue*>(attrVal)->ParseAtom(idVal);
      }
      NS_ASSERTION(attrVal->Type() == nsAttrValue::eAtom,
                   "Should be atom by now");
      if (doc) {
        doc->AddToIdTable(this, attrVal->GetAtomValue());
      }
    }
  }
}

PRBool
nsXMLElement::ParseAttribute(PRInt32 aNamespaceID,
                             nsIAtom* aAttribute,
                             const nsAString& aValue,
                             nsAttrValue& aResult)
{
  if (aAttribute == GetIDAttributeName() &&
      aNamespaceID == kNameSpaceID_None) {
    
    
    RemoveFromIdTable();
    if (aValue.IsEmpty()) {
      ClearHasID();
      return PR_FALSE;
    }
    aResult.ParseAtom(aValue);
    SetHasID();
    AddToIdTable(aResult.GetAtomValue());
    return PR_TRUE;
  }

  return PR_FALSE;
}

nsresult
nsXMLElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                         nsIContent* aBindingParent,
                         PRBool aCompileEventHandlers)
{
  nsresult rv = nsGenericElement::BindToTree(aDocument, aParent,
                                             aBindingParent,
                                             aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDocument && HasID() && !GetBindingParent()) {
    aDocument->AddToIdTable(this, DoGetID());
  }

  return NS_OK;
}

void
nsXMLElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  RemoveFromIdTable();

  return nsGenericElement::UnbindFromTree(aDeep, aNullParent);
}

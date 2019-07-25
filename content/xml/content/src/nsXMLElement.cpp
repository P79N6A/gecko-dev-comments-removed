





































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
  PRBool isId = PR_FALSE;
  if (aAttribute == GetIDAttributeName() &&
      aNameSpaceID == kNameSpaceID_None) {
    
    RemoveFromIdTable();
    isId = PR_TRUE;
  }
  
  nsresult rv = nsGenericElement::UnsetAttr(aNameSpaceID, aAttribute, aNotify);
  
  if (isId) {
    UnsetFlags(NODE_HAS_ID);
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
  NS_ASSERTION(HasFlag(NODE_HAS_ID), "Unexpected call");

  nsIAtom* IDName = GetIDAttributeName();
  if (IDName) {
    const nsAttrValue* attrVal = mAttrsAndChildren.GetAttr(IDName);
    if (attrVal) {
      if (attrVal->Type() == nsAttrValue::eAtom) {
        return attrVal->GetAtomValue();
      }
      if (attrVal->IsEmptyString()) {
        return nsnull;
      }
      
      
      
      if (attrVal->Type() == nsAttrValue::eString) {
        nsAutoString idVal(attrVal->GetStringValue());

        
        const_cast<nsAttrValue*>(attrVal)->ParseAtom(idVal);
        return attrVal->GetAtomValue();
      }
    }
  }
  return nsnull;
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
      UnsetFlags(NODE_HAS_ID);
      return PR_FALSE;
    }
    aResult.ParseAtom(aValue);
    SetFlags(NODE_HAS_ID);
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

  if (aDocument && HasFlag(NODE_HAS_ID) && !GetBindingParent()) {
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

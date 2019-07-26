




#include "nsXMLElement.h"
#include "mozilla/dom/ElementBinding.h"
#include "mozilla/dom/ElementInlines.h"
#include "nsContentUtils.h" 

using namespace mozilla::dom;

nsresult
NS_NewXMLElement(Element** aInstancePtrResult,
                 already_AddRefed<nsINodeInfo>&& aNodeInfo)
{
  nsXMLElement* it = new nsXMLElement(aNodeInfo);
  NS_ADDREF(*aInstancePtrResult = it);
  return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED(nsXMLElement, Element,
                            nsIDOMNode, nsIDOMElement)

JSObject*
nsXMLElement::WrapNode(JSContext *aCx)
{
  return ElementBinding::Wrap(aCx, this);
}

NS_IMPL_ELEMENT_CLONE(nsXMLElement)

nsresult
nsXMLElement::UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                        bool aNotify)
{
  nsAutoScriptBlocker scriptBlocker;
  bool isId = false;
  if (aAttribute == GetIDAttributeName() &&
      aNameSpaceID == kNameSpaceID_None) {
    
    RemoveFromIdTable();
    isId = true;
  }

  nsMutationGuard guard;

  nsresult rv = Element::UnsetAttr(aNameSpaceID, aAttribute, aNotify);

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
  return attrVal ? attrVal->GetAtomValue() : nullptr;
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
      RemoveFromIdTable(attrVal->GetAtomValue());
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
        AddToIdTable(attrVal->GetAtomValue());
      }
    }
  }
}

bool
nsXMLElement::ParseAttribute(int32_t aNamespaceID,
                             nsIAtom* aAttribute,
                             const nsAString& aValue,
                             nsAttrValue& aResult)
{
  if (aAttribute == GetIDAttributeName() &&
      aNamespaceID == kNameSpaceID_None) {
    
    
    RemoveFromIdTable();
    if (aValue.IsEmpty()) {
      ClearHasID();
      return false;
    }
    aResult.ParseAtom(aValue);
    SetHasID();
    AddToIdTable(aResult.GetAtomValue());
    return true;
  }

  return false;
}

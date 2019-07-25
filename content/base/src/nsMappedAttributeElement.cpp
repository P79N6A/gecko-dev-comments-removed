







































#include "nsMappedAttributeElement.h"
#include "nsIDocument.h"

nsresult
nsMappedAttributeElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                                     nsIContent* aBindingParent,
                                     bool aCompileEventHandlers)
{
  nsresult rv = nsMappedAttributeElementBase::BindToTree(aDocument, aParent,
                                                         aBindingParent,
                                                         aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDocument) {
    
    
    nsHTMLStyleSheet* sheet = aDocument->GetAttributeStyleSheet();
    if (sheet) {
      mAttrsAndChildren.SetMappedAttrStyleSheet(sheet);
    }
  }

  return rv;
}

nsresult
nsMappedAttributeElement::WalkContentStyleRules(nsRuleWalker* aRuleWalker)
{
  mAttrsAndChildren.WalkMappedAttributeStyleRules(aRuleWalker);
  return NS_OK;
}

bool
nsMappedAttributeElement::SetMappedAttribute(nsIDocument* aDocument,
                                             nsIAtom* aName,
                                             nsAttrValue& aValue,
                                             nsresult* aRetval)
{
  NS_PRECONDITION(aDocument == GetCurrentDoc(), "Unexpected document");
  nsHTMLStyleSheet* sheet = aDocument ?
    aDocument->GetAttributeStyleSheet() : nsnull;

  *aRetval = mAttrsAndChildren.SetAndTakeMappedAttr(aName, aValue,
                                                    this, sheet);
  return true;
}

nsMapRuleToAttributesFunc
nsMappedAttributeElement::GetAttributeMappingFunction() const
{
  return &MapNoAttributesInto;
}

void
nsMappedAttributeElement::MapNoAttributesInto(const nsMappedAttributes* aAttributes,
                                              nsRuleData* aData)
{
}

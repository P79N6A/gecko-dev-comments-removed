





#include "nsMappedAttributeElement.h"
#include "nsIDocument.h"

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

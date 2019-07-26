




#include "HTMLElementAccessibles.h"

#include "DocAccessible.h"
#include "nsAccUtils.h"
#include "nsIAccessibleRelation.h"
#include "nsTextEquivUtils.h"
#include "Relation.h"
#include "Role.h"
#include "States.h"

using namespace mozilla::a11y;





role
HTMLHRAccessible::NativeRole()
{
  return roles::SEPARATOR;
}





role
HTMLBRAccessible::NativeRole()
{
  return roles::WHITESPACE;
}

uint64_t
HTMLBRAccessible::NativeState()
{
  return states::READONLY;
}

ENameValueFlag
HTMLBRAccessible::NativeName(nsString& aName)
{
  aName = static_cast<PRUnichar>('\n');    
  return eNameOK;
}





NS_IMPL_ISUPPORTS_INHERITED0(HTMLLabelAccessible, HyperTextAccessible)

ENameValueFlag
HTMLLabelAccessible::NativeName(nsString& aName)
{
  nsTextEquivUtils::GetNameFromSubtree(this, aName);
  return aName.IsEmpty() ? eNameOK : eNameFromSubtree;
}

role
HTMLLabelAccessible::NativeRole()
{
  return roles::LABEL;
}





NS_IMPL_ISUPPORTS_INHERITED0(HTMLOutputAccessible, HyperTextAccessible)

Relation
HTMLOutputAccessible::RelationByType(uint32_t aType)
{
  Relation rel = AccessibleWrap::RelationByType(aType);
  if (aType == nsIAccessibleRelation::RELATION_CONTROLLED_BY)
    rel.AppendIter(new IDRefsIterator(mDoc, mContent, nsGkAtoms::_for));

  return rel;
}

role
HTMLOutputAccessible::NativeRole()
{
  return roles::SECTION;
}

nsresult
HTMLOutputAccessible::GetAttributesInternal(nsIPersistentProperties* aAttributes)
{
  nsresult rv = AccessibleWrap::GetAttributesInternal(aAttributes);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::live,
                         NS_LITERAL_STRING("polite"));

  return NS_OK;
}


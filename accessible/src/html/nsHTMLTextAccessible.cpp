




#include "nsHTMLTextAccessible.h"

#include "DocAccessible.h"
#include "nsAccUtils.h"
#include "nsIAccessibleRelation.h"
#include "nsTextEquivUtils.h"
#include "Relation.h"
#include "Role.h"
#include "States.h"

using namespace mozilla::a11y;





nsHTMLHRAccessible::
  nsHTMLHRAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  nsLeafAccessible(aContent, aDoc)
{
}

role
nsHTMLHRAccessible::NativeRole()
{
  return roles::SEPARATOR;
}






nsHTMLBRAccessible::
  nsHTMLBRAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  nsLeafAccessible(aContent, aDoc)
{
}

role
nsHTMLBRAccessible::NativeRole()
{
  return roles::WHITESPACE;
}

PRUint64
nsHTMLBRAccessible::NativeState()
{
  return states::READONLY;
}

nsresult
nsHTMLBRAccessible::GetNameInternal(nsAString& aName)
{
  aName = static_cast<PRUnichar>('\n');    
  return NS_OK;
}





nsHTMLLabelAccessible::
  nsHTMLLabelAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  HyperTextAccessibleWrap(aContent, aDoc)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLLabelAccessible, HyperTextAccessible)

nsresult
nsHTMLLabelAccessible::GetNameInternal(nsAString& aName)
{
  return nsTextEquivUtils::GetNameFromSubtree(this, aName);
}

role
nsHTMLLabelAccessible::NativeRole()
{
  return roles::LABEL;
}





nsHTMLOutputAccessible::
  nsHTMLOutputAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  HyperTextAccessibleWrap(aContent, aDoc)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLOutputAccessible, HyperTextAccessible)

Relation
nsHTMLOutputAccessible::RelationByType(PRUint32 aType)
{
  Relation rel = AccessibleWrap::RelationByType(aType);
  if (aType == nsIAccessibleRelation::RELATION_CONTROLLED_BY)
    rel.AppendIter(new IDRefsIterator(mDoc, mContent, nsGkAtoms::_for));

  return rel;
}

role
nsHTMLOutputAccessible::NativeRole()
{
  return roles::SECTION;
}

nsresult
nsHTMLOutputAccessible::GetAttributesInternal(nsIPersistentProperties* aAttributes)
{
  nsresult rv = AccessibleWrap::GetAttributesInternal(aAttributes);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::live,
                         NS_LITERAL_STRING("polite"));
  
  return NS_OK;
}


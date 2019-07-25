




#include "XULElementAccessibles.h"

#include "Accessible-inl.h"
#include "BaseAccessibles.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "nsTextEquivUtils.h"
#include "Relation.h"
#include "Role.h"
#include "States.h"

#include "nsIAccessibleRelation.h"
#include "nsIDOMXULDescriptionElement.h"
#include "nsINameSpaceManager.h"
#include "nsString.h"
#include "nsNetUtil.h"

using namespace mozilla::a11y;





XULLabelAccessible::
  XULLabelAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  HyperTextAccessibleWrap(aContent, aDoc)
{
}

nsresult
XULLabelAccessible::GetNameInternal(nsAString& aName)
{
  
  
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::value, aName);
  return NS_OK;
}

role
XULLabelAccessible::NativeRole()
{
  return roles::LABEL;
}

uint64_t
XULLabelAccessible::NativeState()
{
  
  
  return HyperTextAccessibleWrap::NativeState() | states::READONLY;
}

Relation
XULLabelAccessible::RelationByType(uint32_t aType)
{
  Relation rel = HyperTextAccessibleWrap::RelationByType(aType);
  if (aType == nsIAccessibleRelation::RELATION_LABEL_FOR) {
    
    nsIContent *parent = mContent->GetParent();
    if (parent && parent->Tag() == nsGkAtoms::caption) {
      Accessible* parent = Parent();
      if (parent && parent->Role() == roles::GROUPING)
        rel.AppendTarget(parent);
    }
  }

  return rel;
}






XULTooltipAccessible::
  XULTooltipAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  LeafAccessible(aContent, aDoc)
{
}

uint64_t
XULTooltipAccessible::NativeState()
{
  return LeafAccessible::NativeState() | states::READONLY;
}

role
XULTooltipAccessible::NativeRole()
{
  return roles::TOOLTIP;
}






XULLinkAccessible::
  XULLinkAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  HyperTextAccessibleWrap(aContent, aDoc)
{
}


NS_IMPL_ISUPPORTS_INHERITED1(XULLinkAccessible, HyperTextAccessibleWrap,
                             nsIAccessibleHyperLink)




void
XULLinkAccessible::Value(nsString& aValue)
{
  aValue.Truncate();

  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::href, aValue);
}

nsresult
XULLinkAccessible::GetNameInternal(nsAString& aName)
{
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::value, aName);
  if (!aName.IsEmpty())
    return NS_OK;

  return nsTextEquivUtils::GetNameFromSubtree(this, aName);
}

role
XULLinkAccessible::NativeRole()
{
  return roles::LINK;
}


uint64_t
XULLinkAccessible::NativeLinkState() const
{
  return states::LINKED;
}

uint8_t
XULLinkAccessible::ActionCount()
{
  return 1;
}

NS_IMETHODIMP
XULLinkAccessible::GetActionName(uint8_t aIndex, nsAString& aName)
{
  aName.Truncate();

  if (aIndex != eAction_Jump)
    return NS_ERROR_INVALID_ARG;

  aName.AssignLiteral("jump");
  return NS_OK;
}

NS_IMETHODIMP
XULLinkAccessible::DoAction(uint8_t aIndex)
{
  if (aIndex != eAction_Jump)
    return NS_ERROR_INVALID_ARG;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  DoCommand();
  return NS_OK;
}




bool
XULLinkAccessible::IsLink()
{
  
  return true;
}

uint32_t
XULLinkAccessible::StartOffset()
{
  
  
  
  
  
  if (Accessible::IsLink())
    return Accessible::StartOffset();
  return IndexInParent();
}

uint32_t
XULLinkAccessible::EndOffset()
{
  if (Accessible::IsLink())
    return Accessible::EndOffset();
  return IndexInParent() + 1;
}

already_AddRefed<nsIURI>
XULLinkAccessible::AnchorURIAt(uint32_t aAnchorIndex)
{
  if (aAnchorIndex != 0)
    return nullptr;

  nsAutoString href;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::href, href);

  nsCOMPtr<nsIURI> baseURI = mContent->GetBaseURI();
  nsIDocument* document = mContent->OwnerDoc();

  nsIURI* anchorURI = nullptr;
  NS_NewURI(&anchorURI, href,
            document->GetDocumentCharacterSet().get(),
            baseURI);

  return anchorURI;
}






#include "XULElementAccessibles.h"

#include "Accessible-inl.h"
#include "BaseAccessibles.h"
#include "DocAccessible-inl.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "nsTextEquivUtils.h"
#include "Relation.h"
#include "Role.h"
#include "States.h"
#include "TextUpdater.h"

#ifdef A11Y_LOG
#include "Logging.h"
#endif

#include "nsIAccessibleRelation.h"
#include "nsIDOMXULDescriptionElement.h"
#include "nsNameSpaceManager.h"
#include "nsNetUtil.h"
#include "nsString.h"
#include "nsTextBoxFrame.h"

using namespace mozilla::a11y;





XULLabelAccessible::
  XULLabelAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  HyperTextAccessibleWrap(aContent, aDoc)
{
  mType = eXULLabelType;

  
  
  
  
  nsTextBoxFrame* textBoxFrame = do_QueryFrame(mContent->GetPrimaryFrame());
  if (textBoxFrame) {
    mValueTextLeaf = new XULLabelTextLeafAccessible(mContent, mDoc);
    mDoc->BindToDocument(mValueTextLeaf, nullptr);

    nsAutoString text;
    textBoxFrame->GetCroppedTitle(text);
    mValueTextLeaf->SetText(text);
  }
}

void
XULLabelAccessible::Shutdown()
{
  mValueTextLeaf = nullptr;
  HyperTextAccessibleWrap::Shutdown();
}

ENameValueFlag
XULLabelAccessible::NativeName(nsString& aName)
{
  
  
  if (mValueTextLeaf)
    return mValueTextLeaf->Name(aName);

  return eNameOK;
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
XULLabelAccessible::RelationByType(RelationType aType)
{
  Relation rel = HyperTextAccessibleWrap::RelationByType(aType);
  if (aType == RelationType::LABEL_FOR) {
    
    nsIContent* parent = mContent->GetFlattenedTreeParent();
    if (parent && parent->Tag() == nsGkAtoms::caption) {
      Accessible* parent = Parent();
      if (parent && parent->Role() == roles::GROUPING)
        rel.AppendTarget(parent);
    }
  }

  return rel;
}

void
XULLabelAccessible::UpdateLabelValue(const nsString& aValue)
{
#ifdef A11Y_LOG
  if (logging::IsEnabled(logging::eText)) {
    logging::MsgBegin("TEXT", "text may be changed (xul:label @value update)");
    logging::Node("container", mContent);
    logging::MsgEntry("old text '%s'",
                      NS_ConvertUTF16toUTF8(mValueTextLeaf->Text()).get());
    logging::MsgEntry("new text: '%s'",
                      NS_ConvertUTF16toUTF8(aValue).get());
    logging::MsgEnd();
  }
#endif

  TextUpdater::Run(mDoc, mValueTextLeaf, aValue);
}

void
XULLabelAccessible::CacheChildren()
{
  if (mValueTextLeaf) {
    AppendChild(mValueTextLeaf);
    return;
  }

  
  AccessibleWrap::CacheChildren();
}






role
XULLabelTextLeafAccessible::NativeRole()
{
  return roles::TEXT_LEAF;
}

uint64_t
XULLabelTextLeafAccessible::NativeState()
{
  return TextLeafAccessibleWrap::NativeState() | states::READONLY;
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
  XULLabelAccessible(aContent, aDoc)
{
}

XULLinkAccessible::~XULLinkAccessible()
{
}


NS_IMPL_ISUPPORTS_INHERITED(XULLinkAccessible, XULLabelAccessible,
                            nsIAccessibleHyperLink)




void
XULLinkAccessible::Value(nsString& aValue)
{
  aValue.Truncate();

  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::href, aValue);
}

ENameValueFlag
XULLinkAccessible::NativeName(nsString& aName)
{
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::value, aName);
  if (!aName.IsEmpty())
    return eNameOK;

  nsTextEquivUtils::GetNameFromSubtree(this, aName);
  return aName.IsEmpty() ? eNameOK : eNameFromSubtree;
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

void
XULLinkAccessible::ActionNameAt(uint8_t aIndex, nsAString& aName)
{
  aName.Truncate();

  if (aIndex == eAction_Jump)
    aName.AssignLiteral("jump");
}

bool
XULLinkAccessible::DoAction(uint8_t aIndex)
{
  if (aIndex != eAction_Jump)
    return false;

  DoCommand();
  return true;
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

  nsCOMPtr<nsIURI> anchorURI;
  NS_NewURI(getter_AddRefs(anchorURI), href,
            document->GetDocumentCharacterSet().get(),
            baseURI);

  return anchorURI.forget();
}

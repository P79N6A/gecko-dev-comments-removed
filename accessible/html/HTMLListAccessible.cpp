





#include "HTMLListAccessible.h"

#include "DocAccessible.h"
#include "nsAccUtils.h"
#include "Role.h"
#include "States.h"

#include "nsBlockFrame.h"
#include "nsBulletFrame.h"

using namespace mozilla;
using namespace mozilla::a11y;





NS_IMPL_ISUPPORTS_INHERITED0(HTMLListAccessible, HyperTextAccessible)

role
HTMLListAccessible::NativeRole()
{
  if (mContent->Tag() == nsGkAtoms::dl)
    return roles::DEFINITION_LIST;

  return roles::LIST;
}

uint64_t
HTMLListAccessible::NativeState()
{
  return HyperTextAccessibleWrap::NativeState() | states::READONLY;
}






HTMLLIAccessible::
  HTMLLIAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  HyperTextAccessibleWrap(aContent, aDoc), mBullet(nullptr)
{
  mType = eHTMLLiType;

  nsBlockFrame* blockFrame = do_QueryFrame(GetFrame());
  if (blockFrame && blockFrame->HasBullet()) {
    mBullet = new HTMLListBulletAccessible(mContent, mDoc);
    Document()->BindToDocument(mBullet, nullptr);
  }
}

NS_IMPL_ISUPPORTS_INHERITED0(HTMLLIAccessible, HyperTextAccessible)

void
HTMLLIAccessible::Shutdown()
{
  mBullet = nullptr;

  HyperTextAccessibleWrap::Shutdown();
}

role
HTMLLIAccessible::NativeRole()
{
  if (mContent->Tag() == nsGkAtoms::dt)
    return roles::TERM;

  return roles::LISTITEM;
}

uint64_t
HTMLLIAccessible::NativeState()
{
  return HyperTextAccessibleWrap::NativeState() | states::READONLY;
}

nsIntRect
HTMLLIAccessible::Bounds() const
{
  nsIntRect rect = AccessibleWrap::Bounds();
  if (rect.IsEmpty() || !mBullet || mBullet->IsInside())
    return rect;

  nsIntRect bulletRect = mBullet->Bounds();

  rect.width += rect.x - bulletRect.x;
  rect.x = bulletRect.x; 
  return rect;
}




void
HTMLLIAccessible::UpdateBullet(bool aHasBullet)
{
  if (aHasBullet == !!mBullet) {
    NS_NOTREACHED("Bullet and accessible are in sync already!");
    return;
  }

  DocAccessible* document = Document();
  if (aHasBullet) {
    mBullet = new HTMLListBulletAccessible(mContent, mDoc);
    document->BindToDocument(mBullet, nullptr);
    InsertChildAt(0, mBullet);
  } else {
    RemoveChild(mBullet);
    document->UnbindFromDocument(mBullet);
    mBullet = nullptr;
  }

  
  
}




void
HTMLLIAccessible::CacheChildren()
{
  if (mBullet)
    AppendChild(mBullet);

  
  AccessibleWrap::CacheChildren();
}




HTMLListBulletAccessible::
  HTMLListBulletAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  LeafAccessible(aContent, aDoc)
{
  mStateFlags |= eSharedNode;
}




nsIFrame*
HTMLListBulletAccessible::GetFrame() const
{
  nsBlockFrame* blockFrame = do_QueryFrame(mContent->GetPrimaryFrame());
  return blockFrame ? blockFrame->GetBullet() : nullptr;
}

ENameValueFlag
HTMLListBulletAccessible::Name(nsString &aName)
{
  aName.Truncate();

  
  nsBlockFrame* blockFrame = do_QueryFrame(mContent->GetPrimaryFrame());
  if (blockFrame) {
    blockFrame->GetSpokenBulletText(aName);
  }

  return eNameOK;
}

role
HTMLListBulletAccessible::NativeRole()
{
  return roles::STATICTEXT;
}

uint64_t
HTMLListBulletAccessible::NativeState()
{
  return LeafAccessible::NativeState() | states::READONLY;
}

void
HTMLListBulletAccessible::AppendTextTo(nsAString& aText, uint32_t aStartOffset,
                                       uint32_t aLength)
{
  nsAutoString bulletText;
  nsBlockFrame* blockFrame = do_QueryFrame(mContent->GetPrimaryFrame());
  if (blockFrame)
    blockFrame->GetSpokenBulletText(bulletText);

  aText.Append(Substring(bulletText, aStartOffset, aLength));
}




bool
HTMLListBulletAccessible::IsInside() const
{
  nsBlockFrame* blockFrame = do_QueryFrame(mContent->GetPrimaryFrame());
  return blockFrame ? blockFrame->HasInsideBullet() : false;
}

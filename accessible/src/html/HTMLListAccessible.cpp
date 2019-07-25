





#include "HTMLListAccessible.h"

#include "DocAccessible.h"
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
  mFlags |= eHTMLListItemAccessible;

  nsBlockFrame* blockFrame = do_QueryFrame(GetFrame());
  if (blockFrame && blockFrame->HasBullet()) {
    mBullet = new HTMLListBulletAccessible(mContent, mDoc);
    if (!Document()->BindToDocument(mBullet, nullptr))
      mBullet = nullptr;
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

NS_IMETHODIMP
HTMLLIAccessible::GetBounds(int32_t* aX, int32_t* aY,
                            int32_t* aWidth, int32_t* aHeight)
{
  nsresult rv = AccessibleWrap::GetBounds(aX, aY, aWidth, aHeight);
  if (NS_FAILED(rv) || !mBullet || mBullet->IsInside())
    return rv;

  int32_t bulletX = 0, bulletY = 0, bulletWidth = 0, bulletHeight = 0;
  rv = mBullet->GetBounds(&bulletX, &bulletY, &bulletWidth, &bulletHeight);
  NS_ENSURE_SUCCESS(rv, rv);

  *aWidth += *aX - bulletX;
  *aX = bulletX; 
  return NS_OK;
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
    if (document->BindToDocument(mBullet, nullptr)) {
      InsertChildAt(0, mBullet);
    }
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








nsIFrame*
HTMLListBulletAccessible::GetFrame() const
{
  nsBlockFrame* blockFrame = do_QueryFrame(mContent->GetPrimaryFrame());
  return blockFrame ? blockFrame->GetBullet() : nullptr;
}

bool
HTMLListBulletAccessible::IsPrimaryForNode() const
{
  return false;
}




ENameValueFlag
HTMLListBulletAccessible::Name(nsString &aName)
{
  aName.Truncate();

  
  nsBlockFrame* blockFrame = do_QueryFrame(mContent->GetPrimaryFrame());
  NS_ASSERTION(blockFrame, "No frame for list item!");
  if (blockFrame) {
    blockFrame->GetBulletText(aName);

    
    aName.Append(' ');
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
  NS_ASSERTION(blockFrame, "No frame for list item!");
  if (blockFrame)
    blockFrame->GetBulletText(bulletText);

  aText.Append(Substring(bulletText, aStartOffset, aLength));
}




bool
HTMLListBulletAccessible::IsInside() const
{
  nsBlockFrame* blockFrame = do_QueryFrame(mContent->GetPrimaryFrame());
  return blockFrame ? blockFrame->HasInsideBullet() : false;
}

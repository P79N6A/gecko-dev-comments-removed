





#include "HTMLListAccessible.h"

#include "DocAccessible.h"
#include "Role.h"
#include "States.h"

#include "nsBlockFrame.h"
#include "nsBulletFrame.h"

using namespace mozilla;
using namespace mozilla::a11y;





NS_IMPL_ISUPPORTS_INHERITED0(HTMLListAccessible, nsHyperTextAccessible)

role
HTMLListAccessible::NativeRole()
{
  if (mContent->Tag() == nsGkAtoms::dl)
    return roles::DEFINITION_LIST;

  return roles::LIST;
}

PRUint64
HTMLListAccessible::NativeState()
{
  return nsHyperTextAccessibleWrap::NativeState() | states::READONLY;
}






HTMLLIAccessible::
  HTMLLIAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  nsHyperTextAccessibleWrap(aContent, aDoc), mBullet(nsnull)
{
  mFlags |= eHTMLListItemAccessible;

  nsBlockFrame* blockFrame = do_QueryFrame(GetFrame());
  if (blockFrame && blockFrame->HasBullet()) {
    mBullet = new HTMLListBulletAccessible(mContent, mDoc);
    if (!Document()->BindToDocument(mBullet, nsnull))
      mBullet = nsnull;
  }
}

NS_IMPL_ISUPPORTS_INHERITED0(HTMLLIAccessible, nsHyperTextAccessible)

void
HTMLLIAccessible::Shutdown()
{
  mBullet = nsnull;

  nsHyperTextAccessibleWrap::Shutdown();
}

role
HTMLLIAccessible::NativeRole()
{
  if (mContent->Tag() == nsGkAtoms::dt)
    return roles::TERM;

  return roles::LISTITEM;
}

PRUint64
HTMLLIAccessible::NativeState()
{
  return nsHyperTextAccessibleWrap::NativeState() | states::READONLY;
}

NS_IMETHODIMP
HTMLLIAccessible::GetBounds(PRInt32* aX, PRInt32* aY,
                            PRInt32* aWidth, PRInt32* aHeight)
{
  nsresult rv = AccessibleWrap::GetBounds(aX, aY, aWidth, aHeight);
  if (NS_FAILED(rv) || !mBullet || mBullet->IsInside())
    return rv;

  PRInt32 bulletX = 0, bulletY = 0, bulletWidth = 0, bulletHeight = 0;
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
    if (document->BindToDocument(mBullet, nsnull)) {
      InsertChildAt(0, mBullet);
    }
  } else {
    RemoveChild(mBullet);
    document->UnbindFromDocument(mBullet);
    mBullet = nsnull;
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
  return blockFrame ? blockFrame->GetBullet() : nsnull;
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

PRUint64
HTMLListBulletAccessible::NativeState()
{
  PRUint64 state = nsLeafAccessible::NativeState();

  state &= ~states::FOCUSABLE;
  state |= states::READONLY;
  return state;
}

void
HTMLListBulletAccessible::AppendTextTo(nsAString& aText, PRUint32 aStartOffset,
                                       PRUint32 aLength)
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

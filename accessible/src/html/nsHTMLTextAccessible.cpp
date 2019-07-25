






































#include "nsHTMLTextAccessible.h"

#include "nsDocAccessible.h"
#include "nsAccUtils.h"
#include "nsRelUtils.h"
#include "nsTextEquivUtils.h"
#include "States.h"

#include "nsIFrame.h"
#include "nsPresContext.h"
#include "nsBlockFrame.h"
#include "nsISelection.h"
#include "nsISelectionController.h"
#include "nsComponentManagerUtils.h"





nsHTMLTextAccessible::
  nsHTMLTextAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsTextAccessibleWrap(aContent, aShell)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLTextAccessible, nsTextAccessible)

NS_IMETHODIMP
nsHTMLTextAccessible::GetName(nsAString& aName)
{
  
  aName = mText;
  return NS_OK;
}

PRUint32
nsHTMLTextAccessible::NativeRole()
{
  nsIFrame *frame = GetFrame();
  
  
  if (frame && frame->IsGeneratedContentFrame()) {
    return nsIAccessibleRole::ROLE_STATICTEXT;
  }

  return nsTextAccessible::NativeRole();
}

PRUint64
nsHTMLTextAccessible::NativeState()
{
  PRUint64 state = nsTextAccessible::NativeState();

  nsDocAccessible *docAccessible = GetDocAccessible();
  if (docAccessible) {
     PRUint64 docState = docAccessible->State();
     if (0 == (docState & states::EDITABLE)) {
       state |= states::READONLY; 
     }
  }

  return state;
}

nsresult
nsHTMLTextAccessible::GetAttributesInternal(nsIPersistentProperties *aAttributes)
{
  if (NativeRole() == nsIAccessibleRole::ROLE_STATICTEXT) {
    nsAutoString oldValueUnused;
    aAttributes->SetStringProperty(NS_LITERAL_CSTRING("auto-generated"),
                                  NS_LITERAL_STRING("true"), oldValueUnused);
  }

  return NS_OK;
}






nsHTMLHRAccessible::
  nsHTMLHRAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsLeafAccessible(aContent, aShell)
{
}

PRUint32
nsHTMLHRAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_SEPARATOR;
}






nsHTMLBRAccessible::
  nsHTMLBRAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsLeafAccessible(aContent, aShell)
{
}

PRUint32
nsHTMLBRAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_WHITESPACE;
}

PRUint64
nsHTMLBRAccessible::NativeState()
{
  return IsDefunct() ? states::DEFUNCT : states::READONLY;
}

nsresult
nsHTMLBRAccessible::GetNameInternal(nsAString& aName)
{
  aName = static_cast<PRUnichar>('\n');    
  return NS_OK;
}





nsHTMLLabelAccessible::
  nsHTMLLabelAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsHyperTextAccessibleWrap(aContent, aShell)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLLabelAccessible, nsHyperTextAccessible)

nsresult
nsHTMLLabelAccessible::GetNameInternal(nsAString& aName)
{
  return nsTextEquivUtils::GetNameFromSubtree(this, aName);
}

PRUint32
nsHTMLLabelAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_LABEL;
}





nsHTMLOutputAccessible::
  nsHTMLOutputAccessible(nsIContent* aContent, nsIWeakReference* aShell) :
  nsHyperTextAccessibleWrap(aContent, aShell)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLOutputAccessible, nsHyperTextAccessible)

NS_IMETHODIMP
nsHTMLOutputAccessible::GetRelationByType(PRUint32 aRelationType,
                                          nsIAccessibleRelation** aRelation)
{
  nsresult rv = nsAccessibleWrap::GetRelationByType(aRelationType, aRelation);
  NS_ENSURE_SUCCESS(rv, rv);

  if (rv != NS_OK_NO_RELATION_TARGET)
    return NS_OK; 

  if (aRelationType == nsIAccessibleRelation::RELATION_CONTROLLED_BY) {
    return nsRelUtils::
      AddTargetFromIDRefsAttr(aRelationType, aRelation, mContent,
                              nsAccessibilityAtoms::_for);
  }

  return NS_OK;
}

PRUint32
nsHTMLOutputAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_SECTION;
}

nsresult
nsHTMLOutputAccessible::GetAttributesInternal(nsIPersistentProperties* aAttributes)
{
  nsresult rv = nsAccessibleWrap::GetAttributesInternal(aAttributes);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAccUtils::SetAccAttr(aAttributes, nsAccessibilityAtoms::live,
                         NS_LITERAL_STRING("polite"));
  
  return NS_OK;
}






nsHTMLLIAccessible::
  nsHTMLLIAccessible(nsIContent* aContent, nsIWeakReference* aShell) :
  nsHyperTextAccessibleWrap(aContent, aShell), mBullet(nsnull)
{
  mFlags |= eHTMLListItemAccessible;

  nsBlockFrame* blockFrame = do_QueryFrame(GetFrame());
  if (blockFrame && blockFrame->HasBullet()) {
    mBullet = new nsHTMLListBulletAccessible(mContent, mWeakShell);
    if (!GetDocAccessible()->BindToDocument(mBullet, nsnull))
      mBullet = nsnull;
  }
}

NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLLIAccessible, nsHyperTextAccessible)

void
nsHTMLLIAccessible::Shutdown()
{
  mBullet = nsnull;

  nsHyperTextAccessibleWrap::Shutdown();
}

PRUint32
nsHTMLLIAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_LISTITEM;
}

PRUint64
nsHTMLLIAccessible::NativeState()
{
  return nsHyperTextAccessibleWrap::NativeState() | states::READONLY;
}

NS_IMETHODIMP nsHTMLLIAccessible::GetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height)
{
  nsresult rv = nsAccessibleWrap::GetBounds(x, y, width, height);
  if (NS_FAILED(rv) || !mBullet)
    return rv;

  PRInt32 bulletX, bulletY, bulletWidth, bulletHeight;
  rv = mBullet->GetBounds(&bulletX, &bulletY, &bulletWidth, &bulletHeight);
  NS_ENSURE_SUCCESS(rv, rv);

  *x = bulletX; 
  *width += bulletWidth;
  return NS_OK;
}




void
nsHTMLLIAccessible::UpdateBullet(bool aHasBullet)
{
  if (aHasBullet == !!mBullet) {
    NS_NOTREACHED("Bullet and accessible are in sync already!");
    return;
  }

  nsDocAccessible* document = GetDocAccessible();
  if (aHasBullet) {
    mBullet = new nsHTMLListBulletAccessible(mContent, mWeakShell);
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
nsHTMLLIAccessible::CacheChildren()
{
  if (mBullet)
    AppendChild(mBullet);

  
  nsAccessibleWrap::CacheChildren();
}





nsHTMLListBulletAccessible::
  nsHTMLListBulletAccessible(nsIContent* aContent, nsIWeakReference* aShell) :
    nsLeafAccessible(aContent, aShell)
{
}




bool
nsHTMLListBulletAccessible::IsPrimaryForNode() const
{
  return false;
}




NS_IMETHODIMP
nsHTMLListBulletAccessible::GetName(nsAString &aName)
{
  aName.Truncate();

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  nsBlockFrame* blockFrame = do_QueryFrame(mContent->GetPrimaryFrame());
  NS_ASSERTION(blockFrame, "No frame for list item!");
  if (blockFrame) {
    blockFrame->GetBulletText(aName);

    
    aName.Append(' ');
  }

  return NS_OK;
}

PRUint32
nsHTMLListBulletAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_STATICTEXT;
}

PRUint64
nsHTMLListBulletAccessible::NativeState()
{
  PRUint64 state = nsLeafAccessible::NativeState();

  state &= ~states::FOCUSABLE;
  state |= states::READONLY;
  return state;
}

void
nsHTMLListBulletAccessible::AppendTextTo(nsAString& aText, PRUint32 aStartOffset,
                                         PRUint32 aLength)
{
  nsAutoString bulletText;
  nsBlockFrame* blockFrame = do_QueryFrame(mContent->GetPrimaryFrame());
  NS_ASSERTION(blockFrame, "No frame for list item!");
  if (blockFrame)
    blockFrame->GetBulletText(bulletText);

  aText.Append(Substring(bulletText, aStartOffset, aLength));
}





nsHTMLListAccessible::
  nsHTMLListAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsHyperTextAccessibleWrap(aContent, aShell)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLListAccessible, nsHyperTextAccessible)

PRUint32
nsHTMLListAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_LIST;
}

PRUint64
nsHTMLListAccessible::NativeState()
{
  return nsHyperTextAccessibleWrap::NativeState() | states::READONLY;
}


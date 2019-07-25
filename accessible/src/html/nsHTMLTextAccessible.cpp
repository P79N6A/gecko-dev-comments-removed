






































#include "nsHTMLTextAccessible.h"

#include "nsDocAccessible.h"
#include "nsTextEquivUtils.h"

#include "nsIFrame.h"
#include "nsPresContext.h"
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
  
  aName.Truncate();
  return AppendTextTo(aName, 0, PR_UINT32_MAX);
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

nsresult
nsHTMLTextAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsTextAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  nsDocAccessible *docAccessible = GetDocAccessible();
  if (docAccessible) {
     PRUint32 state, extState;
     docAccessible->GetState(&state, &extState);
     if (0 == (extState & nsIAccessibleStates::EXT_STATE_EDITABLE)) {
       *aState |= nsIAccessibleStates::STATE_READONLY; 
     }
  }

  return NS_OK;
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

nsresult
nsHTMLBRAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  *aState = 0;

  if (IsDefunct()) {
    if (aExtraState)
      *aExtraState = nsIAccessibleStates::EXT_STATE_DEFUNCT;

    return NS_OK_DEFUNCT_OBJECT;
  }

  *aState = nsIAccessibleStates::STATE_READONLY;
  if (aExtraState)
    *aExtraState = 0;

  return NS_OK;
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





nsHTMLLIAccessible::
  nsHTMLLIAccessible(nsIContent *aContent, nsIWeakReference *aShell,
                     const nsAString& aBulletText) :
  nsHyperTextAccessibleWrap(aContent, aShell)
{
  if (!aBulletText.IsEmpty()) {
    mBulletAccessible = new nsHTMLListBulletAccessible(mContent, mWeakShell, 
                                                       aBulletText);
    if (mBulletAccessible)
      mBulletAccessible->Init();
  }
}

NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLLIAccessible, nsHyperTextAccessible)

void
nsHTMLLIAccessible::Shutdown()
{
  if (mBulletAccessible) {
    
    mBulletAccessible->Shutdown();
  }

  nsHyperTextAccessibleWrap::Shutdown();
  mBulletAccessible = nsnull;
}

PRUint32
nsHTMLLIAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_LISTITEM;
}

nsresult
nsHTMLLIAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsHyperTextAccessibleWrap::GetStateInternal(aState,
                                                            aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  *aState |= nsIAccessibleStates::STATE_READONLY;
  return NS_OK;
}

NS_IMETHODIMP nsHTMLLIAccessible::GetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height)
{
  nsresult rv = nsAccessibleWrap::GetBounds(x, y, width, height);
  if (NS_FAILED(rv) || !mBulletAccessible) {
    return rv;
  }

  PRInt32 bulletX, bulletY, bulletWidth, bulletHeight;
  rv = mBulletAccessible->GetBounds(&bulletX, &bulletY, &bulletWidth, &bulletHeight);
  NS_ENSURE_SUCCESS(rv, rv);

  *x = bulletX; 
  *width += bulletWidth;
  return NS_OK;
}




void
nsHTMLLIAccessible::CacheChildren()
{
  if (mBulletAccessible)
    AppendChild(mBulletAccessible);

  
  nsAccessibleWrap::CacheChildren();
}





nsHTMLListBulletAccessible::
  nsHTMLListBulletAccessible(nsIContent *aContent, nsIWeakReference *aShell,
                             const nsAString& aBulletText) :
    nsLeafAccessible(aContent, aShell), mBulletText(aBulletText)
{
  mBulletText += ' '; 
}

NS_IMETHODIMP
nsHTMLListBulletAccessible::GetUniqueID(void **aUniqueID)
{
  
  
  *aUniqueID = static_cast<void*>(this);
  return NS_OK;
}

void
nsHTMLListBulletAccessible::Shutdown()
{
  mBulletText.Truncate();
  nsLeafAccessible::Shutdown();
}

NS_IMETHODIMP
nsHTMLListBulletAccessible::GetName(nsAString &aName)
{
  
  aName = mBulletText;
  return NS_OK;
}

PRUint32
nsHTMLListBulletAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_STATICTEXT;
}

nsresult
nsHTMLListBulletAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsLeafAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  *aState &= ~nsIAccessibleStates::STATE_FOCUSABLE;
  *aState |= nsIAccessibleStates::STATE_READONLY;
  return NS_OK;
}

nsresult
nsHTMLListBulletAccessible::AppendTextTo(nsAString& aText, PRUint32 aStartOffset,
                                         PRUint32 aLength)
{
  PRUint32 maxLength = mBulletText.Length() - aStartOffset;
  if (aLength > maxLength) {
    aLength = maxLength;
  }
  aText += Substring(mBulletText, aStartOffset, aLength);
  return NS_OK;
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

nsresult
nsHTMLListAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsHyperTextAccessibleWrap::GetStateInternal(aState,
                                                            aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  *aState |= nsIAccessibleStates::STATE_READONLY;
  return NS_OK;
}


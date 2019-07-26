





#include "xpcAccessibleTextRange.h"

#include "HyperTextAccessible.h"
#include "TextRange.h"

using namespace mozilla;
using namespace mozilla::a11y;


NS_IMPL_ISUPPORTS1(xpcAccessibleTextRange, nsIAccessibleTextRange)



NS_IMETHODIMP
xpcAccessibleTextRange::GetStartContainer(nsIAccessible** aAnchor)
{
  NS_ENSURE_ARG_POINTER(aAnchor);
  *aAnchor = static_cast<nsIAccessible*>(mRange.StartContainer());
  return NS_OK;
}

NS_IMETHODIMP
xpcAccessibleTextRange::GetStartOffset(int32_t* aOffset)
{
  NS_ENSURE_ARG_POINTER(aOffset);
  *aOffset = mRange.StartOffset();
  return NS_OK;
}

NS_IMETHODIMP
xpcAccessibleTextRange::GetEndContainer(nsIAccessible** aAnchor)
{
  NS_ENSURE_ARG_POINTER(aAnchor);
  *aAnchor = static_cast<nsIAccessible*>(mRange.EndContainer());
  return NS_OK;
}

NS_IMETHODIMP
xpcAccessibleTextRange::GetEndOffset(int32_t* aOffset)
{
  NS_ENSURE_ARG_POINTER(aOffset);
  *aOffset = mRange.EndOffset();
  return NS_OK;
}

NS_IMETHODIMP
xpcAccessibleTextRange::GetText(nsAString& aText)
{
  nsAutoString text;
  mRange.Text(text);
  aText.Assign(text);

  return NS_OK;
}

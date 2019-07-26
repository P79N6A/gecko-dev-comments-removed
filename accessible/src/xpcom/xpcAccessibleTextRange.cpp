





#include "xpcAccessibleTextRange.h"

#include "HyperTextAccessible.h"
#include "TextRange.h"

using namespace mozilla;
using namespace mozilla::a11y;



NS_IMPL_CYCLE_COLLECTION(xpcAccessibleTextRange,
                         mRange.mRoot,
                         mRange.mStartContainer,
                         mRange.mEndContainer)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(xpcAccessibleTextRange)
  NS_INTERFACE_MAP_ENTRY(nsIAccessibleTextRange)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIAccessibleTextRange)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(xpcAccessibleTextRange)
NS_IMPL_CYCLE_COLLECTING_RELEASE(xpcAccessibleTextRange)



NS_IMETHODIMP
xpcAccessibleTextRange::GetStartContainer(nsIAccessible** aAnchor)
{
  NS_ENSURE_ARG_POINTER(aAnchor);
  NS_IF_ADDREF(*aAnchor = static_cast<nsIAccessible*>(mRange.StartContainer()));
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
  NS_IF_ADDREF(*aAnchor = static_cast<nsIAccessible*>(mRange.EndContainer()));
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

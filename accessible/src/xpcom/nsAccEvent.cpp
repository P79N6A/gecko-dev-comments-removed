





































#include "nsAccEvent.h"
#include "nsAccUtils.h"
#include "nsDocAccessible.h"





NS_IMPL_ISUPPORTS1(nsAccEvent, nsIAccessibleEvent)

NS_IMETHODIMP
nsAccEvent::GetIsFromUserInput(PRBool* aIsFromUserInput)
{
  *aIsFromUserInput = mEvent->IsFromUserInput();
  return NS_OK;
}

NS_IMETHODIMP
nsAccEvent::GetEventType(PRUint32* aEventType)
{
  *aEventType = mEvent->GetEventType();
  return NS_OK;
}

NS_IMETHODIMP
nsAccEvent::GetAccessible(nsIAccessible** aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);
  *aAccessible = nsnull;

  NS_IF_ADDREF(*aAccessible = mEvent->GetAccessible());
  return NS_OK;
}

NS_IMETHODIMP
nsAccEvent::GetDOMNode(nsIDOMNode** aDOMNode)
{
  NS_ENSURE_ARG_POINTER(aDOMNode);
  *aDOMNode = nsnull;

  nsINode* node = mEvent->GetNode();
  if (node)
    CallQueryInterface(node, aDOMNode);

  return NS_OK;
}

NS_IMETHODIMP
nsAccEvent::GetAccessibleDocument(nsIAccessibleDocument** aDocAccessible)
{
  NS_ENSURE_ARG_POINTER(aDocAccessible);

  NS_IF_ADDREF(*aDocAccessible = mEvent->GetDocAccessible());
  return NS_OK;
}






NS_IMPL_ISUPPORTS_INHERITED1(nsAccStateChangeEvent, nsAccEvent,
                             nsIAccessibleStateChangeEvent)

NS_IMETHODIMP
nsAccStateChangeEvent::GetState(PRUint32* aState)
{
  NS_ENSURE_ARG_POINTER(aState);

  PRUint32 state1 = 0, state2 = 0;
  PRUint64 state = static_cast<AccStateChangeEvent*>(mEvent.get())->GetState();
  nsAccUtils::To32States(state, &state1, &state2);

  *aState = state1 | state2; 
  return NS_OK;
}

NS_IMETHODIMP
nsAccStateChangeEvent::IsExtraState(PRBool* aIsExtraState)
{
  NS_ENSURE_ARG_POINTER(aIsExtraState);

  PRUint32 state1 = 0, state2 = 0;
  PRUint64 state = static_cast<AccStateChangeEvent*>(mEvent.get())->GetState();
  nsAccUtils::To32States(state, &state1, &state2);

  *aIsExtraState = (state2 != 0);
  return NS_OK;
}

NS_IMETHODIMP
nsAccStateChangeEvent::IsEnabled(PRBool* aIsEnabled)
{
  NS_ENSURE_ARG_POINTER(aIsEnabled);
  *aIsEnabled = static_cast<AccStateChangeEvent*>(mEvent.get())->IsStateEnabled();
  return NS_OK;
}





NS_IMPL_ISUPPORTS_INHERITED1(nsAccTextChangeEvent, nsAccEvent,
                             nsIAccessibleTextChangeEvent)

NS_IMETHODIMP
nsAccTextChangeEvent::GetStart(PRInt32* aStart)
{
  NS_ENSURE_ARG_POINTER(aStart);
  *aStart = static_cast<AccTextChangeEvent*>(mEvent.get())->GetStartOffset();
  return NS_OK;
}

NS_IMETHODIMP
nsAccTextChangeEvent::GetLength(PRUint32* aLength)
{
  NS_ENSURE_ARG_POINTER(aLength);
  *aLength = static_cast<AccTextChangeEvent*>(mEvent.get())->GetLength();
  return NS_OK;
}

NS_IMETHODIMP
nsAccTextChangeEvent::IsInserted(PRBool* aIsInserted)
{
  NS_ENSURE_ARG_POINTER(aIsInserted);
  *aIsInserted = static_cast<AccTextChangeEvent*>(mEvent.get())->IsTextInserted();
  return NS_OK;
}

NS_IMETHODIMP
nsAccTextChangeEvent::GetModifiedText(nsAString& aModifiedText)
{
  static_cast<AccTextChangeEvent*>(mEvent.get())->GetModifiedText(aModifiedText);
  return NS_OK;
}






NS_IMPL_ISUPPORTS_INHERITED1(nsAccCaretMoveEvent, nsAccEvent,
                             nsIAccessibleCaretMoveEvent)

NS_IMETHODIMP
nsAccCaretMoveEvent::GetCaretOffset(PRInt32 *aCaretOffset)
{
  NS_ENSURE_ARG_POINTER(aCaretOffset);

  *aCaretOffset = static_cast<AccCaretMoveEvent*>(mEvent.get())->GetCaretOffset();
  return NS_OK;
}





NS_IMPL_ISUPPORTS_INHERITED1(nsAccTableChangeEvent, nsAccEvent,
                             nsIAccessibleTableChangeEvent)

NS_IMETHODIMP
nsAccTableChangeEvent::GetRowOrColIndex(PRInt32 *aRowOrColIndex)
{
  NS_ENSURE_ARG_POINTER(aRowOrColIndex);

  *aRowOrColIndex =
    static_cast<AccTableChangeEvent*>(mEvent.get())->GetIndex();
  return NS_OK;
}

NS_IMETHODIMP
nsAccTableChangeEvent::GetNumRowsOrCols(PRInt32* aNumRowsOrCols)
{
  NS_ENSURE_ARG_POINTER(aNumRowsOrCols);

  *aNumRowsOrCols = static_cast<AccTableChangeEvent*>(mEvent.get())->GetCount();
  return NS_OK;
}


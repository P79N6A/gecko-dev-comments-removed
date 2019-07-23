







































#include "nsHyperTextAccessibleWrap.h"

nsString nsHyperTextAccessibleWrap::sText;
PRInt32 nsHyperTextAccessibleWrap::sOffset = 0;
PRUint32 nsHyperTextAccessibleWrap::sLength = 0;
PRBool nsHyperTextAccessibleWrap::sIsInserted = PR_FALSE;

NS_IMPL_ISUPPORTS_INHERITED0(nsHyperTextAccessibleWrap,
                             nsHyperTextAccessible)

IMPL_IUNKNOWN_INHERITED2(nsHyperTextAccessibleWrap, nsAccessibleWrap,
                         CAccessibleText, CAccessibleEditableText);

NS_IMETHODIMP
nsHyperTextAccessibleWrap::FireAccessibleEvent(nsIAccessibleEvent *aEvent)
{
  PRUint32 eventType;
  aEvent->GetEventType(&eventType);

  if (eventType == nsIAccessibleEvent::EVENT_TEXT_CHANGED) {
    nsCOMPtr<nsIAccessibleTextChangeEvent> textEvent(do_QueryInterface(aEvent));
    NS_ENSURE_STATE(textEvent);

    textEvent->GetStart(&sOffset);
    textEvent->GetLength(&sLength);
    textEvent->IsInserted(&sIsInserted);

    GetText(sOffset, sOffset + sLength, sText);
  }

  return nsHyperTextAccessible::FireAccessibleEvent(aEvent);
}

nsresult
nsHyperTextAccessibleWrap::GetModifiedText(PRBool aGetInsertedText,
                                           nsAString& aText,
                                           PRUint32 *aStartOffset,
                                           PRUint32 *aEndOffset)
{
  if ((aGetInsertedText && sIsInserted) || (!aGetInsertedText && !sIsInserted)) {
    aText.Assign(sText);
    *aStartOffset = sOffset;
    *aEndOffset = sOffset + sLength;
  } else {
    aText.Truncate();
    *aStartOffset = 0;
    *aEndOffset = 0;
  }

  return NS_OK;
}


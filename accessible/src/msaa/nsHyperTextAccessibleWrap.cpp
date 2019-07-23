







































#include "nsHyperTextAccessibleWrap.h"

NS_IMPL_ISUPPORTS_INHERITED0(nsHyperTextAccessibleWrap,
                             nsHyperTextAccessible)

IMPL_IUNKNOWN_INHERITED2(nsHyperTextAccessibleWrap,
                         nsAccessibleWrap,
                         CAccessibleHypertext,
                         CAccessibleEditableText);

nsresult
nsHyperTextAccessibleWrap::FireAccessibleEvent(nsIAccessibleEvent *aEvent)
{
  PRUint32 eventType;
  aEvent->GetEventType(&eventType);

  if (eventType == nsIAccessibleEvent::EVENT_TEXT_REMOVED ||
      eventType == nsIAccessibleEvent::EVENT_TEXT_INSERTED) {
    nsCOMPtr<nsIAccessible> accessible;
    aEvent->GetAccessible(getter_AddRefs(accessible));
    if (accessible) {
      nsCOMPtr<nsIWinAccessNode> winAccessNode(do_QueryInterface(accessible));
      if (winAccessNode) {
        void *instancePtr = NULL;
        nsresult rv = winAccessNode->QueryNativeInterface(IID_IAccessibleText,
                                                          &instancePtr);
        if (NS_SUCCEEDED(rv)) {
          NS_IF_RELEASE(gTextEvent);

          CallQueryInterface(aEvent, &gTextEvent);
          (static_cast<IUnknown*>(instancePtr))->Release();
        }
      }
    }
  }

  return nsHyperTextAccessible::FireAccessibleEvent(aEvent);
}

nsresult
nsHyperTextAccessibleWrap::GetModifiedText(PRBool aGetInsertedText,
                                           nsAString& aText,
                                           PRUint32 *aStartOffset,
                                           PRUint32 *aEndOffset)
{
  aText.Truncate();
  *aStartOffset = 0;
  *aEndOffset = 0;

  if (!gTextEvent)
    return NS_OK;
    
  PRBool isInserted;
  gTextEvent->IsInserted(&isInserted);
  if (aGetInsertedText != isInserted)
    return NS_OK;

  nsCOMPtr<nsIAccessibleEvent> event(do_QueryInterface(gTextEvent));

  nsCOMPtr<nsIAccessible> targetAcc;
  event->GetAccessible(getter_AddRefs(targetAcc));
  if (targetAcc != this)
    return NS_OK;

  PRInt32 offset;
  PRUint32 length;

  gTextEvent->GetStart(&offset);
  gTextEvent->GetLength(&length);
  *aStartOffset = offset;
  *aEndOffset = offset + length;
  return gTextEvent->GetModifiedText(aText);
}


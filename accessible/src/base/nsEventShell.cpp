





































#include "nsEventShell.h"

#include "nsAccessible.h"

void
nsEventShell::FireEvent(nsAccEvent *aEvent)
{
  if (!aEvent)
    return;

  nsRefPtr<nsAccessible> acc =
    nsAccUtils::QueryObject<nsAccessible>(aEvent->GetAccessible());
  NS_ENSURE_TRUE(acc,);

  acc->HandleAccEvent(aEvent);
}

void
nsEventShell::FireEvent(PRUint32 aEventType, nsIAccessible *aAccessible,
                        PRBool aIsAsynch)
{
  NS_ENSURE_TRUE(aAccessible,);

  nsRefPtr<nsAccEvent> event = new nsAccEvent(aEventType, aAccessible,
                                              aIsAsynch);

  FireEvent(event);
}

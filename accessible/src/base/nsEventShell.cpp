





































#include "nsEventShell.h"

#include "nsAccessible.h"
#include "nsAccEvent.h"

void
nsEventShell::FireEvent(PRUint32 aEventType, nsIAccessible *aAccessible,
                        PRBool aIsAsynch)
{
  NS_ENSURE_TRUE(aAccessible,);

  nsRefPtr<nsAccessible> acc =
    nsAccUtils::QueryObject<nsAccessible>(aAccessible);

  nsCOMPtr<nsIAccessibleEvent> event =
    new nsAccEvent(aEventType, aAccessible, aIsAsynch);

  if (event)  
    acc->FireAccessibleEvent(event);
}

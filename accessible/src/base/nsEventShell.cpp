





































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

  nsCOMPtr<nsIDOMNode> node;
  aEvent->GetDOMNode(getter_AddRefs(node));
  if (node) {
    sEventTargetNode = node;
    sEventFromUserInput = aEvent->IsFromUserInput();
  }

  acc->HandleAccEvent(aEvent);

  sEventTargetNode = nsnull;
}

void
nsEventShell::FireEvent(PRUint32 aEventType, nsIAccessible *aAccessible,
                        PRBool aIsAsynch, EIsFromUserInput aIsFromUserInput)
{
  NS_ENSURE_TRUE(aAccessible,);

  nsRefPtr<nsAccEvent> event = new nsAccEvent(aEventType, aAccessible,
                                              aIsAsynch, aIsFromUserInput);

  FireEvent(event);
}

void 
nsEventShell::GetEventAttributes(nsIDOMNode *aNode,
                                 nsIPersistentProperties *aAttributes)
{
  if (aNode != sEventTargetNode)
    return;

  nsAccUtils::SetAccAttr(aAttributes, nsAccessibilityAtoms::eventFromInput,
                         sEventFromUserInput ? NS_LITERAL_STRING("true") :
                                               NS_LITERAL_STRING("false"));
}




PRBool nsEventShell::sEventFromUserInput = PR_FALSE;
nsCOMPtr<nsIDOMNode> nsEventShell::sEventTargetNode;

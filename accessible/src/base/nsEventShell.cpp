




#include "nsEventShell.h"

#include "nsAccUtils.h"





void
nsEventShell::FireEvent(AccEvent* aEvent)
{
  if (!aEvent)
    return;

  Accessible* accessible = aEvent->GetAccessible();
  NS_ENSURE_TRUE_VOID(accessible);

  nsINode* node = aEvent->GetNode();
  if (node) {
    sEventTargetNode = node;
    sEventFromUserInput = aEvent->IsFromUserInput();
  }

  accessible->HandleAccEvent(aEvent);

  sEventTargetNode = nullptr;
}

void
nsEventShell::FireEvent(uint32_t aEventType, Accessible* aAccessible,
                        EIsFromUserInput aIsFromUserInput)
{
  NS_ENSURE_TRUE_VOID(aAccessible);

  nsRefPtr<AccEvent> event = new AccEvent(aEventType, aAccessible,
                                          aIsFromUserInput);

  FireEvent(event);
}

void 
nsEventShell::GetEventAttributes(nsINode *aNode,
                                 nsIPersistentProperties *aAttributes)
{
  if (aNode != sEventTargetNode)
    return;

  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::eventFromInput,
                         sEventFromUserInput ? NS_LITERAL_STRING("true") :
                                               NS_LITERAL_STRING("false"));
}




bool nsEventShell::sEventFromUserInput = false;
nsCOMPtr<nsINode> nsEventShell::sEventTargetNode;

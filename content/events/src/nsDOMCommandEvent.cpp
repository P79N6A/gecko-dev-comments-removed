




































#include "nsDOMCommandEvent.h"
#include "nsContentUtils.h"

nsDOMCommandEvent::nsDOMCommandEvent(nsPresContext* aPresContext,
                                     nsCommandEvent* aEvent)
  : nsDOMEvent(aPresContext, aEvent ? aEvent :
               new nsCommandEvent(PR_FALSE, nsnull, nsnull, nsnull))
{
  mEvent->time = PR_Now();
  if (aEvent) {
    mEventIsInternal = PR_FALSE;
  } else {
    mEventIsInternal = PR_TRUE;
  }
}

nsDOMCommandEvent::~nsDOMCommandEvent()
{
  if (mEventIsInternal && mEvent->eventStructType == NS_COMMAND_EVENT) {
    delete static_cast<nsCommandEvent*>(mEvent);
    mEvent = nsnull;
  }
}

DOMCI_DATA(CommandEvent, nsDOMCommandEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMCommandEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCommandEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CommandEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(nsDOMCommandEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMCommandEvent, nsDOMEvent)

NS_IMETHODIMP
nsDOMCommandEvent::GetCommand(nsAString& aCommand)
{
  nsIAtom* command = static_cast<nsCommandEvent*>(mEvent)->command;
  if (command) {
    command->ToString(aCommand);
  } else {
    aCommand.Truncate();
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDOMCommandEvent::InitCommandEvent(const nsAString& aTypeArg,
                                    bool aCanBubbleArg,
                                    bool aCancelableArg,
                                    const nsAString& aCommand)
{
  nsresult rv = nsDOMEvent::InitEvent(aTypeArg, aCanBubbleArg, aCancelableArg);
  NS_ENSURE_SUCCESS(rv, rv);

  static_cast<nsCommandEvent*>(mEvent)->command = do_GetAtom(aCommand);
  return NS_OK;
}

nsresult NS_NewDOMCommandEvent(nsIDOMEvent** aInstancePtrResult,
                               nsPresContext* aPresContext,
                               nsCommandEvent* aEvent)
{
  nsDOMCommandEvent* it = new nsDOMCommandEvent(aPresContext, aEvent);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aInstancePtrResult);
}







































#include "nsHtml5SVGLoadDispatcher.h"
#include "nsPresContext.h"
#include "nsEventDispatcher.h"
#include "nsIPresShell.h"

nsHtml5SVGLoadDispatcher::nsHtml5SVGLoadDispatcher(nsIContent* aElement)
  : mElement(aElement)
{
}

NS_IMETHODIMP
nsHtml5SVGLoadDispatcher::Run()
{
  nsEvent event(PR_TRUE, NS_SVG_LOAD);
  event.eventStructType = NS_SVG_EVENT;
  event.flags |= NS_EVENT_FLAG_CANT_BUBBLE;
  
  
  
  
  nsRefPtr<nsPresContext> ctx;
  nsCOMPtr<nsIPresShell> shell = mElement->GetOwnerDoc()->GetPrimaryShell();
  if (shell) {
    ctx = shell->GetPresContext();
  }
  nsEventDispatcher::Dispatch(mElement, ctx, &event);
  return NS_OK;
}

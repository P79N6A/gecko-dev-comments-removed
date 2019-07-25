





































#include "nsHtml5SVGLoadDispatcher.h"
#include "nsPresContext.h"
#include "nsEventDispatcher.h"
#include "nsIPresShell.h"
#include "nsGUIEvent.h"

nsHtml5SVGLoadDispatcher::nsHtml5SVGLoadDispatcher(nsIContent* aElement)
  : mElement(aElement)
  , mDocument(mElement->GetOwnerDoc())
{
  mDocument->BlockOnload();
}

NS_IMETHODIMP
nsHtml5SVGLoadDispatcher::Run()
{
  nsEvent event(PR_TRUE, NS_SVG_LOAD);
  event.eventStructType = NS_SVG_EVENT;
  event.flags |= NS_EVENT_FLAG_CANT_BUBBLE;
  
  
  
  
  nsRefPtr<nsPresContext> ctx;
  nsCOMPtr<nsIPresShell> shell = mElement->GetOwnerDoc()->GetShell();
  if (shell) {
    ctx = shell->GetPresContext();
  }
  nsEventDispatcher::Dispatch(mElement, ctx, &event);
  
  
  mDocument->UnblockOnload(PR_FALSE);
  return NS_OK;
}

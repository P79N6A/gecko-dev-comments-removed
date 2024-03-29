



#include "nsHtml5SVGLoadDispatcher.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "mozilla/BasicEvents.h"
#include "mozilla/EventDispatcher.h"
#include "nsIDocument.h"

using namespace mozilla;

nsHtml5SVGLoadDispatcher::nsHtml5SVGLoadDispatcher(nsIContent* aElement)
  : mElement(aElement)
  , mDocument(mElement->OwnerDoc())
{
  mDocument->BlockOnload();
}

NS_IMETHODIMP
nsHtml5SVGLoadDispatcher::Run()
{
  WidgetEvent event(true, NS_SVG_LOAD);
  event.mFlags.mBubbles = false;
  
  
  
  
  nsRefPtr<nsPresContext> ctx;
  nsCOMPtr<nsIPresShell> shell = mElement->OwnerDoc()->GetShell();
  if (shell) {
    ctx = shell->GetPresContext();
  }
  EventDispatcher::Dispatch(mElement, ctx, &event);
  
  
  mDocument->UnblockOnload(false);
  return NS_OK;
}

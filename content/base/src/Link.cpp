






































#include "Link.h"

namespace mozilla {
namespace dom {

Link::Link()
  : mLinkState(defaultState)
{
}

nsLinkState
Link::GetLinkState() const
{
  return mLinkState;
}

void
Link::SetLinkState(nsLinkState aState)
{
  mLinkState = aState;
}

void
Link::ResetLinkState()
{
  nsCOMPtr<nsIContent> content(do_QueryInterface(this));
  NS_ASSERTION(content, "Why isn't this an nsIContent node?!");

  nsIDocument *doc = content->GetCurrentDoc();
  if (doc) {
    doc->ForgetLink(content);
  }
  mLinkState = defaultState;
}

} 
} 








































#include "Link.h"

#include "nsIEventStateManager.h"

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

PRInt32
Link::LinkState() const
{
  if (mLinkState == eLinkState_Visited) {
    return NS_EVENT_STATE_VISITED;
  }

  if (mLinkState == eLinkState_Unvisited) {
    return NS_EVENT_STATE_UNVISITED;
  }

  return 0;
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

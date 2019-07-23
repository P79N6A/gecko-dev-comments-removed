




































#include "nsHtml5TreeBuilder.h"
#include "nsHtml5PendingNotification.h"

void
nsHtml5PendingNotification::Fire(nsHtml5TreeBuilder* aBuilder)
{
  aBuilder->NotifyAppend(mParent, mChildCount);
}

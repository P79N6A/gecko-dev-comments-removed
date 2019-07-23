




































#include "nsHtml5ReleasableAttributeName.h"

nsHtml5ReleasableAttributeName::nsHtml5ReleasableAttributeName(PRInt32* uri, nsIAtom** local, nsIAtom** prefix)
  : nsHtml5AttributeName(uri, local, prefix)
{
}

void
nsHtml5ReleasableAttributeName::release()
{
  delete this;
}

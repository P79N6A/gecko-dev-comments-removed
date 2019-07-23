




































#include "nsHtml5ReleasableAttributeName.h"
#include "nsHtml5Portability.h"
#include "nsHtml5AtomTable.h"

nsHtml5ReleasableAttributeName::nsHtml5ReleasableAttributeName(PRInt32* uri, nsIAtom** local, nsIAtom** prefix)
  : nsHtml5AttributeName(uri, local, prefix)
{
}

nsHtml5AttributeName*
nsHtml5ReleasableAttributeName::cloneAttributeName(nsHtml5AtomTable* aInterner)
{
  nsIAtom* l = getLocal(0);
  if (aInterner) {
    if (l->IsStaticAtom()) {
      nsAutoString str;
      l->ToString(str);
      l = aInterner->GetAtom(str);
    }
  }
  return new nsHtml5ReleasableAttributeName(nsHtml5AttributeName::ALL_NO_NS, 
                                            nsHtml5AttributeName::SAME_LOCAL(l), 
                                            nsHtml5AttributeName::ALL_NO_PREFIX);
}

void
nsHtml5ReleasableAttributeName::release()
{
  delete this;
}

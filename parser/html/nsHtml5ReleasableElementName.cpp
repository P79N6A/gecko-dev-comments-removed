




































#include "nsHtml5ReleasableElementName.h"

nsHtml5ReleasableElementName::nsHtml5ReleasableElementName(nsIAtom* name)
  : nsHtml5ElementName(name)
{
}

void
nsHtml5ReleasableElementName::release()
{
  delete this;
}

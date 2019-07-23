




































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

nsHtml5ElementName* 
nsHtml5ReleasableElementName::cloneElementName(nsHtml5AtomTable* aInterner)
{
  nsIAtom* l = name;
  if (aInterner) {
    if (l->IsStaticAtom()) {
      nsAutoString str;
      l->ToString(str);
      l = aInterner->GetAtom(str);
    }
  }
  return new nsHtml5ReleasableElementName(l);
}

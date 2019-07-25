



#ifndef nsHtml5ReleasableElementName_h__
#define nsHtml5ReleasableElementName_h__

#include "nsHtml5ElementName.h"
#include "mozilla/Attributes.h"

class nsHtml5ReleasableElementName MOZ_FINAL : public nsHtml5ElementName
{
  public:
    nsHtml5ReleasableElementName(nsIAtom* name);
    virtual void release();
    virtual nsHtml5ElementName* cloneElementName(nsHtml5AtomTable* interner);
};

#endif 

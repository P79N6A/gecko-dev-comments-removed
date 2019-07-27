



#ifndef nsHtml5ReleasableElementName_h
#define nsHtml5ReleasableElementName_h

#include "nsHtml5ElementName.h"
#include "mozilla/Attributes.h"

class nsHtml5ReleasableElementName final : public nsHtml5ElementName
{
  public:
    explicit nsHtml5ReleasableElementName(nsIAtom* name);
    virtual void release();
    virtual nsHtml5ElementName* cloneElementName(nsHtml5AtomTable* interner);
};

#endif 

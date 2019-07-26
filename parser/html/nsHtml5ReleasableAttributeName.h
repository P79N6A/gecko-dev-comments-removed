



#ifndef nsHtml5ReleasableAttributeName_h
#define nsHtml5ReleasableAttributeName_h

#include "nsHtml5AttributeName.h"
#include "mozilla/Attributes.h"

class nsHtml5AtomTable;

class nsHtml5ReleasableAttributeName MOZ_FINAL : public nsHtml5AttributeName
{
  public:
    nsHtml5ReleasableAttributeName(int32_t* uri, nsIAtom** local, nsIAtom** prefix);
    virtual nsHtml5AttributeName* cloneAttributeName(nsHtml5AtomTable* aInterner);
    virtual void release();
};

#endif 






































#ifndef nsHtml5ReleasableAttributeName_h__
#define nsHtml5ReleasableAttributeName_h__

#include "nsHtml5AttributeName.h"

class nsHtml5AtomTable;

class nsHtml5ReleasableAttributeName : public nsHtml5AttributeName
{
  public:
    nsHtml5ReleasableAttributeName(PRInt32* uri, nsIAtom** local, nsIAtom** prefix);
    virtual nsHtml5AttributeName* cloneAttributeName(nsHtml5AtomTable* aInterner);
    virtual void release();
};

#endif 






































#ifndef nsHtml5ReleasableAttributeName_h__
#define nsHtml5ReleasableAttributeName_h__

#include "nsHtml5AttributeName.h"

class nsHtml5ReleasableAttributeName : public nsHtml5AttributeName
{
  public:
    nsHtml5ReleasableAttributeName(PRInt32* uri, nsIAtom** local, nsIAtom** prefix);
    virtual nsHtml5AttributeName* cloneAttributeName();
    virtual void release();
};

#endif 

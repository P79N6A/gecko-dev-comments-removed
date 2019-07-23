






































#ifndef _NS_LEGACYCHECK_H_
#define _NS_LEGACYCHECK_H_

#include "XIDefines.h"
#include "XIErrors.h"

class nsLegacyCheck
{
public:
    nsLegacyCheck(char *aFilename, char *aMessage);
    ~nsLegacyCheck();

    char            *GetFilename();
    char            *GetMessage();
    int             SetNext(nsLegacyCheck *aNext);
    nsLegacyCheck   *GetNext();

private:
    char            *mFilename;
    char            *mMessage;
    nsLegacyCheck   *mNext;
};

#endif 

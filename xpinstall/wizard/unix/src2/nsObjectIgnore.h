






































#ifndef _NS_OBJECTIGNORE_H_
#define _NS_OBJECTIGNORE_H_

#include "XIDefines.h"
#include "XIErrors.h"

class nsObjectIgnore
{
public:
    nsObjectIgnore(char *aFilename);
    ~nsObjectIgnore();

    char            *GetFilename();
    int             SetNext(nsObjectIgnore *aNext);
    nsObjectIgnore  *GetNext();

private:
    char            *mFilename;
    nsObjectIgnore  *mNext;
};

#endif 

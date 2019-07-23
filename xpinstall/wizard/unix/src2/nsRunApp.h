






































#ifndef _NS_RUNAPP_H_
#define _NS_RUNAPP_H_

#include "XIDefines.h"

class nsRunApp
{
public:
    nsRunApp(char *aApp, char *aArgs);
    ~nsRunApp();

    char        *GetApp();
    char        *GetArgs();
    void        SetNext(nsRunApp *aNext);
    nsRunApp    *GetNext();

private:
    char        *mApp;
    char        *mArgs;
    nsRunApp    *mNext;
};

#endif 

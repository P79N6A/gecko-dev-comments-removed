






































#ifndef _NS_XIOPTIONS_H_
#define _NS_XIOPTIONS_H_

#include "XIDefines.h"

class nsXIOptions
{
public:
    nsXIOptions();
    ~nsXIOptions();
    
    char    *mTitle;
    char    *mDestination;
    int     mSetupType;
    int     mMode;
    int     mShouldRunApps;

    enum
    {
        MODE_DEFAULT = 0,
        MODE_AUTO,
        MODE_SILENT
    };

    char    *mProxyHost;
    char    *mProxyPort;
    char    *mProxyUser;
    char    *mProxyPswd;

    int     mSaveModules;
};
    
#endif 

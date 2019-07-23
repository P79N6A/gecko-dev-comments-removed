






































#include "nsXIOptions.h"

nsXIOptions::nsXIOptions() :
    mTitle(NULL),
    mDestination(NULL),
    mSetupType(0),
    mMode(MODE_DEFAULT),
    mShouldRunApps(TRUE),
    mProxyHost(NULL),
    mProxyPort(NULL),
    mProxyUser(NULL),
    mProxyPswd(NULL),
    mSaveModules(FALSE)
{
}

nsXIOptions::~nsXIOptions()
{
     XI_IF_FREE(mTitle);
     XI_IF_FREE(mDestination);
     XI_IF_FREE(mProxyHost);
     XI_IF_FREE(mProxyPort);
     XI_IF_FREE(mProxyUser);
     XI_IF_FREE(mProxyPswd);
}

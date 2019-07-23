






































#include "nsRunApp.h"

nsRunApp::nsRunApp(char *aApp, char *aArgs) :
    mApp(aApp),
    mArgs(aArgs),
    mNext(NULL)
{
}

nsRunApp::~nsRunApp()
{
    XI_IF_FREE(mApp);
    XI_IF_FREE(mArgs);
}

char *
nsRunApp::GetApp()
{
    return mApp;
}

char *
nsRunApp::GetArgs()
{
    return mArgs;
}

void
nsRunApp::SetNext(nsRunApp *aNext)
{
    mNext = aNext;
}

nsRunApp *
nsRunApp::GetNext()
{
    return mNext;
}









































#include "nsLegacyCheck.h"
#include "assert.h"

nsLegacyCheck::nsLegacyCheck(char *aFilename, char *aMessage) :
    mFilename(aFilename),
    mMessage(aMessage),
    mNext(NULL)
{
}

nsLegacyCheck::~nsLegacyCheck()
{
#ifdef DEBUG
    if (mFilename)
        printf("%s %d: Freeing %s\n", __FILE__, __LINE__, mFilename);
#endif
    XI_IF_FREE(mFilename);
    XI_IF_FREE(mMessage);
}

char *
nsLegacyCheck::GetFilename()
{
    return mFilename;
}

char *
nsLegacyCheck::GetMessage()
{
    return mMessage;
}

int
nsLegacyCheck::SetNext(nsLegacyCheck *aNext)
{
    if (!aNext)
        return E_PARAM;

    mNext = aNext;

    return OK;
}

nsLegacyCheck *
nsLegacyCheck::GetNext()
{
    return mNext;
}

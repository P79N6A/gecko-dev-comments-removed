






































#include "nsObjectIgnore.h"
#include "assert.h"

nsObjectIgnore::nsObjectIgnore(char *aFilename) :
    mFilename(aFilename),
    mNext(NULL)
{
}

nsObjectIgnore::~nsObjectIgnore()
{
    XI_IF_FREE(mFilename);
}

char *
nsObjectIgnore::GetFilename()
{
    return mFilename;
}

int
nsObjectIgnore::SetNext(nsObjectIgnore *aNext)
{
    if (!aNext)
        return E_PARAM;

    mNext = aNext;

    return OK;
}

nsObjectIgnore *
nsObjectIgnore::GetNext()
{
    return mNext;
}










































#include "xptiprivate.h"

xptiFile::xptiFile()
    :   
#ifdef DEBUG
        mDEBUG_WorkingSet(nsnull),
#endif
        mSize(),
        mDate(),
        mName(nsnull),
        mGuts(nsnull),
        mDirectory(0)
{
    
    MOZ_COUNT_CTOR(xptiFile);
}

xptiFile::xptiFile(const nsInt64&  aSize,
         const nsInt64&  aDate,
         PRUint32        aDirectory,
         const char*     aName,
         xptiWorkingSet* aWorkingSet)
    :   
#ifdef DEBUG
        mDEBUG_WorkingSet(aWorkingSet),
#endif
        mSize(aSize),
        mDate(aDate),
        mName(aName),
        mGuts(nsnull),
        mDirectory(aDirectory)
{
    NS_ASSERTION(aWorkingSet,"bad param");
    mName = XPT_STRDUP(aWorkingSet->GetStringArena(), aName);

    MOZ_COUNT_CTOR(xptiFile);
}

xptiFile::xptiFile(const xptiFile& r, xptiWorkingSet* aWorkingSet)
    :   
#ifdef DEBUG
        mDEBUG_WorkingSet(aWorkingSet),
#endif
        mSize(r.mSize),
        mDate(r.mDate),
        mName(nsnull),
        mGuts(nsnull),
        mDirectory(r.mDirectory)
{
    NS_ASSERTION(aWorkingSet,"bad param");
    mName = XPT_STRDUP(aWorkingSet->GetStringArena(), r.mName);

    MOZ_COUNT_CTOR(xptiFile);
}

xptiFile::~xptiFile()
{
    MOZ_COUNT_DTOR(xptiFile);
}

PRBool 
xptiFile::SetHeader(XPTHeader* aHeader, xptiWorkingSet* aWorkingSet)
{
    NS_ASSERTION(!mGuts,"bad state");
    NS_ASSERTION(aHeader,"bad param");
    NS_ASSERTION(aWorkingSet,"bad param");

    mGuts = xptiTypelibGuts::NewGuts(aHeader, aWorkingSet);
    return mGuts != nsnull;
}

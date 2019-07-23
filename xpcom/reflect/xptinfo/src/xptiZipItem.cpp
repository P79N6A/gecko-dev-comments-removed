








































#include "xptiprivate.h"

xptiZipItem::xptiZipItem()
    :   
#ifdef DEBUG
        mDEBUG_WorkingSet(nsnull),
#endif
        mName(nsnull),
        mGuts(nsnull)
{
    MOZ_COUNT_CTOR(xptiZipItem);
    
}

xptiZipItem::xptiZipItem(const char*     aName,
                         xptiWorkingSet* aWorkingSet)

    :   
#ifdef DEBUG
        mDEBUG_WorkingSet(aWorkingSet),
#endif
        mName(aName),
        mGuts(nsnull)
{
    MOZ_COUNT_CTOR(xptiZipItem);

    NS_ASSERTION(aWorkingSet,"bad param");
    mName = XPT_STRDUP(aWorkingSet->GetStringArena(), aName);
}

xptiZipItem::xptiZipItem(const xptiZipItem& r, xptiWorkingSet* aWorkingSet)
    :   
#ifdef DEBUG
        mDEBUG_WorkingSet(aWorkingSet),
#endif
        mName(nsnull),
        mGuts(nsnull)
{
    MOZ_COUNT_CTOR(xptiZipItem);

    NS_ASSERTION(aWorkingSet,"bad param");
    mName = XPT_STRDUP(aWorkingSet->GetStringArena(), r.mName);
}

xptiZipItem::~xptiZipItem()
{
    MOZ_COUNT_DTOR(xptiZipItem);
}

PRBool 
xptiZipItem::SetHeader(XPTHeader* aHeader, xptiWorkingSet* aWorkingSet)
{
    NS_ASSERTION(!mGuts,"bad state");
    NS_ASSERTION(aHeader,"bad param");
    NS_ASSERTION(aWorkingSet,"bad param");

    mGuts = xptiTypelibGuts::NewGuts(aHeader, aWorkingSet);
    return mGuts != nsnull;
}

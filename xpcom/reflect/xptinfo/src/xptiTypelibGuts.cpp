








































#include "xptiprivate.h"


xptiTypelibGuts* 
xptiTypelibGuts::NewGuts(XPTHeader* aHeader,
                         xptiWorkingSet* aWorkingSet)
{
    NS_ASSERTION(aHeader, "bad param");
    void* place = XPT_MALLOC(aWorkingSet->GetStructArena(),
                             sizeof(xptiTypelibGuts) + 
                             (sizeof(xptiInterfaceEntry*) *
                              (aHeader->num_interfaces - 1)));
    if(!place)
        return nsnull;
    return new(place) xptiTypelibGuts(aHeader);
}

xptiTypelibGuts::xptiTypelibGuts(XPTHeader* aHeader)
     :  mHeader(aHeader) 
{
    
}

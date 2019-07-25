








































#include "xptiprivate.h"


xptiTypelibGuts* 
xptiTypelibGuts::Create(XPTHeader* aHeader)
{
    NS_ASSERTION(aHeader, "bad param");
    void* place = XPT_MALLOC(gXPTIStructArena,
                             sizeof(xptiTypelibGuts) + 
                             (sizeof(xptiInterfaceEntry*) *
                              (aHeader->num_interfaces - 1)));
    if(!place)
        return nsnull;
    return new(place) xptiTypelibGuts(aHeader);
}

xptiInterfaceEntry*
xptiTypelibGuts::GetEntryAt(PRUint16 i)
{
    static const nsID zeroIID =
        { 0x0, 0x0, 0x0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } };

    NS_ASSERTION(mHeader, "bad state");
    NS_ASSERTION(i < GetEntryCount(), "bad index");

    xptiInterfaceEntry* r = mEntryArray[i];
    if (r)
        return r;

    XPTInterfaceDirectoryEntry* iface = mHeader->interface_directory + i;

    xptiWorkingSet* set =
        xptiInterfaceInfoManager::GetSingleton()->GetWorkingSet();

    if (iface->iid.Equals(zeroIID))
        r = set->mNameTable.Get(iface->name);
    else
        r = set->mIIDTable.Get(iface->iid);

    if (r)
        SetEntryAt(i, r);

    return r;
}

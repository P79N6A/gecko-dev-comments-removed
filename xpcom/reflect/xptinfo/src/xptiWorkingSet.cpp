








































#include "xptiprivate.h"
#include "nsString.h"

using namespace mozilla;

#define XPTI_STRUCT_ARENA_BLOCK_SIZE    (1024 * 1)
#define XPTI_HASHTABLE_SIZE             2048

xptiWorkingSet::xptiWorkingSet()
    : mTableLock("xptiWorkingSet::mTableLock")
{
    MOZ_COUNT_CTOR(xptiWorkingSet);

    mIIDTable.Init(XPTI_HASHTABLE_SIZE);
    mNameTable.Init(XPTI_HASHTABLE_SIZE);

    gXPTIStructArena = XPT_NewArena(XPTI_STRUCT_ARENA_BLOCK_SIZE, sizeof(double),
                                    "xptiWorkingSet structs");
}        

static PLDHashOperator
xpti_Invalidator(const char* keyname, xptiInterfaceEntry* entry, void* arg)
{
    entry->LockedInvalidateInterfaceInfo();
    return PL_DHASH_NEXT;
}

void 
xptiWorkingSet::InvalidateInterfaceInfos()
{
    MutexAutoLock lock(mTableLock);
    mNameTable.EnumerateRead(xpti_Invalidator, NULL);
}        

xptiWorkingSet::~xptiWorkingSet()
{
    MOZ_COUNT_DTOR(xptiWorkingSet);

    
    
#ifdef NS_FREE_PERMANENT_DATA
    XPT_DestroyArena(gXPTIStructArena);
#endif
}        

XPTArena* gXPTIStructArena;

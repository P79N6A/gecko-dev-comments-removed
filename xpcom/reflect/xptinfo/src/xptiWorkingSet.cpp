






#include "mozilla/XPTInterfaceInfoManager.h"

#include "xptiprivate.h"
#include "nsString.h"

using namespace mozilla;

#define XPTI_STRUCT_ARENA_BLOCK_SIZE    (1024 * 16)
#define XPTI_HASHTABLE_SIZE             2048

XPTInterfaceInfoManager::xptiWorkingSet::xptiWorkingSet()
    : mTableReentrantMonitor("xptiWorkingSet::mTableReentrantMonitor")
    , mIIDTable(XPTI_HASHTABLE_SIZE)
    , mNameTable(XPTI_HASHTABLE_SIZE)
{
    MOZ_COUNT_CTOR(xptiWorkingSet);

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
XPTInterfaceInfoManager::xptiWorkingSet::InvalidateInterfaceInfos()
{
    ReentrantMonitorAutoEnter monitor(mTableReentrantMonitor);
    mNameTable.EnumerateRead(xpti_Invalidator, nullptr);
}        

XPTInterfaceInfoManager::xptiWorkingSet::~xptiWorkingSet()
{
    MOZ_COUNT_DTOR(xptiWorkingSet);

    
    
#ifdef NS_FREE_PERMANENT_DATA
    XPT_DestroyArena(gXPTIStructArena);
#endif
}

XPTArena* gXPTIStructArena;

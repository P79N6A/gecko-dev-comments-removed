







#include "mozilla/XPTInterfaceInfoManager.h"

#include "xptiprivate.h"
#include "nsString.h"

using namespace mozilla;

#define XPTI_STRUCT_ARENA_BLOCK_SIZE    (1024 * 16)
#define XPTI_HASHTABLE_LENGTH           1024

XPTInterfaceInfoManager::xptiWorkingSet::xptiWorkingSet()
    : mTableReentrantMonitor("xptiWorkingSet::mTableReentrantMonitor")
    , mIIDTable(XPTI_HASHTABLE_LENGTH)
    , mNameTable(XPTI_HASHTABLE_LENGTH)
{
    MOZ_COUNT_CTOR(xptiWorkingSet);

    gXPTIStructArena = XPT_NewArena(XPTI_STRUCT_ARENA_BLOCK_SIZE, sizeof(double),
                                    "xptiWorkingSet structs");
}

void
XPTInterfaceInfoManager::xptiWorkingSet::InvalidateInterfaceInfos()
{
    ReentrantMonitorAutoEnter monitor(mTableReentrantMonitor);
    for (auto iter = mNameTable.Iter(); !iter.Done(); iter.Next()) {
        xptiInterfaceEntry* entry = iter.UserData();
        entry->LockedInvalidateInterfaceInfo();
    }
}

XPTInterfaceInfoManager::xptiWorkingSet::~xptiWorkingSet()
{
    MOZ_COUNT_DTOR(xptiWorkingSet);

    
    
#ifdef NS_FREE_PERMANENT_DATA
    XPT_DestroyArena(gXPTIStructArena);
#endif
}

XPTArena* gXPTIStructArena;

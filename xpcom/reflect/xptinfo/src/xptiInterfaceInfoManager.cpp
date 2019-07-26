







#include "mozilla/XPTInterfaceInfoManager.h"

#include "mozilla/FileUtils.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/StaticPtr.h"

#include "xptiprivate.h"
#include "nsDependentString.h"
#include "nsString.h"
#include "nsISupportsArray.h"
#include "nsArrayEnumerator.h"
#include "nsDirectoryService.h"
#include "nsIMemoryReporter.h"

using namespace mozilla;

NS_IMPL_ISUPPORTS(
  XPTInterfaceInfoManager,
  nsIInterfaceInfoManager,
  nsIMemoryReporter)

static StaticRefPtr<XPTInterfaceInfoManager> gInterfaceInfoManager;

size_t
XPTInterfaceInfoManager::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf)
{
    size_t n = aMallocSizeOf(this);
    ReentrantMonitorAutoEnter monitor(mWorkingSet.mTableReentrantMonitor);
    
    
    n += mWorkingSet.mIIDTable.SizeOfExcludingThis(nullptr, aMallocSizeOf);
    n += mWorkingSet.mNameTable.SizeOfExcludingThis(nullptr, aMallocSizeOf);
    return n;
}

MOZ_DEFINE_MALLOC_SIZE_OF(XPTIMallocSizeOf)

NS_IMETHODIMP
XPTInterfaceInfoManager::CollectReports(nsIHandleReportCallback* aHandleReport,
                                        nsISupports* aData)
{
    size_t amount = SizeOfIncludingThis(XPTIMallocSizeOf);

    
    
    
    amount += XPT_SizeOfArena(gXPTIStructArena, XPTIMallocSizeOf);

    return MOZ_COLLECT_REPORT(
        "explicit/xpti-working-set", KIND_HEAP, UNITS_BYTES, amount,
        "Memory used by the XPCOM typelib system.");
}


XPTInterfaceInfoManager*
XPTInterfaceInfoManager::GetSingleton()
{
    if (!gInterfaceInfoManager) {
        gInterfaceInfoManager = new XPTInterfaceInfoManager();
        gInterfaceInfoManager->InitMemoryReporter();
    }
    return gInterfaceInfoManager;
}

void
XPTInterfaceInfoManager::FreeInterfaceInfoManager()
{
    gInterfaceInfoManager = nullptr;
}

XPTInterfaceInfoManager::XPTInterfaceInfoManager()
    :   mWorkingSet(),
        mResolveLock("XPTInterfaceInfoManager.mResolveLock")
{
}

XPTInterfaceInfoManager::~XPTInterfaceInfoManager()
{
    
    mWorkingSet.InvalidateInterfaceInfos();

    UnregisterWeakMemoryReporter(this);
}

void
XPTInterfaceInfoManager::InitMemoryReporter()
{
    RegisterWeakMemoryReporter(this);
}

void
XPTInterfaceInfoManager::RegisterBuffer(char *buf, uint32_t length)
{
    XPTState *state = XPT_NewXDRState(XPT_DECODE, buf, length);
    if (!state)
        return;

    XPTCursor cursor;
    if (!XPT_MakeCursor(state, XPT_HEADER, 0, &cursor)) {
        XPT_DestroyXDRState(state);
        return;
    }

    XPTHeader *header = nullptr;
    if (XPT_DoHeader(gXPTIStructArena, &cursor, &header)) {
        RegisterXPTHeader(header);
    }

    XPT_DestroyXDRState(state);
}

void
XPTInterfaceInfoManager::RegisterXPTHeader(XPTHeader* aHeader)
{
    if (aHeader->major_version >= XPT_MAJOR_INCOMPATIBLE_VERSION) {
        NS_ASSERTION(!aHeader->num_interfaces,"bad libxpt");
        LOG_AUTOREG(("      file is version %d.%d  Type file of version %d.0 or higher can not be read.\n", (int)header->major_version, (int)header->minor_version, (int)XPT_MAJOR_INCOMPATIBLE_VERSION));
    }

    xptiTypelibGuts* typelib = xptiTypelibGuts::Create(aHeader);

    ReentrantMonitorAutoEnter monitor(mWorkingSet.mTableReentrantMonitor);
    for(uint16_t k = 0; k < aHeader->num_interfaces; k++)
        VerifyAndAddEntryIfNew(aHeader->interface_directory + k, k, typelib);
}

void
XPTInterfaceInfoManager::VerifyAndAddEntryIfNew(XPTInterfaceDirectoryEntry* iface,
                                                uint16_t idx,
                                                xptiTypelibGuts* typelib)
{
    if (!iface->interface_descriptor)
        return;

    
    
    
    if (iface->interface_descriptor->num_methods > 250 &&
            !(XPT_ID_IS_BUILTINCLASS(iface->interface_descriptor->flags))) {
        NS_ASSERTION(0, "Too many methods to handle for the stub, cannot load");
        fprintf(stderr, "ignoring too large interface: %s\n", iface->name);
        return;
    }
    
    mWorkingSet.mTableReentrantMonitor.AssertCurrentThreadIn();
    xptiInterfaceEntry* entry = mWorkingSet.mIIDTable.Get(iface->iid);
    if (entry) {
        
        LOG_AUTOREG(("      ignoring repeated interface: %s\n", iface->name));
        return;
    }
    
    

    entry = xptiInterfaceEntry::Create(iface->name,
                                       iface->iid,
                                       iface->interface_descriptor,
                                       typelib);
    if (!entry)
        return;

    
    entry->SetScriptableFlag(XPT_ID_IS_SCRIPTABLE(iface->interface_descriptor->flags));
    entry->SetBuiltinClassFlag(XPT_ID_IS_BUILTINCLASS(iface->interface_descriptor->flags));

    mWorkingSet.mIIDTable.Put(entry->IID(), entry);
    mWorkingSet.mNameTable.Put(entry->GetTheName(), entry);

    typelib->SetEntryAt(idx, entry);

    LOG_AUTOREG(("      added interface: %s\n", iface->name));
}


static nsresult 
EntryToInfo(xptiInterfaceEntry* entry, nsIInterfaceInfo **_retval)
{
    if (!entry) {
        *_retval = nullptr;
        return NS_ERROR_FAILURE;    
    }

    nsRefPtr<xptiInterfaceInfo> info = entry->InterfaceInfo();
    info.forget(_retval);
    return NS_OK;    
}

xptiInterfaceEntry*
XPTInterfaceInfoManager::GetInterfaceEntryForIID(const nsIID *iid)
{
    ReentrantMonitorAutoEnter monitor(mWorkingSet.mTableReentrantMonitor);
    return mWorkingSet.mIIDTable.Get(*iid);
}


NS_IMETHODIMP
XPTInterfaceInfoManager::GetInfoForIID(const nsIID * iid, nsIInterfaceInfo **_retval)
{
    NS_ASSERTION(iid, "bad param");
    NS_ASSERTION(_retval, "bad param");

    ReentrantMonitorAutoEnter monitor(mWorkingSet.mTableReentrantMonitor);
    xptiInterfaceEntry* entry = mWorkingSet.mIIDTable.Get(*iid);
    return EntryToInfo(entry, _retval);
}


NS_IMETHODIMP
XPTInterfaceInfoManager::GetInfoForName(const char *name, nsIInterfaceInfo **_retval)
{
    NS_ASSERTION(name, "bad param");
    NS_ASSERTION(_retval, "bad param");

    ReentrantMonitorAutoEnter monitor(mWorkingSet.mTableReentrantMonitor);
    xptiInterfaceEntry* entry = mWorkingSet.mNameTable.Get(name);
    return EntryToInfo(entry, _retval);
}


NS_IMETHODIMP
XPTInterfaceInfoManager::GetIIDForName(const char *name, nsIID * *_retval)
{
    NS_ASSERTION(name, "bad param");
    NS_ASSERTION(_retval, "bad param");

    ReentrantMonitorAutoEnter monitor(mWorkingSet.mTableReentrantMonitor);
    xptiInterfaceEntry* entry = mWorkingSet.mNameTable.Get(name);
    if (!entry) {
        *_retval = nullptr;
        return NS_ERROR_FAILURE;    
    }

    return entry->GetIID(_retval);
}


NS_IMETHODIMP
XPTInterfaceInfoManager::GetNameForIID(const nsIID * iid, char **_retval)
{
    NS_ASSERTION(iid, "bad param");
    NS_ASSERTION(_retval, "bad param");

    ReentrantMonitorAutoEnter monitor(mWorkingSet.mTableReentrantMonitor);
    xptiInterfaceEntry* entry = mWorkingSet.mIIDTable.Get(*iid);
    if (!entry) {
        *_retval = nullptr;
        return NS_ERROR_FAILURE;    
    }

    return entry->GetName(_retval);
}

static PLDHashOperator
xpti_ArrayAppender(const char* name, xptiInterfaceEntry* entry, void* arg)
{
    nsCOMArray<nsIInterfaceInfo>* array = static_cast<nsCOMArray<nsIInterfaceInfo>*>(arg);

    if (entry->GetScriptableFlag()) {
        nsCOMPtr<nsIInterfaceInfo> ii = entry->InterfaceInfo();
        array->AppendElement(ii);
    }
    return PL_DHASH_NEXT;
}


void
XPTInterfaceInfoManager::GetScriptableInterfaces(nsCOMArray<nsIInterfaceInfo>& aInterfaces)
{
    
    
    
    

    ReentrantMonitorAutoEnter monitor(mWorkingSet.mTableReentrantMonitor);
    aInterfaces.SetCapacity(mWorkingSet.mNameTable.Count());
    mWorkingSet.mNameTable.EnumerateRead(xpti_ArrayAppender, &aInterfaces);
}

struct ArrayAndPrefix
{
    nsISupportsArray* array;
    const char*       prefix;
    uint32_t          length;
};

static PLDHashOperator
xpti_ArrayPrefixAppender(const char* keyname, xptiInterfaceEntry* entry, void* arg)
{
    ArrayAndPrefix* args = (ArrayAndPrefix*) arg;

    const char* name = entry->GetTheName();
    if (name != PL_strnstr(name, args->prefix, args->length))
        return PL_DHASH_NEXT;

    nsCOMPtr<nsIInterfaceInfo> ii;
    if (NS_SUCCEEDED(EntryToInfo(entry, getter_AddRefs(ii))))
        args->array->AppendElement(ii);
    return PL_DHASH_NEXT;
}


NS_IMETHODIMP
XPTInterfaceInfoManager::EnumerateInterfacesWhoseNamesStartWith(const char *prefix, nsIEnumerator **_retval)
{
    nsCOMPtr<nsISupportsArray> array;
    NS_NewISupportsArray(getter_AddRefs(array));
    if (!array)
        return NS_ERROR_UNEXPECTED;

    ReentrantMonitorAutoEnter monitor(mWorkingSet.mTableReentrantMonitor);
    ArrayAndPrefix args = {array, prefix, static_cast<uint32_t>(strlen(prefix))};
    mWorkingSet.mNameTable.EnumerateRead(xpti_ArrayPrefixAppender, &args);

    return array->Enumerate(_retval);
}


NS_IMETHODIMP
XPTInterfaceInfoManager::AutoRegisterInterfaces()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

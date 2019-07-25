








































#include "xptiprivate.h"
#include "nsDependentString.h"
#include "nsString.h"
#include "nsISupportsArray.h"
#include "nsArrayEnumerator.h"
#include "mozilla/FunctionTimer.h"
#include "nsDirectoryService.h"

using namespace mozilla;

NS_IMPL_THREADSAFE_ISUPPORTS2(xptiInterfaceInfoManager, 
                              nsIInterfaceInfoManager,
                              nsIInterfaceInfoSuperManager)

static xptiInterfaceInfoManager* gInterfaceInfoManager = nsnull;
#ifdef DEBUG
static int gCallCount = 0;
#endif


xptiInterfaceInfoManager*
xptiInterfaceInfoManager::GetSingleton()
{
    if (!gInterfaceInfoManager) {
        NS_TIME_FUNCTION;

        gInterfaceInfoManager = new xptiInterfaceInfoManager();
        NS_ADDREF(gInterfaceInfoManager);
    }
    return gInterfaceInfoManager;
}

void
xptiInterfaceInfoManager::FreeInterfaceInfoManager()
{
    NS_IF_RELEASE(gInterfaceInfoManager);
}

xptiInterfaceInfoManager::xptiInterfaceInfoManager()
    :   mWorkingSet(),
        mResolveLock("xptiInterfaceInfoManager.mResolveLock"),
        mAutoRegLock("xptiInterfaceInfoManager.mAutoRegLock"), 
        mInfoMonitor("xptiInterfaceInfoManager.mInfoMonitor"),
        mAdditionalManagersLock(
            "xptiInterfaceInfoManager.mAdditionalManagersLock")
{
}

xptiInterfaceInfoManager::~xptiInterfaceInfoManager()
{
    
    mWorkingSet.InvalidateInterfaceInfos();

    gInterfaceInfoManager = nsnull;
#ifdef DEBUG
    gCallCount = 0;
#endif
}

namespace {

struct AutoCloseFD
{
    AutoCloseFD()
        : mFD(NULL)
    { }
    ~AutoCloseFD() {
        if (mFD)
            PR_Close(mFD);
    }
    operator PRFileDesc*() {
        return mFD;
    }

    PRFileDesc** operator&() {
        NS_ASSERTION(!mFD, "Re-opening a file");
        return &mFD;
    }

    PRFileDesc* mFD;
};

} 

XPTHeader* 
xptiInterfaceInfoManager::ReadXPTFile(nsILocalFile* aFile)
{
    AutoCloseFD fd;
    if (NS_FAILED(aFile->OpenNSPRFileDesc(PR_RDONLY, 0444, &fd)) || !fd)
        return NULL;

    PRFileInfo64 fileInfo;
    if (PR_SUCCESS != PR_GetOpenFileInfo64(fd, &fileInfo))
        return NULL;

    if (fileInfo.size > PRInt64(PR_INT32_MAX))
        return NULL;

    nsAutoArrayPtr<char> whole(new char[PRInt32(fileInfo.size)]);
    if (!whole)
        return nsnull;

    for (PRInt32 totalRead = 0; totalRead < fileInfo.size; ) {
        PRInt32 read = PR_Read(fd, whole + totalRead, PRInt32(fileInfo.size));
        if (read < 0)
            return NULL;
        totalRead += read;
    }

    XPTState* state = XPT_NewXDRState(XPT_DECODE, whole,
                                      PRInt32(fileInfo.size));

    XPTCursor cursor;
    if (!XPT_MakeCursor(state, XPT_HEADER, 0, &cursor)) {
        XPT_DestroyXDRState(state);
        return NULL;
    }
    
    XPTHeader *header = nsnull;
    if (!XPT_DoHeader(gXPTIStructArena, &cursor, &header)) {
        XPT_DestroyXDRState(state);
        return NULL;
    }

    XPT_DestroyXDRState(state);

    return header;
}

XPTHeader*
xptiInterfaceInfoManager::ReadXPTFileFromInputStream(nsIInputStream *stream)
{
    PRUint32 flen;
    stream->Available(&flen);
    
    nsAutoArrayPtr<char> whole(new char[flen]);
    if (!whole)
        return nsnull;

    for (PRUint32 totalRead = 0; totalRead < flen; ) {
        PRUint32 avail;
        PRUint32 read;

        if (NS_FAILED(stream->Available(&avail)))
            return NULL;

        if (avail > flen)
            return NULL;

        if (NS_FAILED(stream->Read(whole+totalRead, avail, &read)))
            return NULL;

        totalRead += read;
    }
    
    XPTState *state = XPT_NewXDRState(XPT_DECODE, whole, flen);
    if (!state)
        return NULL;
    
    XPTCursor cursor;
    if (!XPT_MakeCursor(state, XPT_HEADER, 0, &cursor)) {
        XPT_DestroyXDRState(state);
        return NULL;
    }
    
    XPTHeader *header = nsnull;
    if (!XPT_DoHeader(gXPTIStructArena, &cursor, &header)) {
        XPT_DestroyXDRState(state);
        return NULL;
    }

    XPT_DestroyXDRState(state);

    return header;
}

void
xptiInterfaceInfoManager::RegisterFile(nsILocalFile* aFile)
{
    XPTHeader* header = ReadXPTFile(aFile);
    if (!header)
        return;

    RegisterXPTHeader(header);
}

void
xptiInterfaceInfoManager::RegisterXPTHeader(XPTHeader* aHeader)
{
    if (aHeader->major_version >= XPT_MAJOR_INCOMPATIBLE_VERSION) {
        NS_ASSERTION(!aHeader->num_interfaces,"bad libxpt");
        LOG_AUTOREG(("      file is version %d.%d  Type file of version %d.0 or higher can not be read.\n", (int)header->major_version, (int)header->minor_version, (int)XPT_MAJOR_INCOMPATIBLE_VERSION));
    }

    xptiTypelibGuts* typelib = xptiTypelibGuts::Create(aHeader);

    for(PRUint16 k = 0; k < aHeader->num_interfaces; k++)
        VerifyAndAddEntryIfNew(aHeader->interface_directory + k, k, typelib);
}

void
xptiInterfaceInfoManager::RegisterInputStream(nsIInputStream* aStream)
{
    XPTHeader* header = ReadXPTFileFromInputStream(aStream);
    if (header)
        RegisterXPTHeader(header);
}

void
xptiInterfaceInfoManager::VerifyAndAddEntryIfNew(XPTInterfaceDirectoryEntry* iface,
                                                 PRUint16 idx,
                                                 xptiTypelibGuts* typelib)
{
    if (!iface->interface_descriptor)
        return;
    
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

    mWorkingSet.mIIDTable.Put(entry->IID(), entry);
    mWorkingSet.mNameTable.Put(entry->GetTheName(), entry);

    typelib->SetEntryAt(idx, entry);

    LOG_AUTOREG(("      added interface: %s\n", iface->name));
}


static nsresult 
EntryToInfo(xptiInterfaceEntry* entry, nsIInterfaceInfo **_retval)
{
    xptiInterfaceInfo* info;
    nsresult rv;

    if (!entry) {
        *_retval = nsnull;
        return NS_ERROR_FAILURE;    
    }

    rv = entry->GetInterfaceInfo(&info);
    if (NS_FAILED(rv))
        return rv;

    
    *_retval = static_cast<nsIInterfaceInfo*>(info);
    return NS_OK;    
}

xptiInterfaceEntry*
xptiInterfaceInfoManager::GetInterfaceEntryForIID(const nsIID *iid)
{
    return mWorkingSet.mIIDTable.Get(*iid);
}


NS_IMETHODIMP xptiInterfaceInfoManager::GetInfoForIID(const nsIID * iid, nsIInterfaceInfo **_retval)
{
    NS_ASSERTION(iid, "bad param");
    NS_ASSERTION(_retval, "bad param");

    xptiInterfaceEntry* entry = GetInterfaceEntryForIID(iid);
    return EntryToInfo(entry, _retval);
}


NS_IMETHODIMP xptiInterfaceInfoManager::GetInfoForName(const char *name, nsIInterfaceInfo **_retval)
{
    NS_ASSERTION(name, "bad param");
    NS_ASSERTION(_retval, "bad param");

    xptiInterfaceEntry* entry = mWorkingSet.mNameTable.Get(name);
    return EntryToInfo(entry, _retval);
}


NS_IMETHODIMP xptiInterfaceInfoManager::GetIIDForName(const char *name, nsIID * *_retval)
{
    NS_ASSERTION(name, "bad param");
    NS_ASSERTION(_retval, "bad param");


    xptiInterfaceEntry* entry = mWorkingSet.mNameTable.Get(name);
    if (!entry) {
        *_retval = nsnull;
        return NS_ERROR_FAILURE;    
    }

    return entry->GetIID(_retval);
}


NS_IMETHODIMP xptiInterfaceInfoManager::GetNameForIID(const nsIID * iid, char **_retval)
{
    NS_ASSERTION(iid, "bad param");
    NS_ASSERTION(_retval, "bad param");

    xptiInterfaceEntry* entry = mWorkingSet.mIIDTable.Get(*iid);
    if (!entry) {
        *_retval = nsnull;
        return NS_ERROR_FAILURE;    
    }

    return entry->GetName(_retval);
}

static PLDHashOperator
xpti_ArrayAppender(const char* name, xptiInterfaceEntry* entry, void* arg)
{
    nsISupportsArray* array = (nsISupportsArray*) arg;

    nsCOMPtr<nsIInterfaceInfo> ii;
    if (NS_SUCCEEDED(EntryToInfo(entry, getter_AddRefs(ii))))
        array->AppendElement(ii);
    return PL_DHASH_NEXT;
}


NS_IMETHODIMP xptiInterfaceInfoManager::EnumerateInterfaces(nsIEnumerator **_retval)
{
    
    
    
    

    nsCOMPtr<nsISupportsArray> array;
    NS_NewISupportsArray(getter_AddRefs(array));
    if (!array)
        return NS_ERROR_UNEXPECTED;

    mWorkingSet.mNameTable.EnumerateRead(xpti_ArrayAppender, array);

    return array->Enumerate(_retval);
}

struct ArrayAndPrefix
{
    nsISupportsArray* array;
    const char*       prefix;
    PRUint32          length;
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


NS_IMETHODIMP xptiInterfaceInfoManager::EnumerateInterfacesWhoseNamesStartWith(const char *prefix, nsIEnumerator **_retval)
{
    nsCOMPtr<nsISupportsArray> array;
    NS_NewISupportsArray(getter_AddRefs(array));
    if (!array)
        return NS_ERROR_UNEXPECTED;

    ArrayAndPrefix args = {array, prefix, PL_strlen(prefix)};
    mWorkingSet.mNameTable.EnumerateRead(xpti_ArrayPrefixAppender, &args);

    return array->Enumerate(_retval);
}


NS_IMETHODIMP xptiInterfaceInfoManager::AutoRegisterInterfaces()
{
    NS_TIME_FUNCTION;

    return NS_ERROR_NOT_IMPLEMENTED;
}




NS_IMETHODIMP xptiInterfaceInfoManager::AddAdditionalManager(nsIInterfaceInfoManager *manager)
{
    nsCOMPtr<nsIWeakReference> weakRef = do_GetWeakReference(manager);
    nsISupports* ptrToAdd = weakRef ? 
                    static_cast<nsISupports*>(weakRef) :
                    static_cast<nsISupports*>(manager);
    { 
        MutexAutoLock lock(mAdditionalManagersLock);
        if (mAdditionalManagers.IndexOf(ptrToAdd) != -1)
            return NS_ERROR_FAILURE;
        if (!mAdditionalManagers.AppendObject(ptrToAdd))
            return NS_ERROR_OUT_OF_MEMORY;
    }
    return NS_OK;
}


NS_IMETHODIMP xptiInterfaceInfoManager::RemoveAdditionalManager(nsIInterfaceInfoManager *manager)
{
    nsCOMPtr<nsIWeakReference> weakRef = do_GetWeakReference(manager);
    nsISupports* ptrToRemove = weakRef ? 
                    static_cast<nsISupports*>(weakRef) :
                    static_cast<nsISupports*>(manager);
    { 
        MutexAutoLock lock(mAdditionalManagersLock);
        if (!mAdditionalManagers.RemoveObject(ptrToRemove))
            return NS_ERROR_FAILURE;
    }
    return NS_OK;
}


NS_IMETHODIMP xptiInterfaceInfoManager::HasAdditionalManagers(PRBool *_retval)
{
    *_retval = mAdditionalManagers.Count() > 0;
    return NS_OK;
}


NS_IMETHODIMP xptiInterfaceInfoManager::EnumerateAdditionalManagers(nsISimpleEnumerator **_retval)
{
    MutexAutoLock lock(mAdditionalManagersLock);

    nsCOMArray<nsISupports> managerArray(mAdditionalManagers);
    
    for(PRInt32 i = managerArray.Count(); i--; ) {
        nsISupports *raw = managerArray.ObjectAt(i);
        if (!raw)
            return NS_ERROR_FAILURE;
        nsCOMPtr<nsIWeakReference> weakRef = do_QueryInterface(raw);
        if (weakRef) {
            nsCOMPtr<nsIInterfaceInfoManager> manager = 
                do_QueryReferent(weakRef);
            if (manager) {
                if (!managerArray.ReplaceObjectAt(manager, i))
                    return NS_ERROR_FAILURE;
            }
            else {
                
                mAdditionalManagers.RemoveObjectAt(i);
                managerArray.RemoveObjectAt(i);
            }
        }
    }
    
    return NS_NewArrayEnumerator(_retval, managerArray);
}

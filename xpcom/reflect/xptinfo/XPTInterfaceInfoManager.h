





#ifndef mozilla_XPTInterfaceInfoManager_h_
#define mozilla_XPTInterfaceInfoManager_h_

#include "nsIInterfaceInfoManager.h"
#include "nsIMemoryReporter.h"

#include "mozilla/MemoryReporting.h"
#include "mozilla/Mutex.h"
#include "mozilla/ReentrantMonitor.h"
#include "nsDataHashtable.h"

template<typename T> class nsCOMArray;
class nsIMemoryReporter;
struct XPTHeader;
struct XPTInterfaceDirectoryEntry;
class xptiInterfaceEntry;
class xptiInterfaceInfo;
class xptiTypelibGuts;

namespace mozilla {

class XPTInterfaceInfoManager final
    : public nsIInterfaceInfoManager
    , public nsIMemoryReporter
{
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIINTERFACEINFOMANAGER
    NS_DECL_NSIMEMORYREPORTER

public:
    
    static XPTInterfaceInfoManager* GetSingleton();
    static void FreeInterfaceInfoManager();

    void GetScriptableInterfaces(nsCOMArray<nsIInterfaceInfo>& aInterfaces);

    void RegisterBuffer(char *buf, uint32_t length);

    static Mutex& GetResolveLock()
    {
        return GetSingleton()->mResolveLock;
    }

    xptiInterfaceEntry* GetInterfaceEntryForIID(const nsIID *iid);

    size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf);

private:
    XPTInterfaceInfoManager();
    ~XPTInterfaceInfoManager();

    void InitMemoryReporter();

    void RegisterXPTHeader(XPTHeader* aHeader);

    
    void VerifyAndAddEntryIfNew(XPTInterfaceDirectoryEntry* iface,
                                uint16_t idx,
                                xptiTypelibGuts* typelib);

private:

    class xptiWorkingSet
    {
    public:
        xptiWorkingSet();
        ~xptiWorkingSet();

        bool IsValid() const;

        void InvalidateInterfaceInfos();
        void ClearHashTables();

        

        enum {NOT_FOUND = 0xffffffff};

        

        uint32_t GetDirectoryCount();
        nsresult GetCloneOfDirectoryAt(uint32_t i, nsIFile** dir);
        nsresult GetDirectoryAt(uint32_t i, nsIFile** dir);
        bool     FindDirectory(nsIFile* dir, uint32_t* index);
        bool     FindDirectoryOfFile(nsIFile* file, uint32_t* index);
        bool     DirectoryAtMatchesPersistentDescriptor(uint32_t i, const char* desc);

    private:
        uint32_t        mFileCount;
        uint32_t        mMaxFileCount;

    public:
        
        
        
        
        
        mozilla::ReentrantMonitor mTableReentrantMonitor;
        nsDataHashtable<nsIDHashKey, xptiInterfaceEntry*> mIIDTable;
        nsDataHashtable<nsDepCharHashKey, xptiInterfaceEntry*> mNameTable;
    };

    
    friend class ::xptiInterfaceInfo;
    friend class ::xptiInterfaceEntry;
    friend class ::xptiTypelibGuts;

    xptiWorkingSet               mWorkingSet;
    Mutex                        mResolveLock;
};

} 

#endif

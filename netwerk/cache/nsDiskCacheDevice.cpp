





#include <limits.h>

#include "mozilla/DebugOnly.h"

#include "nsCache.h"
#include "nsIMemoryReporter.h"


#if defined(XP_UNIX)
#include <unistd.h>
#elif defined(XP_WIN)
#include <windows.h>
#else

#endif

#include "prthread.h"

#include "private/pprio.h"

#include "nsDiskCacheDevice.h"
#include "nsDiskCacheEntry.h"
#include "nsDiskCacheMap.h"
#include "nsDiskCacheStreams.h"

#include "nsDiskCache.h"

#include "nsCacheService.h"

#include "nsDeleteDir.h"

#include "nsICacheVisitor.h"
#include "nsReadableUtils.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsCRT.h"
#include "nsCOMArray.h"
#include "nsISimpleEnumerator.h"

#include "nsThreadUtils.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Telemetry.h"

static const char DISK_CACHE_DEVICE_ID[] = { "disk" };
using namespace mozilla;

class nsDiskCacheDeviceDeactivateEntryEvent : public nsRunnable {
public:
    nsDiskCacheDeviceDeactivateEntryEvent(nsDiskCacheDevice *device,
                                          nsCacheEntry * entry,
                                          nsDiskCacheBinding * binding)
        : mCanceled(false),
          mEntry(entry),
          mDevice(device),
          mBinding(binding)
    {
    }

    NS_IMETHOD Run()
    {
        nsCacheServiceAutoLock lock(LOCK_TELEM(NSDISKCACHEDEVICEDEACTIVATEENTRYEVENT_RUN));
#ifdef PR_LOGGING
        CACHE_LOG_DEBUG(("nsDiskCacheDeviceDeactivateEntryEvent[%p]\n", this));
#endif
        if (!mCanceled) {
            (void) mDevice->DeactivateEntry_Private(mEntry, mBinding);
        }
        return NS_OK;
    }

    void CancelEvent() { mCanceled = true; }
private:
    bool mCanceled;
    nsCacheEntry *mEntry;
    nsDiskCacheDevice *mDevice;
    nsDiskCacheBinding *mBinding;
};

class nsEvictDiskCacheEntriesEvent : public nsRunnable {
public:
    explicit nsEvictDiskCacheEntriesEvent(nsDiskCacheDevice *device)
        : mDevice(device) {}

    NS_IMETHOD Run()
    {
        nsCacheServiceAutoLock lock(LOCK_TELEM(NSEVICTDISKCACHEENTRIESEVENT_RUN));
        mDevice->EvictDiskCacheEntries(mDevice->mCacheCapacity);
        return NS_OK;
    }

private:
    nsDiskCacheDevice *mDevice;
};








class nsDiskCacheEvictor : public nsDiskCacheRecordVisitor
{
public:
    nsDiskCacheEvictor( nsDiskCacheMap *      cacheMap,
                        nsDiskCacheBindery *  cacheBindery,
                        uint32_t              targetSize,
                        const char *          clientID)
        : mCacheMap(cacheMap)
        , mBindery(cacheBindery)
        , mTargetSize(targetSize)
        , mClientID(clientID)
    { 
        mClientIDSize = clientID ? strlen(clientID) : 0;
    }
    
    virtual int32_t  VisitRecord(nsDiskCacheRecord *  mapRecord);
 
private:
        nsDiskCacheMap *     mCacheMap;
        nsDiskCacheBindery * mBindery;
        uint32_t             mTargetSize;
        const char *         mClientID;
        uint32_t             mClientIDSize;
};


int32_t
nsDiskCacheEvictor::VisitRecord(nsDiskCacheRecord *  mapRecord)
{
    if (mCacheMap->TotalSize() < mTargetSize)
        return kStopVisitingRecords;
    
    if (mClientID) {
        
        nsDiskCacheEntry * diskEntry = mCacheMap->ReadDiskCacheEntry(mapRecord);
        if (!diskEntry)
            return kVisitNextRecord;  
    
        
        if ((diskEntry->mKeySize <= mClientIDSize) ||
            (diskEntry->Key()[mClientIDSize] != ':') ||
            (memcmp(diskEntry->Key(), mClientID, mClientIDSize) != 0)) {
            return kVisitNextRecord;  
        }
    }
    
    nsDiskCacheBinding * binding = mBindery->FindActiveBinding(mapRecord->HashNumber());
    if (binding) {
        
        
        if (binding->mDeactivateEvent) {
            binding->mDeactivateEvent->CancelEvent();
            binding->mDeactivateEvent = nullptr;
        }
        
        
        
        binding->mDoomed = true;         
        nsCacheService::DoomEntry(binding->mCacheEntry);
    } else {
        
        (void) mCacheMap->DeleteStorage(mapRecord);
    }

    return kDeleteRecordAndContinue;  
}






class nsDiskCacheDeviceInfo : public nsICacheDeviceInfo {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICACHEDEVICEINFO

    explicit nsDiskCacheDeviceInfo(nsDiskCacheDevice* device)
        :   mDevice(device)
    {
    }

private:
    virtual ~nsDiskCacheDeviceInfo() {}

    nsDiskCacheDevice* mDevice;
};

NS_IMPL_ISUPPORTS(nsDiskCacheDeviceInfo, nsICacheDeviceInfo)


NS_IMETHODIMP nsDiskCacheDeviceInfo::GetDescription(char ** aDescription)
{
    NS_ENSURE_ARG_POINTER(aDescription);
    *aDescription = NS_strdup("Disk cache device");
    return *aDescription ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP nsDiskCacheDeviceInfo::GetUsageReport(char ** usageReport)
{
    NS_ENSURE_ARG_POINTER(usageReport);
    nsCString buffer;
    
    buffer.AssignLiteral("  <tr>\n"
                         "    <th>Cache Directory:</th>\n"
                         "    <td>");
    nsCOMPtr<nsIFile> cacheDir;
    nsAutoString path;
    mDevice->getCacheDirectory(getter_AddRefs(cacheDir)); 
    nsresult rv = cacheDir->GetPath(path);
    if (NS_SUCCEEDED(rv)) {
        AppendUTF16toUTF8(path, buffer);
    } else {
        buffer.AppendLiteral("directory unavailable");
    }
    buffer.AppendLiteral("</td>\n"
                         "  </tr>\n");

    *usageReport = ToNewCString(buffer);
    if (!*usageReport) return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}


NS_IMETHODIMP nsDiskCacheDeviceInfo::GetEntryCount(uint32_t *aEntryCount)
{
    NS_ENSURE_ARG_POINTER(aEntryCount);
    *aEntryCount = mDevice->getEntryCount();
    return NS_OK;
}


NS_IMETHODIMP nsDiskCacheDeviceInfo::GetTotalSize(uint32_t *aTotalSize)
{
    NS_ENSURE_ARG_POINTER(aTotalSize);
    
    *aTotalSize = mDevice->getCacheSize() * 1024;
    return NS_OK;
}


NS_IMETHODIMP nsDiskCacheDeviceInfo::GetMaximumSize(uint32_t *aMaximumSize)
{
    NS_ENSURE_ARG_POINTER(aMaximumSize);
    
    *aMaximumSize = mDevice->getCacheCapacity() * 1024;
    return NS_OK;
}


















static inline void hashmix(uint32_t& a, uint32_t& b, uint32_t& c)
{
  a -= b; a -= c; a ^= (c>>13);
  b -= c; b -= a; b ^= (a<<8);
  c -= a; c -= b; c ^= (b>>13);
  a -= b; a -= c; a ^= (c>>12); 
  b -= c; b -= a; b ^= (a<<16);
  c -= a; c -= b; c ^= (b>>5);
  a -= b; a -= c; a ^= (c>>3);
  b -= c; b -= a; b ^= (a<<10);
  c -= a; c -= b; c ^= (b>>15);
}

PLDHashNumber
nsDiskCache::Hash(const char * key, PLDHashNumber initval)
{
  const uint8_t *k = reinterpret_cast<const uint8_t*>(key);
  uint32_t a, b, c, len, length;

  length = strlen(key);
  
  len = length;
  a = b = 0x9e3779b9;  
  c = initval;         

  
  while (len >= 12)
  {
    a += k[0] + (uint32_t(k[1])<<8) + (uint32_t(k[2])<<16) + (uint32_t(k[3])<<24);
    b += k[4] + (uint32_t(k[5])<<8) + (uint32_t(k[6])<<16) + (uint32_t(k[7])<<24);
    c += k[8] + (uint32_t(k[9])<<8) + (uint32_t(k[10])<<16) + (uint32_t(k[11])<<24);
    hashmix(a, b, c);
    k += 12; len -= 12;
  }

  
  c += length;
  switch(len) {              
    case 11: c += (uint32_t(k[10])<<24);
    case 10: c += (uint32_t(k[9])<<16);
    case 9 : c += (uint32_t(k[8])<<8);
    
    case 8 : b += (uint32_t(k[7])<<24);
    case 7 : b += (uint32_t(k[6])<<16);
    case 6 : b += (uint32_t(k[5])<<8);
    case 5 : b += k[4];
    case 4 : a += (uint32_t(k[3])<<24);
    case 3 : a += (uint32_t(k[2])<<16);
    case 2 : a += (uint32_t(k[1])<<8);
    case 1 : a += k[0];
    
  }
  hashmix(a, b, c);

  return c;
}

nsresult
nsDiskCache::Truncate(PRFileDesc *  fd, uint32_t  newEOF)
{
    

#if defined(XP_UNIX)
    if (ftruncate(PR_FileDesc2NativeHandle(fd), newEOF) != 0) {
        NS_ERROR("ftruncate failed");
        return NS_ERROR_FAILURE;
    }

#elif defined(XP_WIN)
    int32_t cnt = PR_Seek(fd, newEOF, PR_SEEK_SET);
    if (cnt == -1)  return NS_ERROR_FAILURE;
    if (!SetEndOfFile((HANDLE) PR_FileDesc2NativeHandle(fd))) {
        NS_ERROR("SetEndOfFile failed");
        return NS_ERROR_FAILURE;
    }

#else
    
#endif
    return NS_OK;
}






nsDiskCacheDevice::nsDiskCacheDevice()
    : mCacheCapacity(0)
    , mMaxEntrySize(-1) 
    , mInitialized(false)
    , mClearingDiskCache(false)
{
}

nsDiskCacheDevice::~nsDiskCacheDevice()
{
    Shutdown();
}





nsresult
nsDiskCacheDevice::Init()
{
    nsresult rv;

    if (Initialized()) {
        NS_ERROR("Disk cache already initialized!");
        return NS_ERROR_UNEXPECTED;
    }
       
    if (!mCacheDirectory)
        return NS_ERROR_FAILURE;

    rv = mBindery.Init();
    if (NS_FAILED(rv))
        return rv;

    
    rv = OpenDiskCache();
    if (NS_FAILED(rv)) {
        (void) mCacheMap.Close(false);
        return rv;
    }

    mInitialized = true;
    return NS_OK;
}





nsresult
nsDiskCacheDevice::Shutdown()
{
    nsCacheService::AssertOwnsLock();

    nsresult rv = Shutdown_Private(true);
    if (NS_FAILED(rv))
        return rv;

    return NS_OK;
}


nsresult
nsDiskCacheDevice::Shutdown_Private(bool    flush)
{
    CACHE_LOG_DEBUG(("CACHE: disk Shutdown_Private [%u]\n", flush));

    if (Initialized()) {
        
        EvictDiskCacheEntries(mCacheCapacity);

        
        
        
        (void) nsCacheService::SyncWithCacheIOThread();

        
        (void) mCacheMap.Close(flush);

        mBindery.Reset();

        mInitialized = false;
    }

    return NS_OK;
}


const char *
nsDiskCacheDevice::GetDeviceID()
{
    return DISK_CACHE_DEVICE_ID;
}










nsCacheEntry *
nsDiskCacheDevice::FindEntry(nsCString * key, bool *collision)
{
    Telemetry::AutoTimer<Telemetry::CACHE_DISK_SEARCH_2> timer;
    if (!Initialized())  return nullptr;  
    if (mClearingDiskCache)  return nullptr;
    nsDiskCacheRecord       record;
    nsDiskCacheBinding *    binding = nullptr;
    PLDHashNumber           hashNumber = nsDiskCache::Hash(key->get());

    *collision = false;

    binding = mBindery.FindActiveBinding(hashNumber);
    if (binding && !binding->mCacheEntry->Key()->Equals(*key)) {
        *collision = true;
        return nullptr;
    } else if (binding && binding->mDeactivateEvent) {
        binding->mDeactivateEvent->CancelEvent();
        binding->mDeactivateEvent = nullptr;
        CACHE_LOG_DEBUG(("CACHE: reusing deactivated entry %p " \
                         "req-key=%s  entry-key=%s\n",
                         binding->mCacheEntry, key, binding->mCacheEntry->Key()));

        return binding->mCacheEntry; 
                                     
                                     
    }
    binding = nullptr;

    
    nsresult rv = mCacheMap.FindRecord(hashNumber, &record);
    if (NS_FAILED(rv))  return nullptr;  
    
    nsDiskCacheEntry * diskEntry = mCacheMap.ReadDiskCacheEntry(&record);
    if (!diskEntry) return nullptr;
    
    
    if (!key->Equals(diskEntry->Key())) {
        *collision = true;
        return nullptr;
    }
    
    nsCacheEntry * entry = diskEntry->CreateCacheEntry(this);
    if (entry) {
        binding = mBindery.CreateBinding(entry, &record);
        if (!binding) {
            delete entry;
            entry = nullptr;
        }
    }

    if (!entry) {
      (void) mCacheMap.DeleteStorage(&record);
      (void) mCacheMap.DeleteRecord(&record);
    }
    
    return entry;
}





nsresult
nsDiskCacheDevice::DeactivateEntry(nsCacheEntry * entry)
{
    nsDiskCacheBinding * binding = GetCacheEntryBinding(entry);
    if (!IsValidBinding(binding))
        return NS_ERROR_UNEXPECTED;

    CACHE_LOG_DEBUG(("CACHE: disk DeactivateEntry [%p %x]\n",
        entry, binding->mRecord.HashNumber()));

    nsDiskCacheDeviceDeactivateEntryEvent *event =
        new nsDiskCacheDeviceDeactivateEntryEvent(this, entry, binding);

    
    binding->mDeactivateEvent = event;

    DebugOnly<nsresult> rv = nsCacheService::DispatchToCacheIOThread(event);
    NS_ASSERTION(NS_SUCCEEDED(rv), "DeactivateEntry: Failed dispatching "
                                   "deactivation event");
    return NS_OK;
}




nsresult
nsDiskCacheDevice::DeactivateEntry_Private(nsCacheEntry * entry,
                                           nsDiskCacheBinding * binding)
{
    nsresult rv = NS_OK;
    if (entry->IsDoomed()) {
        
        rv = mCacheMap.DeleteStorage(&binding->mRecord);

    } else {
        
        rv = mCacheMap.WriteDiskCacheEntry(binding);
        if (NS_FAILED(rv)) {
            
            (void) mCacheMap.DeleteStorage(&binding->mRecord);
            (void) mCacheMap.DeleteRecord(&binding->mRecord);
            binding->mDoomed = true; 
        }
    }

    mBindery.RemoveBinding(binding); 
    delete entry;   
    return rv;
}

















nsresult
nsDiskCacheDevice::BindEntry(nsCacheEntry * entry)
{
    if (!Initialized())  return  NS_ERROR_NOT_INITIALIZED;
    if (mClearingDiskCache)  return NS_ERROR_NOT_AVAILABLE;
    nsresult rv = NS_OK;
    nsDiskCacheRecord record, oldRecord;
    nsDiskCacheBinding *binding;
    PLDHashNumber hashNumber = nsDiskCache::Hash(entry->Key()->get());

    
    
    
    
    binding = mBindery.FindActiveBinding(hashNumber);
    if (binding) {
        NS_ASSERTION(!binding->mCacheEntry->Key()->Equals(*entry->Key()),
                     "BindEntry called for already bound entry!");
        
        if (binding->mDeactivateEvent) {
            binding->mDeactivateEvent->CancelEvent();
            binding->mDeactivateEvent = nullptr;
        }
        nsCacheService::DoomEntry(binding->mCacheEntry);
        binding = nullptr;
    }

    
    
    
    rv = mCacheMap.FindRecord(hashNumber, &record);
    if (NS_SUCCEEDED(rv)) {
        nsDiskCacheEntry * diskEntry = mCacheMap.ReadDiskCacheEntry(&record);
        if (diskEntry) {
            
            if (!entry->Key()->Equals(diskEntry->Key())) {
                mCacheMap.DeleteStorage(&record);
                rv = mCacheMap.DeleteRecord(&record);
                if (NS_FAILED(rv))  return rv;
            }
        }
        record = nsDiskCacheRecord();
    }

    
    record.SetHashNumber(nsDiskCache::Hash(entry->Key()->get()));
    record.SetEvictionRank(ULONG_MAX - SecondsFromPRTime(PR_Now()));

    CACHE_LOG_DEBUG(("CACHE: disk BindEntry [%p %x]\n",
        entry, record.HashNumber()));

    if (!entry->IsDoomed()) {
        
        rv = mCacheMap.AddRecord(&record, &oldRecord); 
        if (NS_FAILED(rv))  return rv;
        
        uint32_t    oldHashNumber = oldRecord.HashNumber();
        if (oldHashNumber) {
            
            nsDiskCacheBinding * oldBinding = mBindery.FindActiveBinding(oldHashNumber);
            if (oldBinding) {
                

                if (!oldBinding->mCacheEntry->IsDoomed()) {
                    
                    if (oldBinding->mDeactivateEvent) {
                        oldBinding->mDeactivateEvent->CancelEvent();
                        oldBinding->mDeactivateEvent = nullptr;
                    }
                
                    nsCacheService::DoomEntry(oldBinding->mCacheEntry);
                    
                }
            } else {
                
                
                rv = mCacheMap.DeleteStorage(&oldRecord);
                if (NS_FAILED(rv))  return rv;  
            }
        }
    }
    
    
    binding = mBindery.CreateBinding(entry, &record);
    NS_ASSERTION(binding, "nsDiskCacheDevice::BindEntry");
    if (!binding) return NS_ERROR_OUT_OF_MEMORY;
    NS_ASSERTION(binding->mRecord.ValidRecord(), "bad cache map record");

    return NS_OK;
}





void
nsDiskCacheDevice::DoomEntry(nsCacheEntry * entry)
{
    CACHE_LOG_DEBUG(("CACHE: disk DoomEntry [%p]\n", entry));

    nsDiskCacheBinding * binding = GetCacheEntryBinding(entry);
    NS_ASSERTION(binding, "DoomEntry: binding == nullptr");
    if (!binding)
        return;

    if (!binding->mDoomed) {
        
#ifdef DEBUG
        nsresult rv =
#endif
            mCacheMap.DeleteRecord(&binding->mRecord);
        NS_ASSERTION(NS_SUCCEEDED(rv),"DeleteRecord failed.");
        binding->mDoomed = true; 
    }
}





nsresult
nsDiskCacheDevice::OpenInputStreamForEntry(nsCacheEntry *      entry,
                                           nsCacheAccessMode   mode, 
                                           uint32_t            offset,
                                           nsIInputStream **   result)
{
    CACHE_LOG_DEBUG(("CACHE: disk OpenInputStreamForEntry [%p %x %u]\n",
        entry, mode, offset));

    NS_ENSURE_ARG_POINTER(entry);
    NS_ENSURE_ARG_POINTER(result);

    nsresult             rv;
    nsDiskCacheBinding * binding = GetCacheEntryBinding(entry);
    if (!IsValidBinding(binding))
        return NS_ERROR_UNEXPECTED;

    NS_ASSERTION(binding->mCacheEntry == entry, "binding & entry don't point to each other");

    rv = binding->EnsureStreamIO();
    if (NS_FAILED(rv)) return rv;

    return binding->mStreamIO->GetInputStream(offset, result);
}





nsresult
nsDiskCacheDevice::OpenOutputStreamForEntry(nsCacheEntry *      entry,
                                            nsCacheAccessMode   mode, 
                                            uint32_t            offset,
                                            nsIOutputStream **  result)
{
    CACHE_LOG_DEBUG(("CACHE: disk OpenOutputStreamForEntry [%p %x %u]\n",
        entry, mode, offset));
 
    NS_ENSURE_ARG_POINTER(entry);
    NS_ENSURE_ARG_POINTER(result);

    nsresult             rv;
    nsDiskCacheBinding * binding = GetCacheEntryBinding(entry);
    if (!IsValidBinding(binding))
        return NS_ERROR_UNEXPECTED;
    
    NS_ASSERTION(binding->mCacheEntry == entry, "binding & entry don't point to each other");

    rv = binding->EnsureStreamIO();
    if (NS_FAILED(rv)) return rv;

    return binding->mStreamIO->GetOutputStream(offset, result);
}





nsresult
nsDiskCacheDevice::GetFileForEntry(nsCacheEntry *    entry,
                                   nsIFile **        result)
{
    NS_ENSURE_ARG_POINTER(result);
    *result = nullptr;

    nsresult             rv;
        
    nsDiskCacheBinding * binding = GetCacheEntryBinding(entry);
    if (!IsValidBinding(binding))
        return NS_ERROR_UNEXPECTED;

    
    if (binding->mRecord.DataLocationInitialized()) {
        if (binding->mRecord.DataFile() != 0)
            return NS_ERROR_NOT_AVAILABLE;  

        NS_ASSERTION(binding->mRecord.DataFileGeneration() == binding->mGeneration, "error generations out of sync");
    } else {
        binding->mRecord.SetDataFileGeneration(binding->mGeneration);
        binding->mRecord.SetDataFileSize(0);    
        if (!binding->mDoomed) {
            
            rv = mCacheMap.UpdateRecord(&binding->mRecord);
            if (NS_FAILED(rv))  return rv;
        }
    }
    
    nsCOMPtr<nsIFile>  file;
    rv = mCacheMap.GetFileForDiskCacheRecord(&binding->mRecord,
                                             nsDiskCache::kData,
                                             false,
                                             getter_AddRefs(file));
    if (NS_FAILED(rv))  return rv;
    
    NS_IF_ADDREF(*result = file);
    return NS_OK;
}







nsresult
nsDiskCacheDevice::OnDataSizeChange(nsCacheEntry * entry, int32_t deltaSize)
{
    CACHE_LOG_DEBUG(("CACHE: disk OnDataSizeChange [%p %d]\n",
        entry, deltaSize));

    
    if (deltaSize < 0)
        return NS_OK;

    nsDiskCacheBinding * binding = GetCacheEntryBinding(entry);
    if (!IsValidBinding(binding))
        return NS_ERROR_UNEXPECTED;

    NS_ASSERTION(binding->mRecord.ValidRecord(), "bad record");

    uint32_t  newSize = entry->DataSize() + deltaSize;
    uint32_t  newSizeK =  ((newSize + 0x3FF) >> 10);

    
    
    if (EntryIsTooBig(newSize)) {
#ifdef DEBUG
        nsresult rv =
#endif
            nsCacheService::DoomEntry(entry);
        NS_ASSERTION(NS_SUCCEEDED(rv),"DoomEntry() failed.");
        return NS_ERROR_ABORT;
    }

    uint32_t  sizeK = ((entry->DataSize() + 0x03FF) >> 10); 

    
    
    if (sizeK > kMaxDataSizeK) sizeK = kMaxDataSizeK;
    if (newSizeK > kMaxDataSizeK) newSizeK = kMaxDataSizeK;

    
    uint32_t  targetCapacity = mCacheCapacity > (newSizeK - sizeK)
                             ? mCacheCapacity - (newSizeK - sizeK)
                             : 0;
    EvictDiskCacheEntries(targetCapacity);
    
    return NS_OK;
}





class EntryInfoVisitor : public nsDiskCacheRecordVisitor
{
public:
    EntryInfoVisitor(nsDiskCacheMap *    cacheMap,
                     nsICacheVisitor *   visitor)
        : mCacheMap(cacheMap)
        , mVisitor(visitor)
    {}
    
    virtual int32_t  VisitRecord(nsDiskCacheRecord *  mapRecord)
    {
        
        
        
        nsDiskCacheEntry * diskEntry = mCacheMap->ReadDiskCacheEntry(mapRecord);
        if (!diskEntry) {
            return kVisitNextRecord;
        }

        
        nsDiskCacheEntryInfo * entryInfo = new nsDiskCacheEntryInfo(DISK_CACHE_DEVICE_ID, diskEntry);
        if (!entryInfo) {
            return kStopVisitingRecords;
        }
        nsCOMPtr<nsICacheEntryInfo> ref(entryInfo);
        
        bool    keepGoing;
        (void)mVisitor->VisitEntry(DISK_CACHE_DEVICE_ID, entryInfo, &keepGoing);
        return keepGoing ? kVisitNextRecord : kStopVisitingRecords;
    }
 
private:
        nsDiskCacheMap *    mCacheMap;
        nsICacheVisitor *   mVisitor;
};


nsresult
nsDiskCacheDevice::Visit(nsICacheVisitor * visitor)
{
    if (!Initialized())  return NS_ERROR_NOT_INITIALIZED;
    nsDiskCacheDeviceInfo* deviceInfo = new nsDiskCacheDeviceInfo(this);
    nsCOMPtr<nsICacheDeviceInfo> ref(deviceInfo);
    
    bool keepGoing;
    nsresult rv = visitor->VisitDevice(DISK_CACHE_DEVICE_ID, deviceInfo, &keepGoing);
    if (NS_FAILED(rv)) return rv;
    
    if (keepGoing) {
        EntryInfoVisitor  infoVisitor(&mCacheMap, visitor);
        return mCacheMap.VisitRecords(&infoVisitor);
    }

    return NS_OK;
}


bool
nsDiskCacheDevice::EntryIsTooBig(int64_t entrySize)
{
    if (mMaxEntrySize == -1) 
        return entrySize > (static_cast<int64_t>(mCacheCapacity) * 1024 / 8);
    else 
        return entrySize > mMaxEntrySize ||
               entrySize > (static_cast<int64_t>(mCacheCapacity) * 1024 / 8);
}

nsresult
nsDiskCacheDevice::EvictEntries(const char * clientID)
{
    CACHE_LOG_DEBUG(("CACHE: disk EvictEntries [%s]\n", clientID));

    if (!Initialized())  return NS_ERROR_NOT_INITIALIZED;
    nsresult  rv;

    if (clientID == nullptr) {
        
        rv = ClearDiskCache();
        if (rv != NS_ERROR_CACHE_IN_USE)
            return rv;
    }

    nsDiskCacheEvictor  evictor(&mCacheMap, &mBindery, 0, clientID);
    rv = mCacheMap.VisitRecords(&evictor);
    
    if (clientID == nullptr)     
        rv = mCacheMap.Trim(); 
    return rv;
}






nsresult
nsDiskCacheDevice::OpenDiskCache()
{
    Telemetry::AutoTimer<Telemetry::NETWORK_DISK_CACHE_OPEN> timer;
    
    bool exists;
    nsresult rv = mCacheDirectory->Exists(&exists);
    if (NS_FAILED(rv))
        return rv;

    if (exists) {
        
        nsDiskCache::CorruptCacheInfo corruptInfo;
        rv = mCacheMap.Open(mCacheDirectory, &corruptInfo, true);

        if (NS_SUCCEEDED(rv)) {
            Telemetry::Accumulate(Telemetry::DISK_CACHE_CORRUPT_DETAILS,
                                  corruptInfo);
        } else if (rv == NS_ERROR_ALREADY_INITIALIZED) {
          NS_WARNING("nsDiskCacheDevice::OpenDiskCache: already open!");
        } else {
            
            Telemetry::Accumulate(Telemetry::DISK_CACHE_CORRUPT_DETAILS,
                                  corruptInfo);
            
            rv = nsDeleteDir::DeleteDir(mCacheDirectory, true, 60000);
            if (NS_FAILED(rv))
                return rv;
            exists = false;
        }
    }

    
    if (!exists) {
        nsCacheService::MarkStartingFresh();
        rv = mCacheDirectory->Create(nsIFile::DIRECTORY_TYPE, 0777);
        CACHE_LOG_PATH(PR_LOG_ALWAYS, "\ncreate cache directory: %s\n", mCacheDirectory);
        CACHE_LOG_ALWAYS(("mCacheDirectory->Create() = %x\n", rv));
        if (NS_FAILED(rv))
            return rv;
    
        
        nsDiskCache::CorruptCacheInfo corruptInfo;
        rv = mCacheMap.Open(mCacheDirectory, &corruptInfo, false);
        if (NS_FAILED(rv))
            return rv;
    }

    return NS_OK;
}


nsresult
nsDiskCacheDevice::ClearDiskCache()
{
    if (mBindery.ActiveBindings())
        return NS_ERROR_CACHE_IN_USE;

    mClearingDiskCache = true;

    nsresult rv = Shutdown_Private(false);  
    if (NS_FAILED(rv))
        return rv;

    mClearingDiskCache = false;

    
    
    rv = nsDeleteDir::DeleteDir(mCacheDirectory, true);
    if (NS_FAILED(rv) && rv != NS_ERROR_FILE_TARGET_DOES_NOT_EXIST)
        return rv;

    return Init();
}


nsresult
nsDiskCacheDevice::EvictDiskCacheEntries(uint32_t  targetCapacity)
{
    CACHE_LOG_DEBUG(("CACHE: disk EvictDiskCacheEntries [%u]\n",
        targetCapacity));

    NS_ASSERTION(targetCapacity > 0, "oops");

    if (mCacheMap.TotalSize() < targetCapacity)
        return NS_OK;

    
    nsDiskCacheEvictor  evictor(&mCacheMap, &mBindery, targetCapacity, nullptr);
    return mCacheMap.EvictRecords(&evictor);
}






void
nsDiskCacheDevice::SetCacheParentDirectory(nsIFile * parentDir)
{
    nsresult rv;
    bool    exists;

    if (Initialized()) {
        NS_ASSERTION(false, "Cannot switch cache directory when initialized");
        return;
    }

    if (!parentDir) {
        mCacheDirectory = nullptr;
        return;
    }

    
    rv = parentDir->Exists(&exists);
    if (NS_SUCCEEDED(rv) && !exists)
        rv = parentDir->Create(nsIFile::DIRECTORY_TYPE, 0700);
    if (NS_FAILED(rv))  return;

    
    nsCOMPtr<nsIFile> directory;
    
    rv = parentDir->Clone(getter_AddRefs(directory));
    if (NS_FAILED(rv))  return;
    rv = directory->AppendNative(NS_LITERAL_CSTRING("Cache"));
    if (NS_FAILED(rv))  return;
    
    mCacheDirectory = do_QueryInterface(directory);
}


void
nsDiskCacheDevice::getCacheDirectory(nsIFile ** result)
{
    *result = mCacheDirectory;
    NS_IF_ADDREF(*result);
}





void
nsDiskCacheDevice::SetCapacity(uint32_t  capacity)
{
    
    mCacheCapacity = capacity;
    if (Initialized()) {
        if (NS_IsMainThread()) {
            
            nsCacheService::DispatchToCacheIOThread(
                new nsEvictDiskCacheEntriesEvent(this));
        } else {
            
            EvictDiskCacheEntries(mCacheCapacity);
        }
    }
    
    mCacheMap.NotifyCapacityChange(capacity);
}


uint32_t nsDiskCacheDevice::getCacheCapacity()
{
    return mCacheCapacity;
}


uint32_t nsDiskCacheDevice::getCacheSize()
{
    return mCacheMap.TotalSize();
}


uint32_t nsDiskCacheDevice::getEntryCount()
{
    return mCacheMap.EntryCount();
}

void
nsDiskCacheDevice::SetMaxEntrySize(int32_t maxSizeInKilobytes)
{
    
    
    if (maxSizeInKilobytes >= 0)
        mMaxEntrySize = maxSizeInKilobytes * 1024;
    else
        mMaxEntrySize = -1;
}

size_t
nsDiskCacheDevice::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf)
{
    size_t usage = aMallocSizeOf(this);

    usage += mCacheMap.SizeOfExcludingThis(aMallocSizeOf);
    usage += mBindery.SizeOfExcludingThis(aMallocSizeOf);

    return usage;
}


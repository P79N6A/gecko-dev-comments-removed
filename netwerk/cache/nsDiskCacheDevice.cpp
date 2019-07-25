








































#include <limits.h>


#if defined(XP_UNIX)
#include <unistd.h>
#elif defined(XP_WIN)
#include <windows.h>
#elif defined(XP_OS2)
#define INCL_DOSERRORS
#include <os2.h>
#else

#endif

#include "prtypes.h"
#include "prthread.h"
#include "prbit.h"

#include "private/pprio.h"

#include "nsDiskCacheDevice.h"
#include "nsDiskCacheEntry.h"
#include "nsDiskCacheMap.h"
#include "nsDiskCacheStreams.h"

#include "nsDiskCache.h"

#include "nsCacheService.h"
#include "nsCache.h"

#include "nsDeleteDir.h"

#include "nsICacheVisitor.h"
#include "nsReadableUtils.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsCRT.h"
#include "nsCOMArray.h"
#include "nsISimpleEnumerator.h"

#include "mozilla/FunctionTimer.h"

static const char DISK_CACHE_DEVICE_ID[] = { "disk" };









class nsDiskCacheEvictor : public nsDiskCacheRecordVisitor
{
public:
    nsDiskCacheEvictor( nsDiskCacheMap *      cacheMap,
                        nsDiskCacheBindery *  cacheBindery,
                        PRUint32              targetSize,
                        const char *          clientID)
        : mCacheMap(cacheMap)
        , mBindery(cacheBindery)
        , mTargetSize(targetSize)
        , mClientID(clientID)
    { 
        mClientIDSize = clientID ? strlen(clientID) : 0;
    }
    
    virtual PRInt32  VisitRecord(nsDiskCacheRecord *  mapRecord);
 
private:
        nsDiskCacheMap *     mCacheMap;
        nsDiskCacheBindery * mBindery;
        PRUint32             mTargetSize;
        const char *         mClientID;
        PRUint32             mClientIDSize;
};


PRInt32
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
        
        
        
        binding->mDoomed = PR_TRUE;         
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

    nsDiskCacheDeviceInfo(nsDiskCacheDevice* device)
        :   mDevice(device)
    {
    }

    virtual ~nsDiskCacheDeviceInfo() {}
    
private:
    nsDiskCacheDevice* mDevice;
};

NS_IMPL_ISUPPORTS1(nsDiskCacheDeviceInfo, nsICacheDeviceInfo)


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
    nsCOMPtr<nsILocalFile> cacheDir;
    nsAutoString           path;
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


NS_IMETHODIMP nsDiskCacheDeviceInfo::GetEntryCount(PRUint32 *aEntryCount)
{
    NS_ENSURE_ARG_POINTER(aEntryCount);
    *aEntryCount = mDevice->getEntryCount();
    return NS_OK;
}


NS_IMETHODIMP nsDiskCacheDeviceInfo::GetTotalSize(PRUint32 *aTotalSize)
{
    NS_ENSURE_ARG_POINTER(aTotalSize);
    
    *aTotalSize = mDevice->getCacheSize() * 1024;
    return NS_OK;
}


NS_IMETHODIMP nsDiskCacheDeviceInfo::GetMaximumSize(PRUint32 *aMaximumSize)
{
    NS_ENSURE_ARG_POINTER(aMaximumSize);
    
    *aMaximumSize = mDevice->getCacheCapacity() * 1024;
    return NS_OK;
}


















static inline void hashmix(PRUint32& a, PRUint32& b, PRUint32& c)
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
  const PRUint8 *k = reinterpret_cast<const PRUint8*>(key);
  PRUint32 a, b, c, len, length;

  length = PL_strlen(key);
  
  len = length;
  a = b = 0x9e3779b9;  
  c = initval;         

  
  while (len >= 12)
  {
    a += k[0] + (PRUint32(k[1])<<8) + (PRUint32(k[2])<<16) + (PRUint32(k[3])<<24);
    b += k[4] + (PRUint32(k[5])<<8) + (PRUint32(k[6])<<16) + (PRUint32(k[7])<<24);
    c += k[8] + (PRUint32(k[9])<<8) + (PRUint32(k[10])<<16) + (PRUint32(k[11])<<24);
    hashmix(a, b, c);
    k += 12; len -= 12;
  }

  
  c += length;
  switch(len) {              
    case 11: c += (PRUint32(k[10])<<24);
    case 10: c += (PRUint32(k[9])<<16);
    case 9 : c += (PRUint32(k[8])<<8);
    
    case 8 : b += (PRUint32(k[7])<<24);
    case 7 : b += (PRUint32(k[6])<<16);
    case 6 : b += (PRUint32(k[5])<<8);
    case 5 : b += k[4];
    case 4 : a += (PRUint32(k[3])<<24);
    case 3 : a += (PRUint32(k[2])<<16);
    case 2 : a += (PRUint32(k[1])<<8);
    case 1 : a += k[0];
    
  }
  hashmix(a, b, c);

  return c;
}

nsresult
nsDiskCache::Truncate(PRFileDesc *  fd, PRUint32  newEOF)
{
    

#if defined(XP_UNIX)
    if (ftruncate(PR_FileDesc2NativeHandle(fd), newEOF) != 0) {
        NS_ERROR("ftruncate failed");
        return NS_ERROR_FAILURE;
    }

#elif defined(XP_WIN)
    PRInt32 cnt = PR_Seek(fd, newEOF, PR_SEEK_SET);
    if (cnt == -1)  return NS_ERROR_FAILURE;
    if (!SetEndOfFile((HANDLE) PR_FileDesc2NativeHandle(fd))) {
        NS_ERROR("SetEndOfFile failed");
        return NS_ERROR_FAILURE;
    }

#elif defined(XP_OS2)
    if (DosSetFileSize((HFILE) PR_FileDesc2NativeHandle(fd), newEOF) != NO_ERROR) {
        NS_ERROR("DosSetFileSize failed");
        return NS_ERROR_FAILURE;
    }
#else
    
#endif
    return NS_OK;
}






nsDiskCacheDevice::nsDiskCacheDevice()
    : mCacheCapacity(0)
    , mInitialized(PR_FALSE)
{
}

nsDiskCacheDevice::~nsDiskCacheDevice()
{
    Shutdown();
}





nsresult
nsDiskCacheDevice::Init()
{
    NS_TIME_FUNCTION;

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
        (void) mCacheMap.Close(PR_FALSE);
        return rv;
    }

    mInitialized = PR_TRUE;
    return NS_OK;
}





nsresult
nsDiskCacheDevice::Shutdown()
{
    nsCacheService::AssertOwnsLock();

    nsresult rv = Shutdown_Private(PR_TRUE);
    if (NS_FAILED(rv))
        return rv;

    if (mCacheDirectory) {
        
        nsCOMPtr<nsIFile> trashDir;
        GetTrashDir(mCacheDirectory, &trashDir);
        if (trashDir) {
            PRBool exists;
            if (NS_SUCCEEDED(trashDir->Exists(&exists)) && exists)
                DeleteDir(trashDir, PR_FALSE, PR_TRUE);
        }
    }

    return NS_OK;
}


nsresult
nsDiskCacheDevice::Shutdown_Private(PRBool  flush)
{
    CACHE_LOG_DEBUG(("CACHE: disk Shutdown_Private [%u]\n", flush));

    if (Initialized()) {
        
        EvictDiskCacheEntries(mCacheCapacity);

        
        
        
        (void) nsCacheService::SyncWithCacheIOThread();

        
        (void) mCacheMap.Close(flush);

        mBindery.Reset();

        mInitialized = PR_FALSE;
    }

    return NS_OK;
}


const char *
nsDiskCacheDevice::GetDeviceID()
{
    return DISK_CACHE_DEVICE_ID;
}











nsCacheEntry *
nsDiskCacheDevice::FindEntry(nsCString * key, PRBool *collision)
{
    if (!Initialized())  return nsnull;  
    nsDiskCacheRecord       record;
    nsDiskCacheBinding *    binding = nsnull;
    PLDHashNumber           hashNumber = nsDiskCache::Hash(key->get());

    *collision = PR_FALSE;

    binding = mBindery.FindActiveBinding(hashNumber);
    if (binding && !binding->mCacheEntry->Key()->Equals(*key)) {
        *collision = PR_TRUE;
        return nsnull;
    }
    binding = nsnull;

    
    nsresult rv = mCacheMap.FindRecord(hashNumber, &record);
    if (NS_FAILED(rv))  return nsnull;  
    
    nsDiskCacheEntry * diskEntry = mCacheMap.ReadDiskCacheEntry(&record);
    if (!diskEntry) return nsnull;
    
    
    if (!key->Equals(diskEntry->Key())) {
        *collision = PR_TRUE;
        return nsnull;
    }
    
    nsCacheEntry * entry = diskEntry->CreateCacheEntry(this);
    if (!entry)  return nsnull;
    
    binding = mBindery.CreateBinding(entry, &record);
    if (!binding) {
        delete entry;
        return nsnull;
    }
    
    return entry;
}





nsresult
nsDiskCacheDevice::DeactivateEntry(nsCacheEntry * entry)
{
    nsresult              rv = NS_OK;
    nsDiskCacheBinding * binding = GetCacheEntryBinding(entry);
    NS_ASSERTION(binding, "DeactivateEntry: binding == nsnull");
    if (!binding)  return NS_ERROR_UNEXPECTED;

    CACHE_LOG_DEBUG(("CACHE: disk DeactivateEntry [%p %x]\n",
        entry, binding->mRecord.HashNumber()));

    if (entry->IsDoomed()) {
        
        rv = mCacheMap.DeleteStorage(&binding->mRecord);

    } else {
        
        rv = mCacheMap.WriteDiskCacheEntry(binding);
        if (NS_FAILED(rv)) {
            
            (void) mCacheMap.DeleteStorage(&binding->mRecord);
            (void) mCacheMap.DeleteRecord(&binding->mRecord);
            binding->mDoomed = PR_TRUE; 
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
    nsresult rv = NS_OK;
    nsDiskCacheRecord record, oldRecord;
    nsDiskCacheBinding *binding;
    PLDHashNumber hashNumber = nsDiskCache::Hash(entry->Key()->get());

    
    
    
    
    binding = mBindery.FindActiveBinding(hashNumber);
    if (binding) {
        NS_ASSERTION(!binding->mCacheEntry->Key()->Equals(*entry->Key()),
                     "BindEntry called for already bound entry!");
        nsCacheService::DoomEntry(binding->mCacheEntry);
        binding = nsnull;
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
        
        PRUint32    oldHashNumber = oldRecord.HashNumber();
        if (oldHashNumber) {
            
            nsDiskCacheBinding * oldBinding = mBindery.FindActiveBinding(oldHashNumber);
            if (oldBinding) {
                

                if (!oldBinding->mCacheEntry->IsDoomed()) {
                
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
    NS_ASSERTION(binding, "DoomEntry: binding == nsnull");
    if (!binding)  return;

    if (!binding->mDoomed) {
        
#ifdef DEBUG
        nsresult rv =
#endif
            mCacheMap.DeleteRecord(&binding->mRecord);
        NS_ASSERTION(NS_SUCCEEDED(rv),"DeleteRecord failed.");
        binding->mDoomed = PR_TRUE; 
    }
}





nsresult
nsDiskCacheDevice::OpenInputStreamForEntry(nsCacheEntry *      entry,
                                           nsCacheAccessMode   mode, 
                                           PRUint32            offset,
                                           nsIInputStream **   result)
{
    CACHE_LOG_DEBUG(("CACHE: disk OpenInputStreamForEntry [%p %x %u]\n",
        entry, mode, offset));

    NS_ENSURE_ARG_POINTER(entry);
    NS_ENSURE_ARG_POINTER(result);

    nsresult             rv;
    nsDiskCacheBinding * binding = GetCacheEntryBinding(entry);
    NS_ENSURE_TRUE(binding, NS_ERROR_UNEXPECTED);
    
    NS_ASSERTION(binding->mCacheEntry == entry, "binding & entry don't point to each other");

    rv = binding->EnsureStreamIO();
    if (NS_FAILED(rv)) return rv;

    return binding->mStreamIO->GetInputStream(offset, result);
}





nsresult
nsDiskCacheDevice::OpenOutputStreamForEntry(nsCacheEntry *      entry,
                                            nsCacheAccessMode   mode, 
                                            PRUint32            offset,
                                            nsIOutputStream **  result)
{
    CACHE_LOG_DEBUG(("CACHE: disk OpenOutputStreamForEntry [%p %x %u]\n",
        entry, mode, offset));
 
    NS_ENSURE_ARG_POINTER(entry);
    NS_ENSURE_ARG_POINTER(result);

    nsresult             rv;
    nsDiskCacheBinding * binding = GetCacheEntryBinding(entry);
    NS_ENSURE_TRUE(binding, NS_ERROR_UNEXPECTED);
    
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
    *result = nsnull;

    nsresult             rv;
        
    nsDiskCacheBinding * binding = GetCacheEntryBinding(entry);
    if (!binding) {
        NS_WARNING("GetFileForEntry: binding == nsnull");
        return NS_ERROR_UNEXPECTED;
    }
    
    
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
                                             PR_FALSE,
                                             getter_AddRefs(file));
    if (NS_FAILED(rv))  return rv;
    
    NS_IF_ADDREF(*result = file);
    return NS_OK;
}







nsresult
nsDiskCacheDevice::OnDataSizeChange(nsCacheEntry * entry, PRInt32 deltaSize)
{
    CACHE_LOG_DEBUG(("CACHE: disk OnDataSizeChange [%p %d]\n",
        entry, deltaSize));

    
    if (deltaSize < 0)
        return NS_OK;

    nsDiskCacheBinding * binding = GetCacheEntryBinding(entry);
    NS_ASSERTION(binding, "OnDataSizeChange: binding == nsnull");
    if (!binding)  return NS_ERROR_UNEXPECTED;

    NS_ASSERTION(binding->mRecord.ValidRecord(), "bad record");

    PRUint32  newSize = entry->DataSize() + deltaSize;
    PRUint32  newSizeK =  ((newSize + 0x3FF) >> 10);

    
    
    
    if (EntryIsTooBig(newSize) &&
        entry->StoragePolicy() != nsICache::STORE_ON_DISK_AS_FILE) {
#ifdef DEBUG
        nsresult rv =
#endif
            nsCacheService::DoomEntry(entry);
        NS_ASSERTION(NS_SUCCEEDED(rv),"DoomEntry() failed.");
        return NS_ERROR_ABORT;
    }

    PRUint32  sizeK = ((entry->DataSize() + 0x03FF) >> 10); 

    NS_ASSERTION(sizeK <= USHRT_MAX, "data size out of range");
    NS_ASSERTION(newSizeK <= USHRT_MAX, "data size out of range");

    
    PRUint32  targetCapacity = mCacheCapacity > (newSizeK - sizeK)
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
    
    virtual PRInt32  VisitRecord(nsDiskCacheRecord *  mapRecord)
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
        
        PRBool  keepGoing;
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
    
    PRBool keepGoing;
    nsresult rv = visitor->VisitDevice(DISK_CACHE_DEVICE_ID, deviceInfo, &keepGoing);
    if (NS_FAILED(rv)) return rv;
    
    if (keepGoing) {
        EntryInfoVisitor  infoVisitor(&mCacheMap, visitor);
        return mCacheMap.VisitRecords(&infoVisitor);
    }

    return NS_OK;
}


bool
nsDiskCacheDevice::EntryIsTooBig(PRInt64 entrySize)
{
    return entrySize > kMaxDataFileSize
           || entrySize > (static_cast<PRInt64>(mCacheCapacity) * 1024 / 8);
}

nsresult
nsDiskCacheDevice::EvictEntries(const char * clientID)
{
    CACHE_LOG_DEBUG(("CACHE: disk EvictEntries [%s]\n", clientID));

    if (!Initialized())  return NS_ERROR_NOT_INITIALIZED;
    nsresult  rv;

    if (clientID == nsnull) {
        
        rv = ClearDiskCache();
        if (rv != NS_ERROR_CACHE_IN_USE)
            return rv;
    }

    nsDiskCacheEvictor  evictor(&mCacheMap, &mBindery, 0, clientID);
    rv = mCacheMap.VisitRecords(&evictor);
    
    if (clientID == nsnull)     
        rv = mCacheMap.Trim(); 
    return rv;
}






nsresult
nsDiskCacheDevice::OpenDiskCache()
{
    
    PRBool exists;
    nsresult rv = mCacheDirectory->Exists(&exists);
    if (NS_FAILED(rv))
        return rv;

    PRBool trashing = PR_FALSE;
    if (exists) {
        
        rv = mCacheMap.Open(mCacheDirectory);        
        
        if (rv == NS_ERROR_FILE_CORRUPTED) {
            rv = DeleteDir(mCacheDirectory, PR_TRUE, PR_FALSE);
            if (NS_FAILED(rv))
                return rv;
            exists = PR_FALSE;
            trashing = PR_TRUE;
        }
        else if (NS_FAILED(rv))
            return rv;
    }

    
    if (!exists) {
        rv = mCacheDirectory->Create(nsIFile::DIRECTORY_TYPE, 0777);
        CACHE_LOG_PATH(PR_LOG_ALWAYS, "\ncreate cache directory: %s\n", mCacheDirectory);
        CACHE_LOG_ALWAYS(("mCacheDirectory->Create() = %x\n", rv));
        if (NS_FAILED(rv))
            return rv;
    
        
        rv = mCacheMap.Open(mCacheDirectory);
        if (NS_FAILED(rv))
            return rv;
    }

    if (!trashing) {
        
        nsCOMPtr<nsIFile> trashDir;
        GetTrashDir(mCacheDirectory, &trashDir);
        if (trashDir) {
            PRBool exists;
            if (NS_SUCCEEDED(trashDir->Exists(&exists)) && exists)
                DeleteDir(trashDir, PR_FALSE, PR_FALSE);
        }
    }

    return NS_OK;
}


nsresult
nsDiskCacheDevice::ClearDiskCache()
{
    if (mBindery.ActiveBindings())
        return NS_ERROR_CACHE_IN_USE;

    nsresult rv = Shutdown_Private(PR_FALSE);  
    if (NS_FAILED(rv))
        return rv;

    
    
    rv = DeleteDir(mCacheDirectory, PR_TRUE, PR_FALSE);
    if (NS_FAILED(rv) && rv != NS_ERROR_FILE_TARGET_DOES_NOT_EXIST)
        return rv;

    return Init();
}


nsresult
nsDiskCacheDevice::EvictDiskCacheEntries(PRUint32  targetCapacity)
{
    CACHE_LOG_DEBUG(("CACHE: disk EvictDiskCacheEntries [%u]\n",
        targetCapacity));

    NS_ASSERTION(targetCapacity > 0, "oops");

    if (mCacheMap.TotalSize() < targetCapacity)
        return NS_OK;

    
    nsDiskCacheEvictor  evictor(&mCacheMap, &mBindery, targetCapacity, nsnull);
    return mCacheMap.EvictRecords(&evictor);
}






void
nsDiskCacheDevice::SetCacheParentDirectory(nsILocalFile * parentDir)
{
    nsresult rv;
    PRBool  exists;

    if (Initialized()) {
        NS_ASSERTION(PR_FALSE, "Cannot switch cache directory when initialized");
        return;
    }

    if (!parentDir) {
        mCacheDirectory = nsnull;
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
nsDiskCacheDevice::getCacheDirectory(nsILocalFile ** result)
{
    *result = mCacheDirectory;
    NS_IF_ADDREF(*result);
}





void
nsDiskCacheDevice::SetCapacity(PRUint32  capacity)
{
    
    mCacheCapacity = capacity;
    if (Initialized()) {
        
        EvictDiskCacheEntries(mCacheCapacity);
    }
    
    mCacheMap.NotifyCapacityChange(capacity);
}


PRUint32 nsDiskCacheDevice::getCacheCapacity()
{
    return mCacheCapacity;
}


PRUint32 nsDiskCacheDevice::getCacheSize()
{
    return mCacheMap.TotalSize();
}


PRUint32 nsDiskCacheDevice::getEntryCount()
{
    return mCacheMap.EntryCount();
}







#include "nsCache.h"
#include "nsDiskCacheMap.h"
#include "nsDiskCacheBinding.h"
#include "nsDiskCacheEntry.h"
#include "nsDiskCacheDevice.h"
#include "nsCacheService.h"

#include <string.h>
#include "nsPrintfCString.h"

#include "nsISerializable.h"
#include "nsSerializationHelper.h"

#include "mozilla/Telemetry.h"
#include "mozilla/VisualEventTracer.h"
#include <algorithm>

using namespace mozilla;









nsresult
nsDiskCacheMap::Open(nsIFile *  cacheDirectory,
                     nsDiskCache::CorruptCacheInfo *  corruptInfo,
                     bool reportCacheCleanTelemetryData)
{
    NS_ENSURE_ARG_POINTER(corruptInfo);

    
    *corruptInfo = nsDiskCache::kUnexpectedError;
    NS_ENSURE_ARG_POINTER(cacheDirectory);
    if (mMapFD)  return NS_ERROR_ALREADY_INITIALIZED;

    mCacheDirectory = cacheDirectory;   
    
    
    nsresult rv;
    nsCOMPtr<nsIFile> file;
    rv = cacheDirectory->Clone(getter_AddRefs(file));
    rv = file->AppendNative(NS_LITERAL_CSTRING("_CACHE_MAP_"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = file->OpenNSPRFileDesc(PR_RDWR | PR_CREATE_FILE, 00600, &mMapFD);
    if (NS_FAILED(rv)) {
        *corruptInfo = nsDiskCache::kOpenCacheMapError;
        NS_WARNING("Could not open cache map file");
        return NS_ERROR_FILE_CORRUPTED;
    }

    bool cacheFilesExist = CacheFilesExist();
    rv = NS_ERROR_FILE_CORRUPTED;  
    uint32_t mapSize = PR_Available(mMapFD);    

    if (NS_FAILED(InitCacheClean(cacheDirectory,
                                 corruptInfo,
                                 reportCacheCleanTelemetryData))) {
        
        goto error_exit;
    }

    
    if (mapSize == 0) {  

        
        if (cacheFilesExist) {
            *corruptInfo = nsDiskCache::kBlockFilesShouldNotExist;
            goto error_exit; 
        }

        if (NS_FAILED(CreateCacheSubDirectories())) {
            *corruptInfo = nsDiskCache::kCreateCacheSubdirectories;
            goto error_exit;
        }

        
        memset(&mHeader, 0, sizeof(nsDiskCacheHeader));
        mHeader.mVersion = nsDiskCache::kCurrentVersion;
        mHeader.mRecordCount = kMinRecordCount;
        mRecordArray = (nsDiskCacheRecord *)
            PR_CALLOC(mHeader.mRecordCount * sizeof(nsDiskCacheRecord));
        if (!mRecordArray) {
            *corruptInfo = nsDiskCache::kOutOfMemory;
            rv = NS_ERROR_OUT_OF_MEMORY;
            goto error_exit;
        }
    } else if (mapSize >= sizeof(nsDiskCacheHeader)) {  
        
        
        if (!cacheFilesExist) {
            *corruptInfo = nsDiskCache::kBlockFilesShouldExist;
            goto error_exit;
        }

        CACHE_LOG_DEBUG(("CACHE: nsDiskCacheMap::Open [this=%p] reading map", this));

        
        uint32_t bytesRead = PR_Read(mMapFD, &mHeader, sizeof(nsDiskCacheHeader));
        if (sizeof(nsDiskCacheHeader) != bytesRead) {
            *corruptInfo = nsDiskCache::kHeaderSizeNotRead;
            goto error_exit;
        }
        mHeader.Unswap();

        if (mHeader.mIsDirty) {
            *corruptInfo = nsDiskCache::kHeaderIsDirty;
            goto error_exit;
        }
        
        if (mHeader.mVersion != nsDiskCache::kCurrentVersion) {
            *corruptInfo = nsDiskCache::kVersionMismatch;
            goto error_exit;
        }

        uint32_t recordArraySize =
                mHeader.mRecordCount * sizeof(nsDiskCacheRecord);
        if (mapSize < recordArraySize + sizeof(nsDiskCacheHeader)) {
            *corruptInfo = nsDiskCache::kRecordsIncomplete;
            goto error_exit;
        }

        
        mRecordArray = (nsDiskCacheRecord *) PR_MALLOC(recordArraySize);
        if (!mRecordArray) {
            *corruptInfo = nsDiskCache::kOutOfMemory;
            rv = NS_ERROR_OUT_OF_MEMORY;
            goto error_exit;
        }

        
        bytesRead = PR_Read(mMapFD, mRecordArray, recordArraySize);
        if (bytesRead < recordArraySize) {
            *corruptInfo = nsDiskCache::kNotEnoughToRead;
            goto error_exit;
        }

        
        int32_t total = 0;
        for (int32_t i = 0; i < mHeader.mRecordCount; ++i) {
            if (mRecordArray[i].HashNumber()) {
#if defined(IS_LITTLE_ENDIAN)
                mRecordArray[i].Unswap();
#endif
                total ++;
            }
        }
        
        
        if (total != mHeader.mEntryCount) {
            *corruptInfo = nsDiskCache::kEntryCountIncorrect;
            goto error_exit;
        }

    } else {
        *corruptInfo = nsDiskCache::kHeaderIncomplete;
        goto error_exit;
    }

    rv = OpenBlockFiles(corruptInfo);
    if (NS_FAILED(rv)) {
        
        goto error_exit;
    }

    
    mHeader.mIsDirty    = true;
    rv = FlushHeader();
    if (NS_FAILED(rv)) {
        *corruptInfo = nsDiskCache::kFlushHeaderError;
        goto error_exit;
    }
    
    Telemetry::Accumulate(Telemetry::HTTP_DISK_CACHE_OVERHEAD,
                          (uint32_t)SizeOfExcludingThis(moz_malloc_size_of));

    *corruptInfo = nsDiskCache::kNotCorrupt;
    return NS_OK;
    
error_exit:
    (void) Close(false);
       
    return rv;
}


nsresult
nsDiskCacheMap::Close(bool flush)
{
    nsCacheService::AssertOwnsLock();
    nsresult  rv = NS_OK;

    
    
    if (mCleanCacheTimer) {
        mCleanCacheTimer->Cancel();
    }

    
    if (mMapFD) {
        
        rv = CloseBlockFiles(flush);
        if (NS_SUCCEEDED(rv) && flush && mRecordArray) {
            
            rv = FlushRecords(false);   
            if (NS_SUCCEEDED(rv)) {
                
                mHeader.mIsDirty = false;
                rv = FlushHeader();
            }
        }
        if ((PR_Close(mMapFD) != PR_SUCCESS) && (NS_SUCCEEDED(rv)))
            rv = NS_ERROR_UNEXPECTED;

        mMapFD = nullptr;
    }

    if (mCleanFD) {
        PR_Close(mCleanFD);
        mCleanFD = nullptr;
    }

    PR_FREEIF(mRecordArray);
    PR_FREEIF(mBuffer);
    mBufferSize = 0;
    return rv;
}


nsresult
nsDiskCacheMap::Trim()
{
    nsresult rv, rv2 = NS_OK;
    for (int i=0; i < kNumBlockFiles; ++i) {
        rv = mBlockFile[i].Trim();
        if (NS_FAILED(rv))  rv2 = rv;   
    }
    
    rv = ShrinkRecords();
    if (NS_FAILED(rv))  rv2 = rv;   
    return rv2;
}


nsresult
nsDiskCacheMap::FlushHeader()
{
    if (!mMapFD)  return NS_ERROR_NOT_AVAILABLE;
    
    
    int32_t filePos = PR_Seek(mMapFD, 0, PR_SEEK_SET);
    if (filePos != 0)  return NS_ERROR_UNEXPECTED;
    
    
    mHeader.Swap();
    int32_t bytesWritten = PR_Write(mMapFD, &mHeader, sizeof(nsDiskCacheHeader));
    mHeader.Unswap();
    if (sizeof(nsDiskCacheHeader) != bytesWritten) {
        return NS_ERROR_UNEXPECTED;
    }

    PRStatus err = PR_Sync(mMapFD);
    if (err != PR_SUCCESS) return NS_ERROR_UNEXPECTED;

    
    if (!mHeader.mIsDirty) {
        RevalidateCache();
    }

    return NS_OK;
}


nsresult
nsDiskCacheMap::FlushRecords(bool unswap)
{
    if (!mMapFD)  return NS_ERROR_NOT_AVAILABLE;
    
    
    int32_t filePos = PR_Seek(mMapFD, sizeof(nsDiskCacheHeader), PR_SEEK_SET);
    if (filePos != sizeof(nsDiskCacheHeader))
        return NS_ERROR_UNEXPECTED;
    
#if defined(IS_LITTLE_ENDIAN)
    
    for (int32_t i = 0; i < mHeader.mRecordCount; ++i) {
        if (mRecordArray[i].HashNumber())   
            mRecordArray[i].Swap();
    }
#endif
    
    int32_t recordArraySize = sizeof(nsDiskCacheRecord) * mHeader.mRecordCount;

    int32_t bytesWritten = PR_Write(mMapFD, mRecordArray, recordArraySize);
    if (bytesWritten != recordArraySize)
        return NS_ERROR_UNEXPECTED;

#if defined(IS_LITTLE_ENDIAN)
    if (unswap) {
        
        for (int32_t i = 0; i < mHeader.mRecordCount; ++i) {
            if (mRecordArray[i].HashNumber())   
                mRecordArray[i].Unswap();
        }
    }
#endif
    
    return NS_OK;
}






uint32_t
nsDiskCacheMap::GetBucketRank(uint32_t bucketIndex, uint32_t targetRank)
{
    nsDiskCacheRecord * records = GetFirstRecordInBucket(bucketIndex);
    uint32_t            rank = 0;

    for (int i = mHeader.mBucketUsage[bucketIndex]-1; i >= 0; i--) {          
        if ((rank < records[i].EvictionRank()) &&
            ((targetRank == 0) || (records[i].EvictionRank() < targetRank)))
                rank = records[i].EvictionRank();
    }
    return rank;
}

nsresult
nsDiskCacheMap::GrowRecords()
{
    if (mHeader.mRecordCount >= mMaxRecordCount)
        return NS_OK;
    CACHE_LOG_DEBUG(("CACHE: GrowRecords\n"));

    
    int32_t newCount = mHeader.mRecordCount << 1;
    if (newCount > mMaxRecordCount)
        newCount = mMaxRecordCount;
    nsDiskCacheRecord *newArray = (nsDiskCacheRecord *)
            PR_REALLOC(mRecordArray, newCount * sizeof(nsDiskCacheRecord));
    if (!newArray)
        return NS_ERROR_OUT_OF_MEMORY;

    
    uint32_t oldRecordsPerBucket = GetRecordsPerBucket();
    uint32_t newRecordsPerBucket = newCount / kBuckets;
    
    for (int bucketIndex = kBuckets - 1; bucketIndex >= 0; --bucketIndex) {
        
        nsDiskCacheRecord *newRecords = newArray + bucketIndex * newRecordsPerBucket;
        const uint32_t count = mHeader.mBucketUsage[bucketIndex];
        memmove(newRecords,
                newArray + bucketIndex * oldRecordsPerBucket,
                count * sizeof(nsDiskCacheRecord));
        
        memset(newRecords + count, 0,
               (newRecordsPerBucket - count) * sizeof(nsDiskCacheRecord));
    }

    
    mRecordArray = newArray;
    mHeader.mRecordCount = newCount;

    InvalidateCache();

    return NS_OK;
}

nsresult
nsDiskCacheMap::ShrinkRecords()
{
    if (mHeader.mRecordCount <= kMinRecordCount)
        return NS_OK;
    CACHE_LOG_DEBUG(("CACHE: ShrinkRecords\n"));

    
    
    uint32_t maxUsage = 0, bucketIndex;
    for (bucketIndex = 0; bucketIndex < kBuckets; ++bucketIndex) {
        if (maxUsage < mHeader.mBucketUsage[bucketIndex])
            maxUsage = mHeader.mBucketUsage[bucketIndex];
    }
    
    uint32_t oldRecordsPerBucket = GetRecordsPerBucket();
    uint32_t newRecordsPerBucket = oldRecordsPerBucket;
    while (maxUsage < (newRecordsPerBucket >> 1))
        newRecordsPerBucket >>= 1;
    if (newRecordsPerBucket < (kMinRecordCount / kBuckets))
        newRecordsPerBucket = (kMinRecordCount / kBuckets);
    NS_ASSERTION(newRecordsPerBucket <= oldRecordsPerBucket,
                 "ShrinkRecords() can't grow records!");
    if (newRecordsPerBucket == oldRecordsPerBucket)
        return NS_OK;
    
    for (bucketIndex = 1; bucketIndex < kBuckets; ++bucketIndex) {
        
        memmove(mRecordArray + bucketIndex * newRecordsPerBucket,
                mRecordArray + bucketIndex * oldRecordsPerBucket,
                newRecordsPerBucket * sizeof(nsDiskCacheRecord));
    }

    
    uint32_t newCount = newRecordsPerBucket * kBuckets;
    nsDiskCacheRecord* newArray = (nsDiskCacheRecord *)
            PR_REALLOC(mRecordArray, newCount * sizeof(nsDiskCacheRecord));
    if (!newArray)
        return NS_ERROR_OUT_OF_MEMORY;

    
    mRecordArray = newArray;
    mHeader.mRecordCount = newCount;

    InvalidateCache();

    return NS_OK;
}

nsresult
nsDiskCacheMap::AddRecord( nsDiskCacheRecord *  mapRecord,
                           nsDiskCacheRecord *  oldRecord)
{
    CACHE_LOG_DEBUG(("CACHE: AddRecord [%x]\n", mapRecord->HashNumber()));

    const uint32_t      hashNumber = mapRecord->HashNumber();
    const uint32_t      bucketIndex = GetBucketIndex(hashNumber);
    const uint32_t      count = mHeader.mBucketUsage[bucketIndex];

    oldRecord->SetHashNumber(0);  

    if (count == GetRecordsPerBucket()) {
        
        GrowRecords();
    }
    
    nsDiskCacheRecord * records = GetFirstRecordInBucket(bucketIndex);
    if (count < GetRecordsPerBucket()) {
        
        records[count] = *mapRecord;
        mHeader.mEntryCount++;
        mHeader.mBucketUsage[bucketIndex]++;           
        if (mHeader.mEvictionRank[bucketIndex] < mapRecord->EvictionRank())
            mHeader.mEvictionRank[bucketIndex] = mapRecord->EvictionRank();
        InvalidateCache();
    } else {
        
        nsDiskCacheRecord * mostEvictable = &records[0];
        for (int i = count-1; i > 0; i--) {
            if (records[i].EvictionRank() > mostEvictable->EvictionRank())
                mostEvictable = &records[i];
        }
        *oldRecord     = *mostEvictable;    
                                            
        *mostEvictable = *mapRecord;        
        
        if (mHeader.mEvictionRank[bucketIndex] < mapRecord->EvictionRank())
            mHeader.mEvictionRank[bucketIndex] = mapRecord->EvictionRank();
        if (oldRecord->EvictionRank() >= mHeader.mEvictionRank[bucketIndex]) 
            mHeader.mEvictionRank[bucketIndex] = GetBucketRank(bucketIndex, 0);
        InvalidateCache();
    }

    NS_ASSERTION(mHeader.mEvictionRank[bucketIndex] == GetBucketRank(bucketIndex, 0),
                 "eviction rank out of sync");
    return NS_OK;
}


nsresult
nsDiskCacheMap::UpdateRecord( nsDiskCacheRecord *  mapRecord)
{
    CACHE_LOG_DEBUG(("CACHE: UpdateRecord [%x]\n", mapRecord->HashNumber()));

    const uint32_t      hashNumber = mapRecord->HashNumber();
    const uint32_t      bucketIndex = GetBucketIndex(hashNumber);
    nsDiskCacheRecord * records = GetFirstRecordInBucket(bucketIndex);

    for (int i = mHeader.mBucketUsage[bucketIndex]-1; i >= 0; i--) {          
        if (records[i].HashNumber() == hashNumber) {
            const uint32_t oldRank = records[i].EvictionRank();

            
            records[i] = *mapRecord;

            
            if (mHeader.mEvictionRank[bucketIndex] < mapRecord->EvictionRank())
                mHeader.mEvictionRank[bucketIndex] = mapRecord->EvictionRank();
            else if (mHeader.mEvictionRank[bucketIndex] == oldRank)
                mHeader.mEvictionRank[bucketIndex] = GetBucketRank(bucketIndex, 0);

            InvalidateCache();

NS_ASSERTION(mHeader.mEvictionRank[bucketIndex] == GetBucketRank(bucketIndex, 0),
             "eviction rank out of sync");
            return NS_OK;
        }
    }
    NS_NOTREACHED("record not found");
    return NS_ERROR_UNEXPECTED;
}


nsresult
nsDiskCacheMap::FindRecord( uint32_t  hashNumber, nsDiskCacheRecord *  result)
{
    const uint32_t      bucketIndex = GetBucketIndex(hashNumber);
    nsDiskCacheRecord * records = GetFirstRecordInBucket(bucketIndex);

    for (int i = mHeader.mBucketUsage[bucketIndex]-1; i >= 0; i--) {          
        if (records[i].HashNumber() == hashNumber) {
            *result = records[i];    
            NS_ASSERTION(result->ValidRecord(), "bad cache map record");
            return NS_OK;
        }
    }
    return NS_ERROR_CACHE_KEY_NOT_FOUND;
}


nsresult
nsDiskCacheMap::DeleteRecord( nsDiskCacheRecord *  mapRecord)
{
    CACHE_LOG_DEBUG(("CACHE: DeleteRecord [%x]\n", mapRecord->HashNumber()));

    const uint32_t      hashNumber = mapRecord->HashNumber();
    const uint32_t      bucketIndex = GetBucketIndex(hashNumber);
    nsDiskCacheRecord * records = GetFirstRecordInBucket(bucketIndex);
    uint32_t            last = mHeader.mBucketUsage[bucketIndex]-1;

    for (int i = last; i >= 0; i--) {          
        if (records[i].HashNumber() == hashNumber) {
            
            uint32_t  evictionRank = records[i].EvictionRank();
            NS_ASSERTION(evictionRank == mapRecord->EvictionRank(),
                         "evictionRank out of sync");
            
            records[i] = records[last];
            records[last].SetHashNumber(0); 
            mHeader.mBucketUsage[bucketIndex] = last;
            mHeader.mEntryCount--;

            
            uint32_t  bucketIndex = GetBucketIndex(mapRecord->HashNumber());
            if (mHeader.mEvictionRank[bucketIndex] <= evictionRank) {
                mHeader.mEvictionRank[bucketIndex] = GetBucketRank(bucketIndex, 0);
            }

            InvalidateCache();

            NS_ASSERTION(mHeader.mEvictionRank[bucketIndex] ==
                         GetBucketRank(bucketIndex, 0), "eviction rank out of sync");
            return NS_OK;
        }
    }
    return NS_ERROR_UNEXPECTED;
}


int32_t
nsDiskCacheMap::VisitEachRecord(uint32_t                    bucketIndex,
                                nsDiskCacheRecordVisitor *  visitor,
                                uint32_t                    evictionRank)
{
    int32_t             rv = kVisitNextRecord;
    uint32_t            count = mHeader.mBucketUsage[bucketIndex];
    nsDiskCacheRecord * records = GetFirstRecordInBucket(bucketIndex);

    
    for (int i = count-1; i >= 0; i--) {
        if (evictionRank > records[i].EvictionRank()) continue;

        rv = visitor->VisitRecord(&records[i]);
        if (rv == kStopVisitingRecords) 
            break;    
        
        if (rv == kDeleteRecordAndContinue) {
            --count;
            records[i] = records[count];
            records[count].SetHashNumber(0);
            InvalidateCache();
        }
    }

    if (mHeader.mBucketUsage[bucketIndex] - count != 0) {
        mHeader.mEntryCount -= mHeader.mBucketUsage[bucketIndex] - count;
        mHeader.mBucketUsage[bucketIndex] = count;
        
        mHeader.mEvictionRank[bucketIndex] = GetBucketRank(bucketIndex, 0);
    }
    NS_ASSERTION(mHeader.mEvictionRank[bucketIndex] ==
                 GetBucketRank(bucketIndex, 0), "eviction rank out of sync");

    return rv;
}







nsresult
nsDiskCacheMap::VisitRecords( nsDiskCacheRecordVisitor *  visitor)
{
    for (int bucketIndex = 0; bucketIndex < kBuckets; ++bucketIndex) {
        if (VisitEachRecord(bucketIndex, visitor, 0) == kStopVisitingRecords)
            break;
    }   
    return NS_OK;
}







nsresult
nsDiskCacheMap::EvictRecords( nsDiskCacheRecordVisitor * visitor)
{
    uint32_t  tempRank[kBuckets];
    int       bucketIndex = 0;
    
    
    for (bucketIndex = 0; bucketIndex < kBuckets; ++bucketIndex)
        tempRank[bucketIndex] = mHeader.mEvictionRank[bucketIndex];

    
    
    
    int32_t entryCount = mHeader.mEntryCount;
    for (int n = 0; n < entryCount; ++n) {
    
        
        uint32_t    rank  = 0;
        for (int i = 0; i < kBuckets; ++i) {
            if (rank < tempRank[i]) {
                rank = tempRank[i];
                bucketIndex = i;
            }
        }
        
        if (rank == 0) break;  

        
        if (VisitEachRecord(bucketIndex, visitor, rank) == kStopVisitingRecords)
            break;

        
        tempRank[bucketIndex] = GetBucketRank(bucketIndex, rank);
    }
    return NS_OK;
}



nsresult
nsDiskCacheMap::OpenBlockFiles(nsDiskCache::CorruptCacheInfo *  corruptInfo)
{
    NS_ENSURE_ARG_POINTER(corruptInfo);

    
    nsCOMPtr<nsIFile> blockFile;
    nsresult rv = NS_OK;
    *corruptInfo = nsDiskCache::kUnexpectedError;
    
    for (int i = 0; i < kNumBlockFiles; ++i) {
        rv = GetBlockFileForIndex(i, getter_AddRefs(blockFile));
        if (NS_FAILED(rv)) {
            *corruptInfo = nsDiskCache::kCouldNotGetBlockFileForIndex;
            break;
        }
    
        uint32_t blockSize = GetBlockSizeForIndex(i+1); 
        uint32_t bitMapSize = GetBitMapSizeForIndex(i+1);
        rv = mBlockFile[i].Open(blockFile, blockSize, bitMapSize, corruptInfo);
        if (NS_FAILED(rv)) {
            
            break;
        }
    }
    
    if (NS_FAILED(rv))
        (void)CloseBlockFiles(false); 

    return rv;
}


nsresult
nsDiskCacheMap::CloseBlockFiles(bool flush)
{
    nsresult rv, rv2 = NS_OK;
    for (int i=0; i < kNumBlockFiles; ++i) {
        rv = mBlockFile[i].Close(flush);
        if (NS_FAILED(rv))  rv2 = rv;   
    }
    return rv2;
}


bool
nsDiskCacheMap::CacheFilesExist()
{
    nsCOMPtr<nsIFile> blockFile;
    nsresult rv;
    
    for (int i = 0; i < kNumBlockFiles; ++i) {
        bool exists;
        rv = GetBlockFileForIndex(i, getter_AddRefs(blockFile));
        if (NS_FAILED(rv))  return false;

        rv = blockFile->Exists(&exists);
        if (NS_FAILED(rv) || !exists)  return false;
    }

    return true;
}


nsresult
nsDiskCacheMap::CreateCacheSubDirectories()
{
    if (!mCacheDirectory)
        return NS_ERROR_UNEXPECTED;

    for (int32_t index = 0 ; index < 16 ; index++) {
        nsCOMPtr<nsIFile> file;
        nsresult rv = mCacheDirectory->Clone(getter_AddRefs(file));
        if (NS_FAILED(rv))
            return rv;

        rv = file->AppendNative(nsPrintfCString("%X", index));
        if (NS_FAILED(rv))
            return rv;

        rv = file->Create(nsIFile::DIRECTORY_TYPE, 0700);
        if (NS_FAILED(rv))
            return rv;
    }

    return NS_OK;
}


nsDiskCacheEntry *
nsDiskCacheMap::ReadDiskCacheEntry(nsDiskCacheRecord * record)
{
    CACHE_LOG_DEBUG(("CACHE: ReadDiskCacheEntry [%x]\n", record->HashNumber()));

    nsresult            rv         = NS_ERROR_UNEXPECTED;
    nsDiskCacheEntry *  diskEntry  = nullptr;
    uint32_t            metaFile   = record->MetaFile();
    int32_t             bytesRead  = 0;
    
    if (!record->MetaLocationInitialized())  return nullptr;
    
    if (metaFile == 0) {  
        
        nsCOMPtr<nsIFile> file;
        rv = GetLocalFileForDiskCacheRecord(record,
                                            nsDiskCache::kMetaData,
                                            false,
                                            getter_AddRefs(file));
        NS_ENSURE_SUCCESS(rv, nullptr);

        CACHE_LOG_DEBUG(("CACHE: nsDiskCacheMap::ReadDiskCacheEntry"
                         "[this=%p] reading disk cache entry", this));

        PRFileDesc * fd = nullptr;

        
        rv = file->OpenNSPRFileDesc(PR_RDONLY, 00600, &fd);
        NS_ENSURE_SUCCESS(rv, nullptr);
        
        int32_t fileSize = PR_Available(fd);
        if (fileSize < 0) {
            
            rv = NS_ERROR_UNEXPECTED;
        } else {
            rv = EnsureBuffer(fileSize);
            if (NS_SUCCEEDED(rv)) {
                bytesRead = PR_Read(fd, mBuffer, fileSize);
                if (bytesRead < fileSize) {
                    rv = NS_ERROR_UNEXPECTED;
                }
            }
        }
        PR_Close(fd);
        NS_ENSURE_SUCCESS(rv, nullptr);

    } else if (metaFile < (kNumBlockFiles + 1)) {
        
        
        
        uint32_t blockCount = record->MetaBlockCount();
        bytesRead = blockCount * GetBlockSizeForIndex(metaFile);

        rv = EnsureBuffer(bytesRead);
        NS_ENSURE_SUCCESS(rv, nullptr);
        
        
        
        
        rv = mBlockFile[metaFile - 1].ReadBlocks(mBuffer,
                                                 record->MetaStartBlock(),
                                                 blockCount, 
                                                 &bytesRead);
        NS_ENSURE_SUCCESS(rv, nullptr);
    }
    diskEntry = (nsDiskCacheEntry *)mBuffer;
    diskEntry->Unswap();    
    
    if (bytesRead < 0 || (uint32_t)bytesRead < diskEntry->Size())
        return nullptr;

    
    return diskEntry;
}







nsDiskCacheEntry *
nsDiskCacheMap::CreateDiskCacheEntry(nsDiskCacheBinding *  binding,
                                     uint32_t * aSize)
{
    nsCacheEntry * entry = binding->mCacheEntry;
    if (!entry)  return nullptr;
    
    
    nsCOMPtr<nsISupports> infoObj = entry->SecurityInfo();
    nsCOMPtr<nsISerializable> serializable = do_QueryInterface(infoObj);
    if (infoObj && !serializable) return nullptr;
    if (serializable) {
        nsCString info;
        nsresult rv = NS_SerializeToString(serializable, info);
        if (NS_FAILED(rv)) return nullptr;
        rv = entry->SetMetaDataElement("security-info", info.get());
        if (NS_FAILED(rv)) return nullptr;
    }

    uint32_t  keySize  = entry->Key()->Length() + 1;
    uint32_t  metaSize = entry->MetaDataSize();
    uint32_t  size     = sizeof(nsDiskCacheEntry) + keySize + metaSize;
    
    if (aSize) *aSize = size;
    
    nsresult rv = EnsureBuffer(size);
    if (NS_FAILED(rv)) return nullptr;

    nsDiskCacheEntry *diskEntry = (nsDiskCacheEntry *)mBuffer;
    diskEntry->mHeaderVersion   = nsDiskCache::kCurrentVersion;
    diskEntry->mMetaLocation    = binding->mRecord.MetaLocation();
    diskEntry->mFetchCount      = entry->FetchCount();
    diskEntry->mLastFetched     = entry->LastFetched();
    diskEntry->mLastModified    = entry->LastModified();
    diskEntry->mExpirationTime  = entry->ExpirationTime();
    diskEntry->mDataSize        = entry->DataSize();
    diskEntry->mKeySize         = keySize;
    diskEntry->mMetaDataSize    = metaSize;
    
    memcpy(diskEntry->Key(), entry->Key()->get(), keySize);
    
    rv = entry->FlattenMetaData(diskEntry->MetaData(), metaSize);
    if (NS_FAILED(rv)) return nullptr;
    
    return diskEntry;
}


nsresult
nsDiskCacheMap::WriteDiskCacheEntry(nsDiskCacheBinding *  binding)
{
    CACHE_LOG_DEBUG(("CACHE: WriteDiskCacheEntry [%x]\n",
        binding->mRecord.HashNumber()));

    mozilla::eventtracer::AutoEventTracer writeDiskCacheEntry(
        binding->mCacheEntry,
        mozilla::eventtracer::eExec,
        mozilla::eventtracer::eDone,
        "net::cache::WriteDiskCacheEntry");

    nsresult            rv        = NS_OK;
    uint32_t            size;
    nsDiskCacheEntry *  diskEntry =  CreateDiskCacheEntry(binding, &size);
    if (!diskEntry)  return NS_ERROR_UNEXPECTED;
    
    uint32_t  fileIndex = CalculateFileIndex(size);

    
    if (binding->mRecord.MetaLocationInitialized()) {
        

        if ((binding->mRecord.MetaFile() == 0) &&
            (fileIndex == 0)) {  
            
            DecrementTotalSize(binding->mRecord.MetaFileSize());
            NS_ASSERTION(binding->mRecord.MetaFileGeneration() == binding->mGeneration,
                         "generations out of sync");
        } else {
            rv = DeleteStorage(&binding->mRecord, nsDiskCache::kMetaData);
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }

    binding->mRecord.SetEvictionRank(ULONG_MAX - SecondsFromPRTime(PR_Now()));
    
    diskEntry->Swap();

    if (fileIndex != 0) {
        while (1) {
            uint32_t  blockSize = GetBlockSizeForIndex(fileIndex);
            uint32_t  blocks    = ((size - 1) / blockSize) + 1;

            int32_t startBlock;
            rv = mBlockFile[fileIndex - 1].WriteBlocks(diskEntry, size, blocks,
                                                       &startBlock);
            if (NS_SUCCEEDED(rv)) {
                
                binding->mRecord.SetMetaBlocks(fileIndex, startBlock, blocks);

                rv = UpdateRecord(&binding->mRecord);
                NS_ENSURE_SUCCESS(rv, rv);

                

                IncrementTotalSize(blocks, blockSize);
                break;
            }

            if (fileIndex == kNumBlockFiles) {
                fileIndex = 0; 
                break;
            }

            
            fileIndex++;
        }
    }

    if (fileIndex == 0) {
        
        uint32_t metaFileSizeK = ((size + 0x03FF) >> 10); 
        if (metaFileSizeK > kMaxDataSizeK)
            metaFileSizeK = kMaxDataSizeK;

        binding->mRecord.SetMetaFileGeneration(binding->mGeneration);
        binding->mRecord.SetMetaFileSize(metaFileSizeK);
        rv = UpdateRecord(&binding->mRecord);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<nsIFile> localFile;
        rv = GetLocalFileForDiskCacheRecord(&binding->mRecord,
                                            nsDiskCache::kMetaData,
                                            true,
                                            getter_AddRefs(localFile));
        NS_ENSURE_SUCCESS(rv, rv);
        
        
        PRFileDesc * fd;
        
        rv = localFile->OpenNSPRFileDesc(PR_RDWR | PR_TRUNCATE | PR_CREATE_FILE, 00600, &fd);
        NS_ENSURE_SUCCESS(rv, rv);

        
        int32_t bytesWritten = PR_Write(fd, diskEntry, size);
        
        PRStatus err = PR_Close(fd);
        if ((bytesWritten != (int32_t)size) || (err != PR_SUCCESS)) {
            return NS_ERROR_UNEXPECTED;
        }

        IncrementTotalSize(metaFileSizeK);
    }

    return rv;
}


nsresult
nsDiskCacheMap::ReadDataCacheBlocks(nsDiskCacheBinding * binding, char * buffer, uint32_t size)
{
    CACHE_LOG_DEBUG(("CACHE: ReadDataCacheBlocks [%x size=%u]\n",
        binding->mRecord.HashNumber(), size));

    uint32_t  fileIndex = binding->mRecord.DataFile();
    int32_t   readSize = size;
    
    nsresult rv = mBlockFile[fileIndex - 1].ReadBlocks(buffer,
                                                       binding->mRecord.DataStartBlock(),
                                                       binding->mRecord.DataBlockCount(),
                                                       &readSize);
    NS_ENSURE_SUCCESS(rv, rv);
    if (readSize < (int32_t)size) {
        rv = NS_ERROR_UNEXPECTED;
    } 
    return rv;
}


nsresult
nsDiskCacheMap::WriteDataCacheBlocks(nsDiskCacheBinding * binding, char * buffer, uint32_t size)
{
    CACHE_LOG_DEBUG(("CACHE: WriteDataCacheBlocks [%x size=%u]\n",
        binding->mRecord.HashNumber(), size));

    mozilla::eventtracer::AutoEventTracer writeDataCacheBlocks(
        binding->mCacheEntry,
        mozilla::eventtracer::eExec,
        mozilla::eventtracer::eDone,
        "net::cache::WriteDataCacheBlocks");

    nsresult  rv = NS_OK;
    
    
    uint32_t  fileIndex  = CalculateFileIndex(size);
    uint32_t  blockCount = 0;
    int32_t   startBlock = 0;

    if (size > 0) {
        while (1) {
            uint32_t  blockSize  = GetBlockSizeForIndex(fileIndex);
            blockCount = ((size - 1) / blockSize) + 1;

            rv = mBlockFile[fileIndex - 1].WriteBlocks(buffer, size, blockCount,
                                                       &startBlock);
            if (NS_SUCCEEDED(rv)) {
                IncrementTotalSize(blockCount, blockSize);
                break;
            }

            if (fileIndex == kNumBlockFiles)
                return rv;

            fileIndex++;
        }
    }

    
    binding->mRecord.SetDataBlocks(fileIndex, startBlock, blockCount);
    if (!binding->mDoomed) {
        rv = UpdateRecord(&binding->mRecord);
    }
    return rv;
}


nsresult
nsDiskCacheMap::DeleteStorage(nsDiskCacheRecord * record)
{
    nsresult  rv1 = DeleteStorage(record, nsDiskCache::kData);
    nsresult  rv2 = DeleteStorage(record, nsDiskCache::kMetaData);
    return NS_FAILED(rv1) ? rv1 : rv2;
}


nsresult
nsDiskCacheMap::DeleteStorage(nsDiskCacheRecord * record, bool metaData)
{
    CACHE_LOG_DEBUG(("CACHE: DeleteStorage [%x %u]\n", record->HashNumber(),
        metaData));

    nsresult    rv = NS_ERROR_UNEXPECTED;
    uint32_t    fileIndex = metaData ? record->MetaFile() : record->DataFile();
    nsCOMPtr<nsIFile> file;
    
    if (fileIndex == 0) {
        
        uint32_t  sizeK = metaData ? record->MetaFileSize() : record->DataFileSize();
        

        rv = GetFileForDiskCacheRecord(record, metaData, false, getter_AddRefs(file));
        if (NS_SUCCEEDED(rv)) {
            rv = file->Remove(false);    
        }
        DecrementTotalSize(sizeK);
        
    } else if (fileIndex < (kNumBlockFiles + 1)) {
        
        uint32_t  startBlock = metaData ? record->MetaStartBlock() : record->DataStartBlock();
        uint32_t  blockCount = metaData ? record->MetaBlockCount() : record->DataBlockCount();
        
        rv = mBlockFile[fileIndex - 1].DeallocateBlocks(startBlock, blockCount);
        DecrementTotalSize(blockCount, GetBlockSizeForIndex(fileIndex));
    }
    if (metaData)  record->ClearMetaLocation();
    else           record->ClearDataLocation();
    
    return rv;
}


nsresult
nsDiskCacheMap::GetFileForDiskCacheRecord(nsDiskCacheRecord * record,
                                          bool                meta,
                                          bool                createPath,
                                          nsIFile **          result)
{
    if (!mCacheDirectory)  return NS_ERROR_NOT_AVAILABLE;
    
    nsCOMPtr<nsIFile> file;
    nsresult rv = mCacheDirectory->Clone(getter_AddRefs(file));
    if (NS_FAILED(rv))  return rv;

    uint32_t hash = record->HashNumber();

    
    
    rv = file->AppendNative(nsPrintfCString("%X", hash >> 28));
    if (NS_FAILED(rv))  return rv;
    rv = file->AppendNative(nsPrintfCString("%02X", (hash >> 20) & 0xFF));
    if (NS_FAILED(rv))  return rv;

    bool exists;
    if (createPath && (NS_FAILED(file->Exists(&exists)) || !exists)) {
        rv = file->Create(nsIFile::DIRECTORY_TYPE, 0700);
        if (NS_FAILED(rv))  return rv;
    }

    int16_t generation = record->Generation();
    char name[32];
    
    ::sprintf(name, "%05X%c%02X", hash & 0xFFFFF, (meta ? 'm' : 'd'),
              generation);
    rv = file->AppendNative(nsDependentCString(name));
    if (NS_FAILED(rv))  return rv;
    
    NS_IF_ADDREF(*result = file);
    return rv;
}


nsresult
nsDiskCacheMap::GetLocalFileForDiskCacheRecord(nsDiskCacheRecord * record,
                                               bool                meta,
                                               bool                createPath,
                                               nsIFile **          result)
{
    nsCOMPtr<nsIFile> file;
    nsresult rv = GetFileForDiskCacheRecord(record,
                                            meta,
                                            createPath,
                                            getter_AddRefs(file));
    if (NS_FAILED(rv))  return rv;
    
    NS_IF_ADDREF(*result = file);
    return rv;
}


nsresult
nsDiskCacheMap::GetBlockFileForIndex(uint32_t index, nsIFile ** result)
{
    if (!mCacheDirectory)  return NS_ERROR_NOT_AVAILABLE;
    
    nsCOMPtr<nsIFile> file;
    nsresult rv = mCacheDirectory->Clone(getter_AddRefs(file));
    if (NS_FAILED(rv))  return rv;
    
    char name[32];
    ::sprintf(name, "_CACHE_%03d_", index + 1);
    rv = file->AppendNative(nsDependentCString(name));
    if (NS_FAILED(rv))  return rv;
    
    NS_IF_ADDREF(*result = file);

    return rv;
}


uint32_t
nsDiskCacheMap::CalculateFileIndex(uint32_t size)
{
    
    
    

    if (size <= 3 * BLOCK_SIZE_FOR_INDEX(1))  return 1;
    if (size <= 3 * BLOCK_SIZE_FOR_INDEX(2))  return 2;
    if (size <= 4 * BLOCK_SIZE_FOR_INDEX(3))  return 3;
    return 0;
}

nsresult
nsDiskCacheMap::EnsureBuffer(uint32_t bufSize)
{
    if (mBufferSize < bufSize) {
        char * buf = (char *)PR_REALLOC(mBuffer, bufSize);
        if (!buf) {
            mBufferSize = 0;
            return NS_ERROR_OUT_OF_MEMORY;
        }
        mBuffer = buf;
        mBufferSize = bufSize;
    }
    return NS_OK;
}        

void
nsDiskCacheMap::NotifyCapacityChange(uint32_t capacity)
{
  
  
  
  const int32_t RECORD_COUNT_LIMIT = 32 * 1024 * 1024 / sizeof(nsDiskCacheRecord);
  int32_t maxRecordCount = std::min(int32_t(capacity), RECORD_COUNT_LIMIT);
  if (mMaxRecordCount < maxRecordCount) {
    
    mMaxRecordCount = maxRecordCount;
  }
}

size_t
nsDiskCacheMap::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf)
{
  size_t usage = aMallocSizeOf(mRecordArray);

  usage += aMallocSizeOf(mBuffer);
  usage += aMallocSizeOf(mMapFD);
  usage += aMallocSizeOf(mCleanFD);
  usage += aMallocSizeOf(mCacheDirectory);
  usage += aMallocSizeOf(mCleanCacheTimer);

  for (int i = 0; i < kNumBlockFiles; i++) {
    usage += mBlockFile[i].SizeOfExcludingThis(aMallocSizeOf);
  }

  return usage;
}

nsresult
nsDiskCacheMap::InitCacheClean(nsIFile *  cacheDirectory,
                               nsDiskCache::CorruptCacheInfo *  corruptInfo,
                               bool reportCacheCleanTelemetryData)
{
    
    
    bool cacheCleanFileExists = false;
    nsCOMPtr<nsIFile> cacheCleanFile;
    nsresult rv = cacheDirectory->GetParent(getter_AddRefs(cacheCleanFile));
    if (NS_SUCCEEDED(rv)) {
        rv = cacheCleanFile->AppendNative(
                 NS_LITERAL_CSTRING("_CACHE_CLEAN_"));
        if (NS_SUCCEEDED(rv)) {
            
            
            cacheCleanFile->Exists(&cacheCleanFileExists);
        }
    }
    if (NS_FAILED(rv)) {
        NS_WARNING("Could not build cache clean file path");
        *corruptInfo = nsDiskCache::kCacheCleanFilePathError;
        return rv;
    }

    
    rv = cacheCleanFile->OpenNSPRFileDesc(PR_RDWR | PR_CREATE_FILE,
                                          00600, &mCleanFD);
    if (NS_FAILED(rv)) {
        NS_WARNING("Could not open cache clean file");
        *corruptInfo = nsDiskCache::kCacheCleanOpenFileError;
        return rv;
    }

    if (cacheCleanFileExists) {
        char clean = '0';
        int32_t bytesRead = PR_Read(mCleanFD, &clean, 1);
        if (bytesRead != 1) {
            NS_WARNING("Could not read _CACHE_CLEAN_ file contents");
        } else if (reportCacheCleanTelemetryData) {
            Telemetry::Accumulate(Telemetry::DISK_CACHE_REDUCTION_TRIAL,
                                  clean == '1' ? 1 : 0);
        }
    }

    
    
    mCleanCacheTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
    if (NS_SUCCEEDED(rv)) {
        mCleanCacheTimer->SetTarget(nsCacheService::GlobalInstance()->mCacheIOThread);
        rv = ResetCacheTimer();
    }

    if (NS_FAILED(rv)) {
        NS_WARNING("Could not create cache clean timer");
        mCleanCacheTimer = nullptr;
        *corruptInfo = nsDiskCache::kCacheCleanTimerError;
        return rv;
    }

    return NS_OK;
}

nsresult
nsDiskCacheMap::WriteCacheClean(bool clean)
{
    nsCacheService::AssertOwnsLock();
    if (!mCleanFD) {
        NS_WARNING("Cache clean file is not open!");
        return NS_ERROR_FAILURE;
    }

    CACHE_LOG_DEBUG(("CACHE: WriteCacheClean: %d\n", clean? 1 : 0));
    
    
    char data = clean? '1' : '0';
    int32_t filePos = PR_Seek(mCleanFD, 0, PR_SEEK_SET);
    if (filePos != 0) {
        NS_WARNING("Could not seek in cache clean file!");
        return NS_ERROR_FAILURE;
    }
    int32_t bytesWritten = PR_Write(mCleanFD, &data, 1);
    if (bytesWritten != 1) {
        NS_WARNING("Could not write cache clean file!");
        return NS_ERROR_FAILURE;
    }
    PRStatus err = PR_Sync(mCleanFD);
    if (err != PR_SUCCESS) {
        NS_WARNING("Could not flush cache clean file!");
    }

    return NS_OK;
}

nsresult
nsDiskCacheMap::InvalidateCache()
{
    nsCacheService::AssertOwnsLock();
    CACHE_LOG_DEBUG(("CACHE: InvalidateCache\n"));
    nsresult rv;
  
    if (!mIsDirtyCacheFlushed) {
        rv = WriteCacheClean(false);
        if (NS_FAILED(rv)) {
          Telemetry::Accumulate(Telemetry::DISK_CACHE_INVALIDATION_SUCCESS, 0);
          return rv;
        }

        Telemetry::Accumulate(Telemetry::DISK_CACHE_INVALIDATION_SUCCESS, 1);
        mIsDirtyCacheFlushed = true;
    }

    rv = ResetCacheTimer();
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
nsDiskCacheMap::ResetCacheTimer(int32_t timeout)
{
    mCleanCacheTimer->Cancel();
    nsresult rv =
      mCleanCacheTimer->InitWithFuncCallback(RevalidateTimerCallback,
                                             nullptr, timeout,
                                             nsITimer::TYPE_ONE_SHOT);
    NS_ENSURE_SUCCESS(rv, rv);
    mLastInvalidateTime = PR_IntervalNow();

    return rv;
}

void
nsDiskCacheMap::RevalidateTimerCallback(nsITimer *aTimer, void *arg)
{
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSDISKCACHEMAP_REVALIDATION));
    if (!nsCacheService::gService->mDiskDevice ||
        !nsCacheService::gService->mDiskDevice->Initialized()) {
        return;
    }

    nsDiskCacheMap *diskCacheMap =
        &nsCacheService::gService->mDiskDevice->mCacheMap;

    
    
    
    
    
    
    uint32_t delta =
        PR_IntervalToMilliseconds(PR_IntervalNow() -
                                  diskCacheMap->mLastInvalidateTime) +
        kRevalidateCacheTimeoutTolerance;
    if (delta < kRevalidateCacheTimeout) {
        diskCacheMap->ResetCacheTimer();
        return;
    }

    nsresult rv = diskCacheMap->RevalidateCache();
    if (NS_FAILED(rv)) {
        diskCacheMap->ResetCacheTimer(kRevalidateCacheErrorTimeout);
    }
}

bool
nsDiskCacheMap::IsCacheInSafeState()
{
    return nsCacheService::GlobalInstance()->IsDoomListEmpty();
}

nsresult
nsDiskCacheMap::RevalidateCache()
{
    CACHE_LOG_DEBUG(("CACHE: RevalidateCache\n"));
    nsresult rv;

    if (!IsCacheInSafeState()) {
        Telemetry::Accumulate(Telemetry::DISK_CACHE_REVALIDATION_SAFE, 0);
        CACHE_LOG_DEBUG(("CACHE: Revalidation should not performed because "
                         "cache not in a safe state\n"));
        
        
        
        
        
    } else {
        Telemetry::Accumulate(Telemetry::DISK_CACHE_REVALIDATION_SAFE, 1);
    }

    
    Telemetry::AutoTimer<Telemetry::NETWORK_DISK_CACHE_REVALIDATION> totalTimer;

    
    
  
    
    rv = WriteCacheClean(true);
    if (NS_FAILED(rv)) {
        Telemetry::Accumulate(Telemetry::DISK_CACHE_REVALIDATION_SUCCESS, 0);
        return rv;
    }

    Telemetry::Accumulate(Telemetry::DISK_CACHE_REVALIDATION_SUCCESS, 1);
    mIsDirtyCacheFlushed = false;

    return NS_OK;
}

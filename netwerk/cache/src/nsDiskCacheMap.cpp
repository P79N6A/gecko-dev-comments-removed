










































#include "nsDiskCacheMap.h"
#include "nsDiskCacheBinding.h"
#include "nsDiskCacheEntry.h"

#include "nsCache.h"

#include <string.h>










nsresult
nsDiskCacheMap::Open(nsILocalFile *  cacheDirectory)
{
    NS_ENSURE_ARG_POINTER(cacheDirectory);
    if (mMapFD)  return NS_ERROR_ALREADY_INITIALIZED;

    mCacheDirectory = cacheDirectory;   
    
    
    nsresult rv;
    nsCOMPtr<nsIFile> file;
    rv = cacheDirectory->Clone(getter_AddRefs(file));
    nsCOMPtr<nsILocalFile> localFile(do_QueryInterface(file, &rv));
    if (NS_FAILED(rv))  return rv;
    rv = localFile->AppendNative(NS_LITERAL_CSTRING("_CACHE_MAP_"));
    if (NS_FAILED(rv))  return rv;

    
    rv = localFile->OpenNSPRFileDesc(PR_RDWR | PR_CREATE_FILE, 00600, &mMapFD);
    if (NS_FAILED(rv))  return NS_ERROR_FILE_CORRUPTED;

    PRBool cacheFilesExist = CacheFilesExist();
    rv = NS_ERROR_FILE_CORRUPTED;  

    
    PRUint32 mapSize = PR_Available(mMapFD);    
    if (mapSize == 0) {  

        
        if (cacheFilesExist)
            goto error_exit;

        
        memset(&mHeader, 0, sizeof(nsDiskCacheHeader));
        mHeader.mVersion = nsDiskCache::kCurrentVersion;
        mHeader.mRecordCount = kMinRecordCount;
        mRecordArray = (nsDiskCacheRecord *)
            PR_CALLOC(mHeader.mRecordCount * sizeof(nsDiskCacheRecord));
        if (!mRecordArray) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            goto error_exit;
        }
    } else if (mapSize >= sizeof(nsDiskCacheHeader)) {  
        
        
        if (!cacheFilesExist)
            goto error_exit;

        
        PRUint32 bytesRead = PR_Read(mMapFD, &mHeader, sizeof(nsDiskCacheHeader));
        if (sizeof(nsDiskCacheHeader) != bytesRead)  goto error_exit;
        mHeader.Unswap();

        if (mHeader.mIsDirty || (mHeader.mVersion != nsDiskCache::kCurrentVersion))
            goto error_exit;

        PRUint32 recordArraySize =
                mHeader.mRecordCount * sizeof(nsDiskCacheRecord);
        if (mapSize < recordArraySize + sizeof(nsDiskCacheHeader))
            goto error_exit;

        
        mRecordArray = (nsDiskCacheRecord *) PR_MALLOC(recordArraySize);
        if (!mRecordArray) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            goto error_exit;
        }

        
        bytesRead = PR_Read(mMapFD, mRecordArray, recordArraySize);
        if (bytesRead < recordArraySize)
            goto error_exit;

        
        PRInt32 total = 0;
        for (PRInt32 i = 0; i < mHeader.mRecordCount; ++i) {
            if (mRecordArray[i].HashNumber()) {
#if defined(IS_LITTLE_ENDIAN)
                mRecordArray[i].Unswap();
#endif
                total ++;
            }
        }
        
        
        if (total != mHeader.mEntryCount)
            goto error_exit;

    } else {
        goto error_exit;
    }

    rv = OpenBlockFiles();
    if (NS_FAILED(rv))  goto error_exit;

    
    mHeader.mIsDirty    = PR_TRUE;
    rv = FlushHeader();
    if (NS_FAILED(rv))  goto error_exit;
    
    return NS_OK;
    
error_exit:
    (void) Close(PR_FALSE);
       
    return rv;
}


nsresult
nsDiskCacheMap::Close(PRBool flush)
{
    nsresult  rv = NS_OK;

    
    if (mMapFD) {
        
        rv = CloseBlockFiles(flush);
        if (NS_SUCCEEDED(rv) && flush && mRecordArray) {
            
            rv = FlushRecords(PR_FALSE);   
            if (NS_SUCCEEDED(rv)) {
                
                mHeader.mIsDirty = PR_FALSE;
                rv = FlushHeader();
            }
        }
        if ((PR_Close(mMapFD) != PR_SUCCESS) && (NS_SUCCEEDED(rv)))
            rv = NS_ERROR_UNEXPECTED;

        mMapFD = nsnull;
    }
    PR_FREEIF(mRecordArray);
    return rv;
}


nsresult
nsDiskCacheMap::Trim()
{
    nsresult rv, rv2 = NS_OK;
    for (int i=0; i < 3; ++i) {
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
    
    
    PRInt32 filePos = PR_Seek(mMapFD, 0, PR_SEEK_SET);
    if (filePos != 0)  return NS_ERROR_UNEXPECTED;
    
    
    mHeader.Swap();
    PRInt32 bytesWritten = PR_Write(mMapFD, &mHeader, sizeof(nsDiskCacheHeader));
    mHeader.Unswap();
    if (sizeof(nsDiskCacheHeader) != bytesWritten) {
        return NS_ERROR_UNEXPECTED;
    }
    
    return NS_OK;
}


nsresult
nsDiskCacheMap::FlushRecords(PRBool unswap)
{
    if (!mMapFD)  return NS_ERROR_NOT_AVAILABLE;
    
    
    PRInt32 filePos = PR_Seek(mMapFD, sizeof(nsDiskCacheHeader), PR_SEEK_SET);
    if (filePos != sizeof(nsDiskCacheHeader))
        return NS_ERROR_UNEXPECTED;
    
#if defined(IS_LITTLE_ENDIAN)
    
    for (PRInt32 i = 0; i < mHeader.mRecordCount; ++i) {
        if (mRecordArray[i].HashNumber())   
            mRecordArray[i].Swap();
    }
#endif
    
    PRInt32 recordArraySize = sizeof(nsDiskCacheRecord) * mHeader.mRecordCount;

    PRInt32 bytesWritten = PR_Write(mMapFD, mRecordArray, recordArraySize);
    if (bytesWritten != recordArraySize)
        return NS_ERROR_UNEXPECTED;

#if defined(IS_LITTLE_ENDIAN)
    if (unswap) {
        
        for (PRInt32 i = 0; i < mHeader.mRecordCount; ++i) {
            if (mRecordArray[i].HashNumber())   
                mRecordArray[i].Unswap();
        }
    }
#endif
    
    return NS_OK;
}






PRUint32
nsDiskCacheMap::GetBucketRank(PRUint32 bucketIndex, PRUint32 targetRank)
{
    nsDiskCacheRecord * records = GetFirstRecordInBucket(bucketIndex);
    PRUint32            rank = 0;

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
    if (mHeader.mRecordCount >= kMaxRecordCount)
        return NS_OK;
    CACHE_LOG_DEBUG(("CACHE: GrowRecords\n"));

    
    PRUint32 newCount = mHeader.mRecordCount << 1;
    if (newCount > kMaxRecordCount)
        newCount = kMaxRecordCount;
    nsDiskCacheRecord *newArray = (nsDiskCacheRecord *)
            PR_REALLOC(mRecordArray, newCount * sizeof(nsDiskCacheRecord));
    if (!newArray)
        return NS_ERROR_OUT_OF_MEMORY;

    
    PRUint32 oldRecordsPerBucket = GetRecordsPerBucket();
    PRUint32 newRecordsPerBucket = newCount / kBuckets;
    
    for (int bucketIndex = kBuckets - 1; bucketIndex >= 0; --bucketIndex) {
        
        nsDiskCacheRecord *newRecords = newArray + bucketIndex * newRecordsPerBucket;
        const PRUint32 count = mHeader.mBucketUsage[bucketIndex];
        memmove(newRecords,
                newArray + bucketIndex * oldRecordsPerBucket,
                count * sizeof(nsDiskCacheRecord));
        
        for (PRUint32 i = count; i < newRecordsPerBucket; ++i)
            newRecords[i].SetHashNumber(0);
    }

    
    mRecordArray = newArray;
    mHeader.mRecordCount = newCount;
    return NS_OK;
}

nsresult
nsDiskCacheMap::ShrinkRecords()
{
    if (mHeader.mRecordCount <= kMinRecordCount)
        return NS_OK;
    CACHE_LOG_DEBUG(("CACHE: ShrinkRecords\n"));

    
    
    PRUint32 maxUsage = 0, bucketIndex;
    for (bucketIndex = 0; bucketIndex < kBuckets; ++bucketIndex) {
        if (maxUsage < mHeader.mBucketUsage[bucketIndex])
            maxUsage = mHeader.mBucketUsage[bucketIndex];
    }
    
    PRUint32 oldRecordsPerBucket = GetRecordsPerBucket();
    PRUint32 newRecordsPerBucket = oldRecordsPerBucket;
    while (maxUsage < (newRecordsPerBucket >> 1))
        newRecordsPerBucket >>= 1;
    if (newRecordsPerBucket < kMinRecordCount) 
        newRecordsPerBucket = kMinRecordCount;
    if (newRecordsPerBucket == oldRecordsPerBucket)
        return NS_OK;
    
    for (bucketIndex = 0; bucketIndex < kBuckets; ++bucketIndex) {
        
        memmove(mRecordArray + bucketIndex * newRecordsPerBucket,
                mRecordArray + bucketIndex * oldRecordsPerBucket,
                mHeader.mBucketUsage[bucketIndex] * sizeof(nsDiskCacheRecord));
    }

    
    PRUint32 newCount = newRecordsPerBucket * kBuckets;
    nsDiskCacheRecord* newArray = (nsDiskCacheRecord *)
            PR_REALLOC(mRecordArray, newCount * sizeof(nsDiskCacheRecord));
    if (!newArray)
        return NS_ERROR_OUT_OF_MEMORY;

    
    mRecordArray = newArray;
    mHeader.mRecordCount = newCount;
    return NS_OK;
}

nsresult
nsDiskCacheMap::AddRecord( nsDiskCacheRecord *  mapRecord,
                           nsDiskCacheRecord *  oldRecord)
{
    CACHE_LOG_DEBUG(("CACHE: AddRecord [%x]\n", mapRecord->HashNumber()));

    const PRUint32      hashNumber = mapRecord->HashNumber();
    const PRUint32      bucketIndex = GetBucketIndex(hashNumber);
    const PRUint32      count = mHeader.mBucketUsage[bucketIndex];

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
    }

    NS_ASSERTION(mHeader.mEvictionRank[bucketIndex] == GetBucketRank(bucketIndex, 0),
                 "eviction rank out of sync");
    return NS_OK;
}


nsresult
nsDiskCacheMap::UpdateRecord( nsDiskCacheRecord *  mapRecord)
{
    CACHE_LOG_DEBUG(("CACHE: UpdateRecord [%x]\n", mapRecord->HashNumber()));

    const PRUint32      hashNumber = mapRecord->HashNumber();
    const PRUint32      bucketIndex = GetBucketIndex(hashNumber);
    nsDiskCacheRecord * records = GetFirstRecordInBucket(bucketIndex);

    for (int i = mHeader.mBucketUsage[bucketIndex]-1; i >= 0; i--) {          
        if (records[i].HashNumber() == hashNumber) {
            const PRUint32 oldRank = records[i].EvictionRank();

            
            records[i] = *mapRecord;

            
            if (mHeader.mEvictionRank[bucketIndex] < mapRecord->EvictionRank())
                mHeader.mEvictionRank[bucketIndex] = mapRecord->EvictionRank();
            else if (mHeader.mEvictionRank[bucketIndex] == oldRank)
                mHeader.mEvictionRank[bucketIndex] = GetBucketRank(bucketIndex, 0);

NS_ASSERTION(mHeader.mEvictionRank[bucketIndex] == GetBucketRank(bucketIndex, 0),
             "eviction rank out of sync");
            return NS_OK;
        }
    }
    NS_NOTREACHED("record not found");
    return NS_ERROR_UNEXPECTED;
}


nsresult
nsDiskCacheMap::FindRecord( PRUint32  hashNumber, nsDiskCacheRecord *  result)
{
    const PRUint32      bucketIndex = GetBucketIndex(hashNumber);
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

    const PRUint32      hashNumber = mapRecord->HashNumber();
    const PRUint32      bucketIndex = GetBucketIndex(hashNumber);
    nsDiskCacheRecord * records = GetFirstRecordInBucket(bucketIndex);
    PRUint32            last = mHeader.mBucketUsage[bucketIndex]-1;

    for (int i = last; i >= 0; i--) {          
        if (records[i].HashNumber() == hashNumber) {
            
            PRUint32  evictionRank = records[i].EvictionRank();
            NS_ASSERTION(evictionRank == mapRecord->EvictionRank(),
                         "evictionRank out of sync");
            
            records[i] = records[last];
            records[last].SetHashNumber(0); 
            mHeader.mBucketUsage[bucketIndex] = last;
            mHeader.mEntryCount--;

            
            PRUint32  bucketIndex = GetBucketIndex(mapRecord->HashNumber());
            if (mHeader.mEvictionRank[bucketIndex] <= evictionRank) {
                mHeader.mEvictionRank[bucketIndex] = GetBucketRank(bucketIndex, 0);
            }

            NS_ASSERTION(mHeader.mEvictionRank[bucketIndex] ==
                         GetBucketRank(bucketIndex, 0), "eviction rank out of sync");
            return NS_OK;
        }
    }
    return NS_ERROR_UNEXPECTED;
}


PRInt32
nsDiskCacheMap::VisitEachRecord(PRUint32                    bucketIndex,
                                nsDiskCacheRecordVisitor *  visitor,
                                PRUint32                    evictionRank)
{
    PRInt32             rv = kVisitNextRecord;
    PRUint32            count = mHeader.mBucketUsage[bucketIndex];
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
    PRUint32  tempRank[kBuckets];
    int       bucketIndex = 0;
    
    
    for (bucketIndex = 0; bucketIndex < kBuckets; ++bucketIndex)
        tempRank[bucketIndex] = mHeader.mEvictionRank[bucketIndex];

    
    
    for (int n = 0; n < mHeader.mEntryCount; ++n) {
    
        
        PRUint32    rank  = 0;
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
nsDiskCacheMap::OpenBlockFiles()
{
    
    nsCOMPtr<nsILocalFile> blockFile;
    nsresult rv;
    
    for (int i = 0; i < 3; ++i) {
        rv = GetBlockFileForIndex(i, getter_AddRefs(blockFile));
        if (NS_FAILED(rv))  goto error_exit;
    
        PRUint32 blockSize = GetBlockSizeForIndex(i+1); 
        rv = mBlockFile[i].Open(blockFile, blockSize);
        if (NS_FAILED(rv)) goto error_exit;
    }
    return NS_OK;

error_exit:
    (void)CloseBlockFiles(PR_FALSE); 
    return rv;
}


nsresult
nsDiskCacheMap::CloseBlockFiles(PRBool flush)
{
    nsresult rv, rv2 = NS_OK;
    for (int i=0; i < 3; ++i) {
        rv = mBlockFile[i].Close(flush);
        if (NS_FAILED(rv))  rv2 = rv;   
    }
    return rv2;
}


PRBool
nsDiskCacheMap::CacheFilesExist()
{
    nsCOMPtr<nsILocalFile> blockFile;
    nsresult rv;
    
    for (int i = 0; i < 3; ++i) {
        PRBool exists;
        rv = GetBlockFileForIndex(i, getter_AddRefs(blockFile));
        if (NS_FAILED(rv))  return PR_FALSE;

        rv = blockFile->Exists(&exists);
        if (NS_FAILED(rv) || !exists)  return PR_FALSE;
    }

    return PR_TRUE;
}


nsresult
nsDiskCacheMap::ReadDiskCacheEntry(nsDiskCacheRecord * record, nsDiskCacheEntry ** result)
{
    CACHE_LOG_DEBUG(("CACHE: ReadDiskCacheEntry [%x]\n", record->HashNumber()));

    nsresult            rv         = NS_ERROR_UNEXPECTED;
    nsDiskCacheEntry *  diskEntry  = nsnull;
    PRUint32            metaFile   = record->MetaFile();
    PRFileDesc *        fd         = nsnull;
    PRInt32             bytesRead  = 0;
    *result = nsnull;
    
    if (!record->MetaLocationInitialized())  return NS_ERROR_NOT_AVAILABLE;
    
    if (metaFile == 0) {  
        
        nsCOMPtr<nsILocalFile> file;
        rv = GetLocalFileForDiskCacheRecord(record, nsDiskCache::kMetaData, getter_AddRefs(file));
        if (NS_FAILED(rv))  return rv;

        PRFileDesc * fd = nsnull;
        
        rv = file->OpenNSPRFileDesc(PR_RDONLY, 00600, &fd);
        if (NS_FAILED(rv))  return rv;
        
        PRInt32 fileSize = PR_Available(fd);
        if (fileSize < 0) {
            
            rv = NS_ERROR_UNEXPECTED;
            goto exit;
        }

        diskEntry = (nsDiskCacheEntry *) new char[fileSize];
        if (!diskEntry) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            goto exit;
        }
        
        bytesRead = PR_Read(fd, diskEntry, fileSize);
        if (bytesRead < fileSize) {
            rv = NS_ERROR_UNEXPECTED;
            goto exit;
        }

    } else if (metaFile < 4) {  
        
        
        
        PRUint32 blockCount = record->MetaBlockCount();
        bytesRead = blockCount * GetBlockSizeForIndex(metaFile);
        diskEntry = (nsDiskCacheEntry *) new char[bytesRead];
        
        
        
        
        rv = mBlockFile[metaFile - 1].ReadBlocks((char *)diskEntry,
                                                 record->MetaStartBlock(),
                                                 blockCount, 
                                                 &bytesRead);
        if (NS_FAILED(rv))  goto exit;
    }
    
    diskEntry->Unswap();    
    
    if (bytesRead < 0 || (PRUint32)bytesRead < diskEntry->Size()) {
        rv = NS_ERROR_UNEXPECTED;
    } else {
        
        *result = diskEntry;
        diskEntry = nsnull;
    }
exit:
    
    if (fd) (void) PR_Close(fd);
    delete [] (char *)diskEntry;
    return rv;
}


nsresult
nsDiskCacheMap::WriteDiskCacheEntry(nsDiskCacheBinding *  binding)
{
    CACHE_LOG_DEBUG(("CACHE: WriteDiskCacheEntry [%x]\n",
        binding->mRecord.HashNumber()));

    nsresult            rv        = NS_OK;
    PRUint32            size;
    nsDiskCacheEntry *  diskEntry =  CreateDiskCacheEntry(binding, &size);
    if (!diskEntry)  return NS_ERROR_UNEXPECTED;
    
    PRUint32  fileIndex = CalculateFileIndex(size);

    
    if (binding->mRecord.MetaLocationInitialized()) {
        

        if ((binding->mRecord.MetaFile() == 0) &&
            (fileIndex == 0)) {  
            
            
            DecrementTotalSize(binding->mRecord.MetaFileSize());
            NS_ASSERTION(binding->mRecord.MetaFileGeneration() == binding->mGeneration,
                         "generations out of sync");
        } else {
            rv = DeleteStorage(&binding->mRecord, nsDiskCache::kMetaData);
            if (NS_FAILED(rv))  goto exit;
        }
    }

    binding->mRecord.SetEvictionRank(ULONG_MAX - SecondsFromPRTime(PR_Now()));
        
    if (fileIndex == 0) {
        
        PRUint32 metaFileSizeK = ((size + 0x03FF) >> 10); 
        nsCOMPtr<nsILocalFile> localFile;
        
        
        binding->mRecord.SetMetaFileGeneration(binding->mGeneration);
        binding->mRecord.SetMetaFileSize(metaFileSizeK);
        rv = UpdateRecord(&binding->mRecord);
        if (NS_FAILED(rv))  goto exit;

        rv = GetLocalFileForDiskCacheRecord(&binding->mRecord,
                                            nsDiskCache::kMetaData,
                                            getter_AddRefs(localFile));
        if (NS_FAILED(rv))  goto exit;
        
        
        PRFileDesc * fd;
        
        rv = localFile->OpenNSPRFileDesc(PR_RDWR | PR_TRUNCATE | PR_CREATE_FILE, 00600, &fd);
        if (NS_FAILED(rv))  goto exit;  

        
        diskEntry->Swap();
        PRInt32 bytesWritten = PR_Write(fd, diskEntry, size);
        
        PRStatus err = PR_Close(fd);
        if ((bytesWritten != (PRInt32)size) || (err != PR_SUCCESS)) {
            rv = NS_ERROR_UNEXPECTED;
            goto exit;
        }
        
        IncrementTotalSize(metaFileSizeK);
        
    } else {
        PRUint32  blockSize = GetBlockSizeForIndex(fileIndex);
        PRUint32  blocks    = ((size - 1) / blockSize) + 1;

        
        diskEntry->Swap();
        PRInt32 startBlock;
        rv = mBlockFile[fileIndex - 1].WriteBlocks(diskEntry, size, blocks, &startBlock);
        if (NS_FAILED(rv))  goto exit;
        
        
        binding->mRecord.SetMetaBlocks(fileIndex, startBlock, blocks);
        rv = UpdateRecord(&binding->mRecord);
        if (NS_FAILED(rv))  goto exit;
        
        
        IncrementTotalSize(blocks, blockSize);
    }

exit:
    delete [] (char *)diskEntry;
    return rv;
}


nsresult
nsDiskCacheMap::ReadDataCacheBlocks(nsDiskCacheBinding * binding, char * buffer, PRUint32 size)
{
    CACHE_LOG_DEBUG(("CACHE: ReadDataCacheBlocks [%x size=%u]\n",
        binding->mRecord.HashNumber(), size));

    PRUint32  fileIndex = binding->mRecord.DataFile();
    PRInt32   readSize = size;
    
    nsresult rv = mBlockFile[fileIndex - 1].ReadBlocks(buffer,
                                                       binding->mRecord.DataStartBlock(),
                                                       binding->mRecord.DataBlockCount(),
                                                       &readSize);
    NS_ENSURE_SUCCESS(rv, rv);
    if (readSize < (PRInt32)size) {
        rv = NS_ERROR_UNEXPECTED;
    } 
    return rv;
}


nsresult
nsDiskCacheMap::WriteDataCacheBlocks(nsDiskCacheBinding * binding, char * buffer, PRUint32 size)
{
    CACHE_LOG_DEBUG(("CACHE: WriteDataCacheBlocks [%x size=%u]\n",
        binding->mRecord.HashNumber(), size));

    nsresult  rv = NS_OK;
    
    
    PRUint32  fileIndex  = CalculateFileIndex(size);
    PRUint32  blockSize  = GetBlockSizeForIndex(fileIndex);
    PRUint32  blockCount = 0;
    PRInt32   startBlock = 0;
    
    if (size > 0) {
        blockCount = ((size - 1) / blockSize) + 1;

        rv = mBlockFile[fileIndex - 1].WriteBlocks(buffer, size, blockCount, &startBlock);
        NS_ENSURE_SUCCESS(rv, rv);
        
        IncrementTotalSize(blockCount, blockSize);
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
nsDiskCacheMap::DeleteStorage(nsDiskCacheRecord * record, PRBool metaData)
{
    CACHE_LOG_DEBUG(("CACHE: DeleteStorage [%x %u]\n", record->HashNumber(),
        metaData));

    nsresult    rv = NS_ERROR_UNEXPECTED;
    PRUint32    fileIndex = metaData ? record->MetaFile() : record->DataFile();
    nsCOMPtr<nsIFile> file;
    
    if (fileIndex == 0) {
        
        PRUint32  sizeK = metaData ? record->MetaFileSize() : record->DataFileSize();
        

        rv = GetFileForDiskCacheRecord(record, metaData, getter_AddRefs(file));
        if (NS_SUCCEEDED(rv)) {
            rv = file->Remove(PR_FALSE);    
        }
        DecrementTotalSize(sizeK);
        
    } else if (fileIndex < 4) {
        
        PRUint32  startBlock = metaData ? record->MetaStartBlock() : record->DataStartBlock();
        PRUint32  blockCount = metaData ? record->MetaBlockCount() : record->DataBlockCount();
        
        rv = mBlockFile[fileIndex - 1].DeallocateBlocks(startBlock, blockCount);
        DecrementTotalSize(blockCount, GetBlockSizeForIndex(fileIndex));
    }
    if (metaData)  record->ClearMetaLocation();
    else           record->ClearDataLocation();
    
    return rv;
}


nsresult
nsDiskCacheMap::GetFileForDiskCacheRecord(nsDiskCacheRecord * record,
                                          PRBool              meta,
                                          nsIFile **          result)
{
    if (!mCacheDirectory)  return NS_ERROR_NOT_AVAILABLE;
    
    nsCOMPtr<nsIFile> file;
    nsresult rv = mCacheDirectory->Clone(getter_AddRefs(file));
    if (NS_FAILED(rv))  return rv;
    
    PRInt16 generation = record->Generation();
    char name[32];
    ::sprintf(name, "%08X%c%02X", record->HashNumber(),  (meta ? 'm' : 'd'), generation);
    rv = file->AppendNative(nsDependentCString(name));
    if (NS_FAILED(rv))  return rv;
    
    NS_IF_ADDREF(*result = file);
    return rv;
}


nsresult
nsDiskCacheMap::GetLocalFileForDiskCacheRecord(nsDiskCacheRecord * record,
                                               PRBool              meta,
                                               nsILocalFile **     result)
{
    nsCOMPtr<nsIFile> file;
    nsresult rv = GetFileForDiskCacheRecord(record, meta, getter_AddRefs(file));
    if (NS_FAILED(rv))  return rv;
    
    nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(file, &rv);
    if (NS_FAILED(rv))  return rv;
    
    NS_IF_ADDREF(*result = localFile);
    return rv;
}


nsresult
nsDiskCacheMap::GetBlockFileForIndex(PRUint32 index, nsILocalFile ** result)
{
    if (!mCacheDirectory)  return NS_ERROR_NOT_AVAILABLE;
    
    nsCOMPtr<nsIFile> file;
    nsresult rv = mCacheDirectory->Clone(getter_AddRefs(file));
    if (NS_FAILED(rv))  return rv;
    
    char name[32];
    ::sprintf(name, "_CACHE_%03d_", index + 1);
    rv = file->AppendNative(nsDependentCString(name));
    if (NS_FAILED(rv))  return rv;
    
    nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(file, &rv);
    NS_IF_ADDREF(*result = localFile);

    return rv;
}


PRUint32
nsDiskCacheMap::CalculateFileIndex(PRUint32 size)
{
    if (size <=  1024)  return 1;
    if (size <=  4096)  return 2;
    if (size <= 16384)  return 3;
    return 0;  
}


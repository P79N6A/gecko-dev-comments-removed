





#ifndef _nsDiskCacheMap_h_
#define _nsDiskCacheMap_h_

#include <limits.h>

#include "prtypes.h"
#include "prnetdb.h"
#include "nsDebug.h"
#include "nsError.h"
#include "nsIFile.h"
#include "nsITimer.h"

#include "nsDiskCache.h"
#include "nsDiskCacheBlockFile.h"
 
 
class nsDiskCacheBinding;
struct nsDiskCacheEntry;


































#define kNumBlockFiles             3
#define SIZE_SHIFT(idx)            (2 * ((idx) - 1))
#define BLOCK_SIZE_FOR_INDEX(idx)  ((idx) ? (256    << SIZE_SHIFT(idx)) : 0)
#define BITMAP_SIZE_FOR_INDEX(idx) ((idx) ? (131072 >> SIZE_SHIFT(idx)) : 0)


#define kMinRecordCount    512

#define kSeparateFile      0
#define kBuckets           (1 << 5)    // must be a power of 2!








#define kMaxDataSizeK      0xFFFF


#define kPreallocateLimit  1 * 1024 * 1024



#define kRevalidateCacheTimeout 3000
#define kRevalidateCacheTimeoutTolerance 10
#define kRevalidateCacheErrorTimeout 1000

class nsDiskCacheRecord {

private:
    uint32_t    mHashNumber;
    uint32_t    mEvictionRank;
    uint32_t    mDataLocation;
    uint32_t    mMetaLocation;
 
    enum {
        eLocationInitializedMask = 0x80000000,
        
        eLocationSelectorMask    = 0x30000000,
        eLocationSelectorOffset  = 28,
        
        eExtraBlocksMask         = 0x03000000,
        eExtraBlocksOffset       = 24,
        
        eReservedMask            = 0x4C000000,
        
        eBlockNumberMask         = 0x00FFFFFF,

        eFileSizeMask            = 0x00FFFF00,
        eFileSizeOffset          = 8,
        eFileGenerationMask      = 0x000000FF,
        eFileReservedMask        = 0x4F000000
        
    };

public:
    nsDiskCacheRecord()
        :   mHashNumber(0), mEvictionRank(0), mDataLocation(0), mMetaLocation(0)
    {
    }
    
    bool    ValidRecord()
    {
        if ((mDataLocation & eReservedMask) || (mMetaLocation & eReservedMask))
            return false;
        return true;
    }
    
    
    uint32_t  HashNumber() const                  { return mHashNumber; }
    void      SetHashNumber( uint32_t hashNumber) { mHashNumber = hashNumber; }

    
    uint32_t  EvictionRank() const              { return mEvictionRank; }
    void      SetEvictionRank( uint32_t rank)   { mEvictionRank = rank ? rank : 1; }

    
    bool      DataLocationInitialized() const { return 0 != (mDataLocation & eLocationInitializedMask); }
    void      ClearDataLocation()       { mDataLocation = 0; }
    
    uint32_t  DataFile() const
    {
        return (uint32_t)(mDataLocation & eLocationSelectorMask) >> eLocationSelectorOffset;
    }

    void      SetDataBlocks( uint32_t index, uint32_t startBlock, uint32_t blockCount)
    {
        
        mDataLocation = 0;
        
        
        NS_ASSERTION( index < (kNumBlockFiles + 1), "invalid location index");
        NS_ASSERTION( index > 0,"invalid location index");
        mDataLocation |= (index << eLocationSelectorOffset) & eLocationSelectorMask;

        
        NS_ASSERTION(startBlock == (startBlock & eBlockNumberMask), "invalid block number");
        mDataLocation |= startBlock & eBlockNumberMask;
        
        
        NS_ASSERTION( (blockCount>=1) && (blockCount<=4),"invalid block count");
        --blockCount;
        mDataLocation |= (blockCount << eExtraBlocksOffset) & eExtraBlocksMask;
        
        mDataLocation |= eLocationInitializedMask;
    }

    uint32_t   DataBlockCount() const
    {
        return (uint32_t)((mDataLocation & eExtraBlocksMask) >> eExtraBlocksOffset) + 1;
    }

    uint32_t   DataStartBlock() const
    {
        return (mDataLocation & eBlockNumberMask);
    }
    
    uint32_t   DataBlockSize() const
    {
        return BLOCK_SIZE_FOR_INDEX(DataFile());
    }
    
    uint32_t   DataFileSize() const  { return (mDataLocation & eFileSizeMask) >> eFileSizeOffset; }
    void       SetDataFileSize(uint32_t  size)
    {
        NS_ASSERTION((mDataLocation & eFileReservedMask) == 0, "bad location");
        mDataLocation &= ~eFileSizeMask;    
        mDataLocation |= (size << eFileSizeOffset) & eFileSizeMask;
    }

    uint8_t   DataFileGeneration() const
    {
        return (mDataLocation & eFileGenerationMask);
    }

    void       SetDataFileGeneration( uint8_t generation)
    {
        
        mDataLocation = 0;
        mDataLocation |= generation & eFileGenerationMask;
        mDataLocation |= eLocationInitializedMask;
    }

    
    bool      MetaLocationInitialized() const { return 0 != (mMetaLocation & eLocationInitializedMask); }
    void      ClearMetaLocation()             { mMetaLocation = 0; }   
    uint32_t  MetaLocation() const            { return mMetaLocation; }
    
    uint32_t  MetaFile() const
    {
        return (uint32_t)(mMetaLocation & eLocationSelectorMask) >> eLocationSelectorOffset;
    }

    void      SetMetaBlocks( uint32_t index, uint32_t startBlock, uint32_t blockCount)
    {
        
        mMetaLocation = 0;
        
        
        NS_ASSERTION( index < (kNumBlockFiles + 1), "invalid location index");
        NS_ASSERTION( index > 0, "invalid location index");
        mMetaLocation |= (index << eLocationSelectorOffset) & eLocationSelectorMask;

        
        NS_ASSERTION(startBlock == (startBlock & eBlockNumberMask), "invalid block number");
        mMetaLocation |= startBlock & eBlockNumberMask;
        
        
        NS_ASSERTION( (blockCount>=1) && (blockCount<=4),"invalid block count");
        --blockCount;
        mMetaLocation |= (blockCount << eExtraBlocksOffset) & eExtraBlocksMask;
        
        mMetaLocation |= eLocationInitializedMask;
    }

    uint32_t   MetaBlockCount() const
    {
        return (uint32_t)((mMetaLocation & eExtraBlocksMask) >> eExtraBlocksOffset) + 1;
    }

    uint32_t   MetaStartBlock() const
    {
        return (mMetaLocation & eBlockNumberMask);
    }

    uint32_t   MetaBlockSize() const
    {
        return BLOCK_SIZE_FOR_INDEX(MetaFile());
    }
    
    uint32_t   MetaFileSize() const  { return (mMetaLocation & eFileSizeMask) >> eFileSizeOffset; }
    void       SetMetaFileSize(uint32_t  size)
    {
        mMetaLocation &= ~eFileSizeMask;    
        mMetaLocation |= (size << eFileSizeOffset) & eFileSizeMask;
    }

    uint8_t   MetaFileGeneration() const
    {
        return (mMetaLocation & eFileGenerationMask);
    }

    void       SetMetaFileGeneration( uint8_t generation)
    {
        
        mMetaLocation = 0;
        mMetaLocation |= generation & eFileGenerationMask;
        mMetaLocation |= eLocationInitializedMask;
    }

    uint8_t   Generation() const
    {
        if ((mDataLocation & eLocationInitializedMask)  &&
            (DataFile() == 0))
            return DataFileGeneration();
            
        if ((mMetaLocation & eLocationInitializedMask)  &&
            (MetaFile() == 0))
            return MetaFileGeneration();
        
        return 0;  
    }

#if defined(IS_LITTLE_ENDIAN)
    void        Swap()
    {
        mHashNumber   = htonl(mHashNumber);
        mEvictionRank = htonl(mEvictionRank);
        mDataLocation = htonl(mDataLocation);
        mMetaLocation = htonl(mMetaLocation);
    }
#endif
    
#if defined(IS_LITTLE_ENDIAN)
    void        Unswap()
    {
        mHashNumber   = ntohl(mHashNumber);
        mEvictionRank = ntohl(mEvictionRank);
        mDataLocation = ntohl(mDataLocation);
        mMetaLocation = ntohl(mMetaLocation);
    }
#endif

};






enum {  kDeleteRecordAndContinue = -1,
        kStopVisitingRecords     =  0,
        kVisitNextRecord         =  1
};

class nsDiskCacheRecordVisitor {
    public:

    virtual int32_t  VisitRecord( nsDiskCacheRecord *  mapRecord) = 0;
};






struct nsDiskCacheHeader {
    uint32_t    mVersion;                           
    uint32_t    mDataSize;                          
    int32_t     mEntryCount;                        
    uint32_t    mIsDirty;                           
    int32_t     mRecordCount;                       
    uint32_t    mEvictionRank[kBuckets];            
    uint32_t    mBucketUsage[kBuckets];             
  
    nsDiskCacheHeader()
        : mVersion(nsDiskCache::kCurrentVersion)
        , mDataSize(0)
        , mEntryCount(0)
        , mIsDirty(true)
        , mRecordCount(0)
    {}

    void        Swap()
    {
#if defined(IS_LITTLE_ENDIAN)
        mVersion     = htonl(mVersion);
        mDataSize    = htonl(mDataSize);
        mEntryCount  = htonl(mEntryCount);
        mIsDirty     = htonl(mIsDirty);
        mRecordCount = htonl(mRecordCount);

        for (uint32_t i = 0; i < kBuckets ; i++) {
            mEvictionRank[i] = htonl(mEvictionRank[i]);
            mBucketUsage[i]  = htonl(mBucketUsage[i]);
        }
#endif
    }
    
    void        Unswap()
    {
#if defined(IS_LITTLE_ENDIAN)
        mVersion     = ntohl(mVersion);
        mDataSize    = ntohl(mDataSize);
        mEntryCount  = ntohl(mEntryCount);
        mIsDirty     = ntohl(mIsDirty);
        mRecordCount = ntohl(mRecordCount);

        for (uint32_t i = 0; i < kBuckets ; i++) {
            mEvictionRank[i] = ntohl(mEvictionRank[i]);
            mBucketUsage[i]  = ntohl(mBucketUsage[i]);
        }
#endif
    }
};






class nsDiskCacheMap {
public:

     nsDiskCacheMap() : 
        mCacheDirectory(nullptr),
        mMapFD(nullptr),
        mCleanFD(nullptr),
        mRecordArray(nullptr),
        mBufferSize(0),
        mBuffer(nullptr),
        mMaxRecordCount(16384), 
        mIsDirtyCacheFlushed(false),
        mLastInvalidateTime(0)
    { }

    ~nsDiskCacheMap()
    {
        (void) Close(true);
    }









    nsresult  Open( nsIFile *  cacheDirectory,
                    nsDiskCache::CorruptCacheInfo *  corruptInfo,
                    bool reportCacheCleanTelemetryData);
    nsresult  Close(bool flush);
    nsresult  Trim();

    nsresult  FlushHeader();
    nsresult  FlushRecords( bool unswap);

    void      NotifyCapacityChange(uint32_t capacity);




    nsresult AddRecord( nsDiskCacheRecord *  mapRecord, nsDiskCacheRecord * oldRecord);
    nsresult UpdateRecord( nsDiskCacheRecord *  mapRecord);
    nsresult FindRecord( uint32_t  hashNumber, nsDiskCacheRecord *  mapRecord);
    nsresult DeleteRecord( nsDiskCacheRecord *  mapRecord);
    nsresult VisitRecords( nsDiskCacheRecordVisitor * visitor);
    nsresult EvictRecords( nsDiskCacheRecordVisitor * visitor);




    nsresult    DeleteStorage( nsDiskCacheRecord *  record);

    nsresult    GetFileForDiskCacheRecord( nsDiskCacheRecord * record,
                                           bool                meta,
                                           bool                createPath,
                                           nsIFile **          result);
                                          
    nsresult    GetLocalFileForDiskCacheRecord( nsDiskCacheRecord *  record,
                                                bool                 meta,
                                                bool                 createPath,
                                                nsIFile **           result);

    
    
    nsDiskCacheEntry * ReadDiskCacheEntry( nsDiskCacheRecord *  record);

    nsresult    WriteDiskCacheEntry( nsDiskCacheBinding *  binding);
    
    nsresult    ReadDataCacheBlocks(nsDiskCacheBinding * binding, char * buffer, uint32_t size);
    nsresult    WriteDataCacheBlocks(nsDiskCacheBinding * binding, char * buffer, uint32_t size);
    nsresult    DeleteStorage( nsDiskCacheRecord * record, bool metaData);
    
    


    void     IncrementTotalSize( uint32_t  delta)
             {
                mHeader.mDataSize += delta;
                mHeader.mIsDirty   = true;
             }
             
    void     DecrementTotalSize( uint32_t  delta)
             {
                NS_ASSERTION(mHeader.mDataSize >= delta, "disk cache size negative?");
                mHeader.mDataSize  = mHeader.mDataSize > delta ? mHeader.mDataSize - delta : 0;               
                mHeader.mIsDirty   = true;
             }
    
    inline void IncrementTotalSize( uint32_t  blocks, uint32_t blockSize)
             {
                
                IncrementTotalSize(((blocks*blockSize) + 0x03FF) >> 10);
             }

    inline void DecrementTotalSize( uint32_t  blocks, uint32_t blockSize)
             {
                
                DecrementTotalSize(((blocks*blockSize) + 0x03FF) >> 10);
             }
                 
    uint32_t TotalSize()   { return mHeader.mDataSize; }
    
    int32_t  EntryCount()  { return mHeader.mEntryCount; }


private:

    


    nsresult    OpenBlockFiles(nsDiskCache::CorruptCacheInfo *  corruptInfo);
    nsresult    CloseBlockFiles(bool flush);
    bool        CacheFilesExist();

    nsresult    CreateCacheSubDirectories();

    uint32_t    CalculateFileIndex(uint32_t size);

    nsresult    GetBlockFileForIndex( uint32_t index, nsIFile ** result);
    uint32_t    GetBlockSizeForIndex( uint32_t index) const {
        return BLOCK_SIZE_FOR_INDEX(index);
    }
    uint32_t    GetBitMapSizeForIndex( uint32_t index) const {
        return BITMAP_SIZE_FOR_INDEX(index);
    }
    
    
    uint32_t GetBucketIndex( uint32_t hashNumber) const {
        return (hashNumber & (kBuckets - 1));
    }
    
    
    uint32_t GetRecordsPerBucket() const {
        return mHeader.mRecordCount / kBuckets;
    }

    
    nsDiskCacheRecord *GetFirstRecordInBucket(uint32_t bucket) const {
        return mRecordArray + bucket * GetRecordsPerBucket();
    }

    uint32_t GetBucketRank(uint32_t bucketIndex, uint32_t targetRank);

    int32_t  VisitEachRecord(uint32_t                    bucketIndex,
                             nsDiskCacheRecordVisitor *  visitor,
                             uint32_t                    evictionRank);

    nsresult GrowRecords();
    nsresult ShrinkRecords();

    nsresult EnsureBuffer(uint32_t bufSize);

    
    
    nsDiskCacheEntry *  CreateDiskCacheEntry(nsDiskCacheBinding *  binding,
                                             uint32_t * size);

    
    nsresult InitCacheClean(nsIFile *  cacheDirectory,
                            nsDiskCache::CorruptCacheInfo *  corruptInfo,
                            bool reportCacheCleanTelemetryData);
    
    nsresult WriteCacheClean(bool clean);
    
    nsresult ResetCacheTimer(int32_t timeout = kRevalidateCacheTimeout);
    
    nsresult InvalidateCache();
    
    bool IsCacheInSafeState();
    
    
    nsresult RevalidateCache();
    
    static void RevalidateTimerCallback(nsITimer *aTimer, void *arg);




private:
    nsCOMPtr<nsITimer>      mCleanCacheTimer;
    nsCOMPtr<nsIFile>       mCacheDirectory;
    PRFileDesc *            mMapFD;
    PRFileDesc *            mCleanFD;
    nsDiskCacheRecord *     mRecordArray;
    nsDiskCacheBlockFile    mBlockFile[kNumBlockFiles];
    uint32_t                mBufferSize;
    char *                  mBuffer;
    nsDiskCacheHeader       mHeader;
    int32_t                 mMaxRecordCount;
    bool                    mIsDirtyCacheFlushed;
    PRIntervalTime          mLastInvalidateTime;
};

#endif 

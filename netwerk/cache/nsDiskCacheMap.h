





#ifndef _nsDiskCacheMap_h_
#define _nsDiskCacheMap_h_

#include <limits.h>

#include "prtypes.h"
#include "prnetdb.h"
#include "nsDebug.h"
#include "nsError.h"
#include "nsIFile.h"

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

class nsDiskCacheRecord {

private:
    PRUint32    mHashNumber;
    PRUint32    mEvictionRank;
    PRUint32    mDataLocation;
    PRUint32    mMetaLocation;
 
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
    
    
    PRUint32  HashNumber() const                  { return mHashNumber; }
    void      SetHashNumber( PRUint32 hashNumber) { mHashNumber = hashNumber; }

    
    PRUint32  EvictionRank() const              { return mEvictionRank; }
    void      SetEvictionRank( PRUint32 rank)   { mEvictionRank = rank ? rank : 1; }

    
    bool      DataLocationInitialized() const { return 0 != (mDataLocation & eLocationInitializedMask); }
    void      ClearDataLocation()       { mDataLocation = 0; }
    
    PRUint32  DataFile() const
    {
        return (PRUint32)(mDataLocation & eLocationSelectorMask) >> eLocationSelectorOffset;
    }

    void      SetDataBlocks( PRUint32 index, PRUint32 startBlock, PRUint32 blockCount)
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

    PRUint32   DataBlockCount() const
    {
        return (PRUint32)((mDataLocation & eExtraBlocksMask) >> eExtraBlocksOffset) + 1;
    }

    PRUint32   DataStartBlock() const
    {
        return (mDataLocation & eBlockNumberMask);
    }
    
    PRUint32   DataBlockSize() const
    {
        return BLOCK_SIZE_FOR_INDEX(DataFile());
    }
    
    PRUint32   DataFileSize() const  { return (mDataLocation & eFileSizeMask) >> eFileSizeOffset; }
    void       SetDataFileSize(PRUint32  size)
    {
        NS_ASSERTION((mDataLocation & eFileReservedMask) == 0, "bad location");
        mDataLocation &= ~eFileSizeMask;    
        mDataLocation |= (size << eFileSizeOffset) & eFileSizeMask;
    }

    PRUint8   DataFileGeneration() const
    {
        return (mDataLocation & eFileGenerationMask);
    }

    void       SetDataFileGeneration( PRUint8 generation)
    {
        
        mDataLocation = 0;
        mDataLocation |= generation & eFileGenerationMask;
        mDataLocation |= eLocationInitializedMask;
    }

    
    bool      MetaLocationInitialized() const { return 0 != (mMetaLocation & eLocationInitializedMask); }
    void      ClearMetaLocation()             { mMetaLocation = 0; }   
    PRUint32  MetaLocation() const            { return mMetaLocation; }
    
    PRUint32  MetaFile() const
    {
        return (PRUint32)(mMetaLocation & eLocationSelectorMask) >> eLocationSelectorOffset;
    }

    void      SetMetaBlocks( PRUint32 index, PRUint32 startBlock, PRUint32 blockCount)
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

    PRUint32   MetaBlockCount() const
    {
        return (PRUint32)((mMetaLocation & eExtraBlocksMask) >> eExtraBlocksOffset) + 1;
    }

    PRUint32   MetaStartBlock() const
    {
        return (mMetaLocation & eBlockNumberMask);
    }

    PRUint32   MetaBlockSize() const
    {
        return BLOCK_SIZE_FOR_INDEX(MetaFile());
    }
    
    PRUint32   MetaFileSize() const  { return (mMetaLocation & eFileSizeMask) >> eFileSizeOffset; }
    void       SetMetaFileSize(PRUint32  size)
    {
        mMetaLocation &= ~eFileSizeMask;    
        mMetaLocation |= (size << eFileSizeOffset) & eFileSizeMask;
    }

    PRUint8   MetaFileGeneration() const
    {
        return (mMetaLocation & eFileGenerationMask);
    }

    void       SetMetaFileGeneration( PRUint8 generation)
    {
        
        mMetaLocation = 0;
        mMetaLocation |= generation & eFileGenerationMask;
        mMetaLocation |= eLocationInitializedMask;
    }

    PRUint8   Generation() const
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

    virtual PRInt32  VisitRecord( nsDiskCacheRecord *  mapRecord) = 0;
};






struct nsDiskCacheHeader {
    PRUint32    mVersion;                           
    PRUint32    mDataSize;                          
    PRInt32     mEntryCount;                        
    PRUint32    mIsDirty;                           
    PRInt32     mRecordCount;                       
    PRUint32    mEvictionRank[kBuckets];            
    PRUint32    mBucketUsage[kBuckets];             
  
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

        for (PRUint32 i = 0; i < kBuckets ; i++) {
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

        for (PRUint32 i = 0; i < kBuckets ; i++) {
            mEvictionRank[i] = ntohl(mEvictionRank[i]);
            mBucketUsage[i]  = ntohl(mBucketUsage[i]);
        }
#endif
    }
};






class nsDiskCacheMap {
public:

     nsDiskCacheMap() : 
        mCacheDirectory(nsnull),
        mMapFD(nsnull),
        mRecordArray(nsnull),
        mBufferSize(0),
        mBuffer(nsnull),
        mMaxRecordCount(16384) 
    { }

    ~nsDiskCacheMap() {
        (void) Close(true);
    }









    nsresult  Open( nsIFile *  cacheDirectory);
    nsresult  Close(bool flush);
    nsresult  Trim();

    nsresult  FlushHeader();
    nsresult  FlushRecords( bool unswap);

    void      NotifyCapacityChange(PRUint32 capacity);




    nsresult AddRecord( nsDiskCacheRecord *  mapRecord, nsDiskCacheRecord * oldRecord);
    nsresult UpdateRecord( nsDiskCacheRecord *  mapRecord);
    nsresult FindRecord( PRUint32  hashNumber, nsDiskCacheRecord *  mapRecord);
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
    
    nsresult    ReadDataCacheBlocks(nsDiskCacheBinding * binding, char * buffer, PRUint32 size);
    nsresult    WriteDataCacheBlocks(nsDiskCacheBinding * binding, char * buffer, PRUint32 size);
    nsresult    DeleteStorage( nsDiskCacheRecord * record, bool metaData);
    
    


    void     IncrementTotalSize( PRUint32  delta)
             {
                mHeader.mDataSize += delta;
                mHeader.mIsDirty   = true;
             }
             
    void     DecrementTotalSize( PRUint32  delta)
             {
                NS_ASSERTION(mHeader.mDataSize >= delta, "disk cache size negative?");
                mHeader.mDataSize  = mHeader.mDataSize > delta ? mHeader.mDataSize - delta : 0;               
                mHeader.mIsDirty   = true;
             }
    
    inline void IncrementTotalSize( PRUint32  blocks, PRUint32 blockSize)
             {
                
                IncrementTotalSize(((blocks*blockSize) + 0x03FF) >> 10);
             }

    inline void DecrementTotalSize( PRUint32  blocks, PRUint32 blockSize)
             {
                
                DecrementTotalSize(((blocks*blockSize) + 0x03FF) >> 10);
             }
                 
    PRUint32 TotalSize()   { return mHeader.mDataSize; }
    
    PRInt32  EntryCount()  { return mHeader.mEntryCount; }


private:

    


    nsresult    OpenBlockFiles();
    nsresult    CloseBlockFiles(bool flush);
    bool        CacheFilesExist();

    nsresult    CreateCacheSubDirectories();

    PRUint32    CalculateFileIndex(PRUint32 size);

    nsresult    GetBlockFileForIndex( PRUint32 index, nsIFile ** result);
    PRUint32    GetBlockSizeForIndex( PRUint32 index) const {
        return BLOCK_SIZE_FOR_INDEX(index);
    }
    PRUint32    GetBitMapSizeForIndex( PRUint32 index) const {
        return BITMAP_SIZE_FOR_INDEX(index);
    }
    
    
    PRUint32 GetBucketIndex( PRUint32 hashNumber) const {
        return (hashNumber & (kBuckets - 1));
    }
    
    
    PRUint32 GetRecordsPerBucket() const {
        return mHeader.mRecordCount / kBuckets;
    }

    
    nsDiskCacheRecord *GetFirstRecordInBucket(PRUint32 bucket) const {
        return mRecordArray + bucket * GetRecordsPerBucket();
    }

    PRUint32 GetBucketRank(PRUint32 bucketIndex, PRUint32 targetRank);

    PRInt32  VisitEachRecord(PRUint32                    bucketIndex,
                             nsDiskCacheRecordVisitor *  visitor,
                             PRUint32                    evictionRank);

    nsresult GrowRecords();
    nsresult ShrinkRecords();

    nsresult EnsureBuffer(PRUint32 bufSize);

    
    
    nsDiskCacheEntry *  CreateDiskCacheEntry(nsDiskCacheBinding *  binding,
                                             PRUint32 * size);




private:
    nsCOMPtr<nsIFile>       mCacheDirectory;
    PRFileDesc *            mMapFD;
    nsDiskCacheRecord *     mRecordArray;
    nsDiskCacheBlockFile    mBlockFile[kNumBlockFiles];
    PRUint32                mBufferSize;
    char *                  mBuffer;
    nsDiskCacheHeader       mHeader;
    PRInt32                 mMaxRecordCount;
};

#endif 

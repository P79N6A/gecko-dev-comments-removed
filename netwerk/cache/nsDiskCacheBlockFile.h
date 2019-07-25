





#ifndef _nsDiskCacheBlockFile_h_
#define _nsDiskCacheBlockFile_h_

#include "nsIFile.h"
#include "nsDiskCache.h"









class nsDiskCacheBlockFile {
public:
    nsDiskCacheBlockFile()
           : mFD(nullptr)
           , mBitMap(nullptr)
           , mBlockSize(0)
           , mBitMapWords(0)
           , mFileSize(0)
           , mBitMapDirty(false)
            {}
    ~nsDiskCacheBlockFile() { (void) Close(true); }
    
    nsresult  Open( nsIFile *  blockFile, uint32_t  blockSize,
                    uint32_t  bitMapSize, nsDiskCache::CorruptCacheInfo *  corruptInfo);
    nsresult  Close(bool flush);

    



    nsresult  Trim() { return nsDiskCache::Truncate(mFD, CalcBlockFileSize()); }
    nsresult  DeallocateBlocks( int32_t  startBlock, int32_t  numBlocks);
    nsresult  WriteBlocks( void * buffer, uint32_t size, int32_t  numBlocks, 
                           int32_t * startBlock);
    nsresult  ReadBlocks( void * buffer, int32_t  startBlock, int32_t  numBlocks, 
                          int32_t * bytesRead);
    
private:
    nsresult  FlushBitMap();
    int32_t   AllocateBlocks( int32_t  numBlocks);
    nsresult  VerifyAllocation( int32_t startBlock, int32_t numBLocks);
    uint32_t  CalcBlockFileSize();
    bool   Write(int32_t offset, const void *buf, int32_t amount);




    PRFileDesc *                mFD;
    uint32_t *                  mBitMap;      
    uint32_t                    mBlockSize;
    uint32_t                    mBitMapWords;
    int32_t                     mFileSize;
    bool                        mBitMapDirty;
};

#endif 

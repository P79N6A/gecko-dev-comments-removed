





#include "nsCache.h"
#include "nsDiskCache.h"
#include "nsDiskCacheBlockFile.h"
#include "mozilla/FileUtils.h"
#include <algorithm>

using namespace mozilla;








nsresult
nsDiskCacheBlockFile::Open(nsIFile * blockFile,
                           uint32_t  blockSize,
                           uint32_t  bitMapSize,
                           nsDiskCache::CorruptCacheInfo *  corruptInfo)
{
    NS_ENSURE_ARG_POINTER(corruptInfo);
    *corruptInfo = nsDiskCache::kUnexpectedError;

    if (bitMapSize % 32) {
        *corruptInfo = nsDiskCache::kInvalidArgPointer;
        return NS_ERROR_INVALID_ARG;
    }

    mBlockSize = blockSize;
    mBitMapWords = bitMapSize / 32;
    uint32_t bitMapBytes = mBitMapWords * 4;
    
    
    nsresult rv = blockFile->OpenNSPRFileDesc(PR_RDWR | PR_CREATE_FILE, 00600, &mFD);
    if (NS_FAILED(rv)) {
        *corruptInfo = nsDiskCache::kCouldNotCreateBlockFile;
        CACHE_LOG_DEBUG(("CACHE: nsDiskCacheBlockFile::Open "
                         "[this=%p] unable to open or create file: %d",
                         this, rv));
        return rv;  
    }
    
    
    mBitMap = new uint32_t[mBitMapWords];
    
    
    mFileSize = PR_Available(mFD);
    if (mFileSize < 0) {
        
        *corruptInfo = nsDiskCache::kBlockFileSizeError;
        rv = NS_ERROR_UNEXPECTED;
        goto error_exit;
    }
    if (mFileSize == 0) {
        
        memset(mBitMap, 0, bitMapBytes);
        if (!Write(0, mBitMap, bitMapBytes)) {
            *corruptInfo = nsDiskCache::kBlockFileBitMapWriteError;
            goto error_exit;
        }
        
    } else if ((uint32_t)mFileSize < bitMapBytes) {
        *corruptInfo = nsDiskCache::kBlockFileSizeLessThanBitMap;
        rv = NS_ERROR_UNEXPECTED;  
        goto error_exit;
        
    } else {
        
        const int32_t bytesRead = PR_Read(mFD, mBitMap, bitMapBytes);
        if ((bytesRead < 0) || ((uint32_t)bytesRead < bitMapBytes)) {
            *corruptInfo = nsDiskCache::kBlockFileBitMapReadError;
            rv = NS_ERROR_UNEXPECTED;
            goto error_exit;
        }
#if defined(IS_LITTLE_ENDIAN)
        
        for (unsigned int i = 0; i < mBitMapWords; ++i)
            mBitMap[i] = ntohl(mBitMap[i]);
#endif
        
        
        
        
        const uint32_t  estimatedSize = CalcBlockFileSize();
        if ((uint32_t)mFileSize + blockSize < estimatedSize) {
            *corruptInfo = nsDiskCache::kBlockFileEstimatedSizeError;
            rv = NS_ERROR_UNEXPECTED;
            goto error_exit;
        }
    }
    CACHE_LOG_DEBUG(("CACHE: nsDiskCacheBlockFile::Open [this=%p] succeeded",
                      this));
    return NS_OK;

error_exit:
    CACHE_LOG_DEBUG(("CACHE: nsDiskCacheBlockFile::Open [this=%p] failed with "
                     "error %d", this, rv));
    Close(false);
    return rv;
}





nsresult
nsDiskCacheBlockFile::Close(bool flush)
{
    nsresult rv = NS_OK;

    if (mFD) {
        if (flush)
            rv  = FlushBitMap();
        PRStatus err = PR_Close(mFD);
        if (NS_SUCCEEDED(rv) && (err != PR_SUCCESS))
            rv = NS_ERROR_UNEXPECTED;
        mFD = nullptr;
    }

     if (mBitMap) {
         delete [] mBitMap;
         mBitMap = nullptr;
     }
        
    return rv;
}











int32_t
nsDiskCacheBlockFile::AllocateBlocks(int32_t numBlocks)
{
    const int maxPos = 32 - numBlocks;
    const uint32_t mask = (0x01 << numBlocks) - 1;
    for (unsigned int i = 0; i < mBitMapWords; ++i) {
        uint32_t mapWord = ~mBitMap[i]; 
        if (mapWord) {                  
            
            int bit = 0;
            if ((mapWord & 0x0FFFF) == 0) { bit |= 16; mapWord >>= 16; }
            if ((mapWord & 0x000FF) == 0) { bit |= 8;  mapWord >>= 8;  }
            if ((mapWord & 0x0000F) == 0) { bit |= 4;  mapWord >>= 4;  }
            if ((mapWord & 0x00003) == 0) { bit |= 2;  mapWord >>= 2;  }
            if ((mapWord & 0x00001) == 0) { bit |= 1;  mapWord >>= 1;  }
            
            for (; bit <= maxPos; ++bit) {
                
                if ((mask & mapWord) == mask) {
                    mBitMap[i] |= mask << bit; 
                    mBitMapDirty = true;
                    return (int32_t)i * 32 + bit;
                }
            }
        }
    }
    
    return -1;
}





nsresult
nsDiskCacheBlockFile::DeallocateBlocks( int32_t  startBlock, int32_t  numBlocks)
{
    if (!mFD)  return NS_ERROR_NOT_AVAILABLE;

    if ((startBlock < 0) || ((uint32_t)startBlock > mBitMapWords * 32 - 1) ||
        (numBlocks < 1)  || (numBlocks > 4))
       return NS_ERROR_ILLEGAL_VALUE;
           
    const int32_t startWord = startBlock >> 5;      
    const uint32_t startBit = startBlock & 31;      
      
    
    if (startBit + numBlocks > 32)  return NS_ERROR_UNEXPECTED;
    uint32_t mask = ((0x01 << numBlocks) - 1) << startBit;
    
    
    if ((mBitMap[startWord] & mask) != mask)    return NS_ERROR_ABORT;

    mBitMap[startWord] ^= mask;    
    mBitMapDirty = true;
    
    return NS_OK;
}





nsresult
nsDiskCacheBlockFile::WriteBlocks( void *   buffer,
                                   uint32_t size,
                                   int32_t  numBlocks,
                                   int32_t * startBlock)
{
    
    NS_ENSURE_TRUE(mFD, NS_ERROR_NOT_AVAILABLE);

    
    *startBlock = AllocateBlocks(numBlocks);
    if (*startBlock < 0)
        return NS_ERROR_NOT_AVAILABLE;

    
    int32_t blockPos = mBitMapWords * 4 + *startBlock * mBlockSize;
    
    
    return Write(blockPos, buffer, size) ? NS_OK : NS_ERROR_FAILURE;
}





nsresult
nsDiskCacheBlockFile::ReadBlocks( void *    buffer,
                                  int32_t   startBlock,
                                  int32_t   numBlocks,
                                  int32_t * bytesRead)
{
    

    if (!mFD)  return NS_ERROR_NOT_AVAILABLE;
    nsresult rv = VerifyAllocation(startBlock, numBlocks);
    if (NS_FAILED(rv))  return rv;
    
    
    int32_t blockPos = mBitMapWords * 4 + startBlock * mBlockSize;
    int32_t filePos = PR_Seek(mFD, blockPos, PR_SEEK_SET);
    if (filePos != blockPos)  return NS_ERROR_UNEXPECTED;

    
    int32_t bytesToRead = *bytesRead;
    if ((bytesToRead <= 0) || ((uint32_t)bytesToRead > mBlockSize * numBlocks)) {
        bytesToRead = mBlockSize * numBlocks;
    }
    *bytesRead = PR_Read(mFD, buffer, bytesToRead);
    
    CACHE_LOG_DEBUG(("CACHE: nsDiskCacheBlockFile::Read [this=%p] "
                     "returned %d / %d bytes", this, *bytesRead, bytesToRead));

    return NS_OK;
}





nsresult
nsDiskCacheBlockFile::FlushBitMap()
{
    if (!mBitMapDirty)  return NS_OK;
    
#if defined(IS_LITTLE_ENDIAN)
    uint32_t *bitmap = new uint32_t[mBitMapWords];
    
    uint32_t *p = bitmap;
    for (unsigned int i = 0; i < mBitMapWords; ++i, ++p)
      *p = htonl(mBitMap[i]);
#else
    uint32_t *bitmap = mBitMap;
#endif

    
    bool written = Write(0, bitmap, mBitMapWords * 4);
#if defined(IS_LITTLE_ENDIAN)
    delete [] bitmap;
#endif
    if (!written)
        return NS_ERROR_UNEXPECTED;

    PRStatus err = PR_Sync(mFD);
    if (err != PR_SUCCESS)  return NS_ERROR_UNEXPECTED;

    mBitMapDirty = false;
    return NS_OK;
}











nsresult
nsDiskCacheBlockFile::VerifyAllocation( int32_t  startBlock, int32_t  numBlocks)
{
    if ((startBlock < 0) || ((uint32_t)startBlock > mBitMapWords * 32 - 1) ||
        (numBlocks < 1)  || (numBlocks > 4))
       return NS_ERROR_ILLEGAL_VALUE;
    
    const int32_t startWord = startBlock >> 5;      
    const uint32_t startBit = startBlock & 31;      
      
    
    if (startBit + numBlocks > 32)  return NS_ERROR_ILLEGAL_VALUE;
    uint32_t mask = ((0x01 << numBlocks) - 1) << startBit;
    
    
    if ((mBitMap[startWord] & mask) != mask)    return NS_ERROR_FAILURE;
    
    return NS_OK;
}








uint32_t
nsDiskCacheBlockFile::CalcBlockFileSize()
{
    
    uint32_t  estimatedSize = mBitMapWords * 4;
    int32_t   i = mBitMapWords;
    while (--i >= 0) {
        if (mBitMap[i]) break;
    }

    if (i >= 0) {
        
        uint32_t mapWord = mBitMap[i];
        uint32_t lastBit = 31;
        if ((mapWord & 0xFFFF0000) == 0) { lastBit ^= 16; mapWord <<= 16; }
        if ((mapWord & 0xFF000000) == 0) { lastBit ^= 8; mapWord <<= 8; }
        if ((mapWord & 0xF0000000) == 0) { lastBit ^= 4; mapWord <<= 4; }
        if ((mapWord & 0xC0000000) == 0) { lastBit ^= 2; mapWord <<= 2; }
        if ((mapWord & 0x80000000) == 0) { lastBit ^= 1; mapWord <<= 1; }
        estimatedSize +=  (i * 32 + lastBit + 1) * mBlockSize;
    }

    return estimatedSize;
}







bool
nsDiskCacheBlockFile::Write(int32_t offset, const void *buf, int32_t amount)
{
    



    const int32_t upTo = offset + amount;
    
    const int32_t minPreallocate = 4*1024*1024;
    const int32_t maxPreallocate = 20*1000*1000;
    if (mFileSize < upTo) {
        
        const int32_t maxFileSize = mBitMapWords * 4 * (mBlockSize * 8 + 1);
        if (upTo > maxPreallocate) {
            
            mFileSize = ((upTo + minPreallocate - 1) / minPreallocate) * minPreallocate;
        } else {
            
            if (mFileSize)
                while(mFileSize < upTo)
                    mFileSize *= 2;
            mFileSize = clamped(mFileSize, minPreallocate, maxPreallocate);
        }
        mFileSize = std::min(mFileSize, maxFileSize);
        
        
    }
    if (PR_Seek(mFD, offset, PR_SEEK_SET) != offset)
        return false;
    return PR_Write(mFD, buf, amount) == amount;
}

size_t
nsDiskCacheBlockFile::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf)
{
    return aMallocSizeOf(mBitMap) + aMallocSizeOf(mFD);
}

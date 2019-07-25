







































#include "nsDiskCache.h"
#include "nsDiskCacheBlockFile.h"
#include "mozilla/FileUtils.h"








nsresult
nsDiskCacheBlockFile::Open(nsILocalFile * blockFile,
                           PRUint32       blockSize,
                           PRUint32       bitMapSize)
{
    if (bitMapSize % 32)
        return NS_ERROR_INVALID_ARG;

    mBlockSize = blockSize;
    mBitMapWords = bitMapSize / 32;
    PRUint32 bitMapBytes = mBitMapWords * 4;
    
    
    nsresult rv = blockFile->OpenNSPRFileDesc(PR_RDWR | PR_CREATE_FILE, 00600, &mFD);
    if (NS_FAILED(rv))  return rv;  
    
    
    mBitMap = new PRUint32[mBitMapWords];
    if (!mBitMap) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        goto error_exit;
    }
    
    
    mFileSize = PR_Available(mFD);
    if (mFileSize < 0) {
        
        rv = NS_ERROR_UNEXPECTED;
        goto error_exit;
    }
    if (mFileSize == 0) {
        
        memset(mBitMap, 0, bitMapBytes);
        if (!Write(0, mBitMap, bitMapBytes))
            goto error_exit;
        
    } else if ((PRUint32)mFileSize < bitMapBytes) {
        rv = NS_ERROR_UNEXPECTED;  
        goto error_exit;
        
    } else {
        
        const PRInt32 bytesRead = PR_Read(mFD, mBitMap, bitMapBytes);
        if ((bytesRead < 0) || ((PRUint32)bytesRead < bitMapBytes)) {
            rv = NS_ERROR_UNEXPECTED;
            goto error_exit;
        }
#if defined(IS_LITTLE_ENDIAN)
        
        for (unsigned int i = 0; i < mBitMapWords; ++i)
            mBitMap[i] = ntohl(mBitMap[i]);
#endif
        
        
        
        
        const PRUint32  estimatedSize = CalcBlockFileSize();
        if ((PRUint32)mFileSize + blockSize < estimatedSize) {
            rv = NS_ERROR_UNEXPECTED;
            goto error_exit;
        }
    }
    return NS_OK;

error_exit:
    Close(PR_FALSE);
    return rv;
}





nsresult
nsDiskCacheBlockFile::Close(PRBool flush)
{
    nsresult rv = NS_OK;

    if (mFD) {
        if (flush)
            rv  = FlushBitMap();
        PRStatus err = PR_Close(mFD);
        if (NS_SUCCEEDED(rv) && (err != PR_SUCCESS))
            rv = NS_ERROR_UNEXPECTED;
        mFD = nsnull;
    }

     if (mBitMap) {
         delete [] mBitMap;
         mBitMap = nsnull;
     }
        
    return rv;
}











PRInt32
nsDiskCacheBlockFile::AllocateBlocks(PRInt32 numBlocks)
{
    const int maxPos = 32 - numBlocks;
    const PRUint32 mask = (0x01 << numBlocks) - 1;
    for (unsigned int i = 0; i < mBitMapWords; ++i) {
        PRUint32 mapWord = ~mBitMap[i]; 
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
                    mBitMapDirty = PR_TRUE;
                    return (PRInt32)i * 32 + bit;
                }
            }
        }
    }
    
    return -1;
}





nsresult
nsDiskCacheBlockFile::DeallocateBlocks( PRInt32  startBlock, PRInt32  numBlocks)
{
    if (!mFD)  return NS_ERROR_NOT_AVAILABLE;

    if ((startBlock < 0) || ((PRUint32)startBlock > mBitMapWords * 32 - 1) ||
        (numBlocks < 1)  || (numBlocks > 4))
       return NS_ERROR_ILLEGAL_VALUE;
           
    const PRInt32 startWord = startBlock >> 5;      
    const PRUint32 startBit = startBlock & 31;      
      
    
    if (startBit + numBlocks > 32)  return NS_ERROR_UNEXPECTED;
    PRUint32 mask = ((0x01 << numBlocks) - 1) << startBit;
    
    
    if ((mBitMap[startWord] & mask) != mask)    return NS_ERROR_ABORT;

    mBitMap[startWord] ^= mask;    
    mBitMapDirty = PR_TRUE;
    
    return NS_OK;
}





nsresult
nsDiskCacheBlockFile::WriteBlocks( void *   buffer,
                                   PRUint32 size,
                                   PRInt32  numBlocks,
                                   PRInt32 * startBlock)
{
    
    NS_ENSURE_TRUE(mFD, NS_ERROR_NOT_AVAILABLE);

    
    *startBlock = AllocateBlocks(numBlocks);
    if (*startBlock < 0)
        return NS_ERROR_NOT_AVAILABLE;

    
    PRInt32 blockPos = mBitMapWords * 4 + *startBlock * mBlockSize;
    
    
    return Write(blockPos, buffer, size) ? NS_OK : NS_ERROR_FAILURE;
}





nsresult
nsDiskCacheBlockFile::ReadBlocks( void *    buffer,
                                  PRInt32   startBlock,
                                  PRInt32   numBlocks,
                                  PRInt32 * bytesRead)
{
    

    if (!mFD)  return NS_ERROR_NOT_AVAILABLE;
    nsresult rv = VerifyAllocation(startBlock, numBlocks);
    if (NS_FAILED(rv))  return rv;
    
    
    PRInt32 blockPos = mBitMapWords * 4 + startBlock * mBlockSize;
    PRInt32 filePos = PR_Seek(mFD, blockPos, PR_SEEK_SET);
    if (filePos != blockPos)  return NS_ERROR_UNEXPECTED;

    
    PRInt32 bytesToRead = *bytesRead;
    if ((bytesToRead <= 0) || ((PRUint32)bytesToRead > mBlockSize * numBlocks)) {
        bytesToRead = mBlockSize * numBlocks;
    }
    *bytesRead = PR_Read(mFD, buffer, bytesToRead);
    
    return NS_OK;
}





nsresult
nsDiskCacheBlockFile::FlushBitMap()
{
    if (!mBitMapDirty)  return NS_OK;
    
#if defined(IS_LITTLE_ENDIAN)
    PRUint32 *bitmap = new PRUint32[mBitMapWords];
    
    PRUint32 *p = bitmap;
    for (unsigned int i = 0; i < mBitMapWords; ++i, ++p)
      *p = htonl(mBitMap[i]);
#else
    PRUint32 *bitmap = mBitMap;
#endif

    
    bool written = Write(0, bitmap, mBitMapWords * 4);
#if defined(IS_LITTLE_ENDIAN)
    delete [] bitmap;
#endif
    if (!written)
        return NS_ERROR_UNEXPECTED;

    PRStatus err = PR_Sync(mFD);
    if (err != PR_SUCCESS)  return NS_ERROR_UNEXPECTED;

    mBitMapDirty = PR_FALSE;
    return NS_OK;
}











nsresult
nsDiskCacheBlockFile::VerifyAllocation( PRInt32  startBlock, PRInt32  numBlocks)
{
    if ((startBlock < 0) || ((PRUint32)startBlock > mBitMapWords * 32 - 1) ||
        (numBlocks < 1)  || (numBlocks > 4))
       return NS_ERROR_ILLEGAL_VALUE;
    
    const PRInt32 startWord = startBlock >> 5;      
    const PRUint32 startBit = startBlock & 31;      
      
    
    if (startBit + numBlocks > 32)  return NS_ERROR_ILLEGAL_VALUE;
    PRUint32 mask = ((0x01 << numBlocks) - 1) << startBit;
    
    
    if ((mBitMap[startWord] & mask) != mask)    return NS_ERROR_FAILURE;
    
    return NS_OK;
}








PRUint32
nsDiskCacheBlockFile::CalcBlockFileSize()
{
    
    PRUint32  estimatedSize = mBitMapWords * 4;
    PRInt32   i = mBitMapWords;
    while (--i >= 0) {
        if (mBitMap[i]) break;
    }

    if (i >= 0) {
        
        PRUint32 mapWord = mBitMap[i];
        PRUint32 lastBit = 31;
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
nsDiskCacheBlockFile::Write(PRInt32 offset, const void *buf, PRInt32 amount)
{
    



    const PRInt32 upTo = offset + amount;
    
    const PRInt32 minPreallocate = 4*1024*1024;
    const PRInt32 maxPreallocate = 20*1000*1000;
    if (mFileSize < upTo) {
        
        const PRInt32 maxFileSize = mBitMapWords * 4 * (mBlockSize * 8 + 1);
        if (upTo > maxPreallocate) {
            
            mFileSize = ((upTo + minPreallocate - 1) / minPreallocate) * minPreallocate;
        } else {
            
            if (mFileSize)
                while(mFileSize < upTo)
                    mFileSize *= 2;
            mFileSize = NS_MIN(maxPreallocate, NS_MAX(mFileSize, minPreallocate));
        }
        mFileSize = NS_MIN(mFileSize, maxFileSize);
        
        
    }
    if (PR_Seek(mFD, offset, PR_SEEK_SET) != offset)
        return false;
    return PR_Write(mFD, buf, amount) == amount;
}

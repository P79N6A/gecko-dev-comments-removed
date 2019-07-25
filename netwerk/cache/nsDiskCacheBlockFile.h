







































#ifndef _nsDiskCacheBlockFile_h_
#define _nsDiskCacheBlockFile_h_

#include "nsILocalFile.h"









class nsDiskCacheBlockFile {
public:
    nsDiskCacheBlockFile()
           : mFD(nsnull)
           , mBitMap(nsnull)
           , mBlockSize(0)
           , mBitMapWords(0)
           , mFileSize(0)
           , mBitMapDirty(false)
            {}
    ~nsDiskCacheBlockFile() { (void) Close(true); }
    
    nsresult  Open( nsILocalFile *  blockFile, PRUint32  blockSize,
                    PRUint32  bitMapSize);
    nsresult  Close(bool flush);
    
    



    nsresult  Trim() { return nsDiskCache::Truncate(mFD, CalcBlockFileSize()); }
    nsresult  DeallocateBlocks( PRInt32  startBlock, PRInt32  numBlocks);
    nsresult  WriteBlocks( void * buffer, PRUint32 size, PRInt32  numBlocks, 
                           PRInt32 * startBlock);
    nsresult  ReadBlocks( void * buffer, PRInt32  startBlock, PRInt32  numBlocks, 
                          PRInt32 * bytesRead);
    
private:
    nsresult  FlushBitMap();
    PRInt32   AllocateBlocks( PRInt32  numBlocks);
    nsresult  VerifyAllocation( PRInt32 startBlock, PRInt32 numBLocks);
    PRUint32  CalcBlockFileSize();
    bool   Write(PRInt32 offset, const void *buf, PRInt32 amount);




    PRFileDesc *                mFD;
    PRUint32 *                  mBitMap;      
    PRUint32                    mBlockSize;
    PRUint32                    mBitMapWords;
    PRInt32                     mFileSize;
    bool                        mBitMapDirty;
};

#endif 

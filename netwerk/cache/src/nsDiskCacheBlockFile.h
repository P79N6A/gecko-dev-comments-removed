







































#ifndef _nsDiskCacheBlockFile_h_
#define _nsDiskCacheBlockFile_h_

#include "nsILocalFile.h"









class nsDiskCacheBlockFile {
public:
    nsDiskCacheBlockFile()
           : mFD(nsnull)
           , mBlockSize(0)
           , mBitMap(nsnull)
           , mBitMapDirty(PR_FALSE)
            {}
    ~nsDiskCacheBlockFile() { (void) Close(PR_TRUE); }
    
    nsresult  Open( nsILocalFile *  blockFile, PRUint32  blockSize);
    nsresult  Close(PRBool flush);
    
    



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




    PRFileDesc *                mFD;
    PRUint32                    mBlockSize;
    PRUint32 *                  mBitMap;      
    PRBool                      mBitMapDirty;
};

#endif 

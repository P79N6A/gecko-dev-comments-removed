






#ifndef _nsDiskCacheStreams_h_
#define _nsDiskCacheStreams_h_

#include "nsDiskCacheBinding.h"

#include "nsCache.h"

#include "nsIInputStream.h"
#include "nsIOutputStream.h"

#include "pratom.h"

class nsDiskCacheInputStream;
class nsDiskCacheOutputStream;
class nsDiskCacheDevice;

class nsDiskCacheStreamIO : public nsISupports {
public:
             nsDiskCacheStreamIO(nsDiskCacheBinding *   binding);
    virtual ~nsDiskCacheStreamIO();
    
    NS_DECL_ISUPPORTS

    nsresult    GetInputStream(uint32_t offset, nsIInputStream ** inputStream);
    nsresult    GetOutputStream(uint32_t offset, nsIOutputStream ** outputStream);

    nsresult    CloseOutputStream(nsDiskCacheOutputStream * outputStream);

    nsresult    Write( const char * buffer,
                       uint32_t     count,
                       uint32_t *   bytesWritten);

    nsresult    ClearBinding();
    
    void        IncrementInputStreamCount() { PR_ATOMIC_INCREMENT(&mInStreamCount); }
    void        DecrementInputStreamCount()
                {
                    PR_ATOMIC_DECREMENT(&mInStreamCount);
                    NS_ASSERTION(mInStreamCount >= 0, "mInStreamCount has gone negative");
                }

    size_t     SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf);

    
    
    nsDiskCacheStreamIO() { NS_NOTREACHED("oops"); }

private:

    void        Close();
    nsresult    OpenCacheFile(int flags, PRFileDesc ** fd);
    nsresult    ReadCacheBlocks(uint32_t bufferSize);
    nsresult    FlushBufferToFile();
    void        UpdateFileSize();
    void        DeleteBuffer();
    nsresult    Flush();
    nsresult    SeekAndTruncate(uint32_t offset);

    nsDiskCacheBinding *        mBinding;       
    nsDiskCacheDevice *         mDevice;
    nsDiskCacheOutputStream *   mOutStream;     
    int32_t                     mInStreamCount;
    PRFileDesc *                mFD;

    uint32_t                    mStreamEnd;     
    uint32_t                    mBufSize;       
    char *                      mBuffer;
};

#endif 

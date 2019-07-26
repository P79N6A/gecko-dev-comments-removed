






#ifndef _nsDiskCacheStreams_h_
#define _nsDiskCacheStreams_h_

#include "nsDiskCacheBinding.h"

#include "nsCache.h"

#include "nsIInputStream.h"
#include "nsIOutputStream.h"

#include "pratom.h"

class nsDiskCacheInputStream;
class nsDiskCacheDevice;

class nsDiskCacheStreamIO : public nsIOutputStream {
public:
             nsDiskCacheStreamIO(nsDiskCacheBinding *   binding);
    virtual ~nsDiskCacheStreamIO();
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOUTPUTSTREAM

    nsresult    GetInputStream(uint32_t offset, nsIInputStream ** inputStream);
    nsresult    GetOutputStream(uint32_t offset, nsIOutputStream ** outputStream);

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
    nsresult    OpenCacheFile(int flags, PRFileDesc ** fd);
    nsresult    ReadCacheBlocks(uint32_t bufferSize);
    nsresult    FlushBufferToFile();
    void        UpdateFileSize();
    void        DeleteBuffer();
    nsresult    CloseOutputStream();
    nsresult    SeekAndTruncate(uint32_t offset);

    nsDiskCacheBinding *        mBinding;       
    nsDiskCacheDevice *         mDevice;
    int32_t                     mInStreamCount;
    PRFileDesc *                mFD;

    uint32_t                    mStreamEnd;     
    uint32_t                    mBufSize;       
    char *                      mBuffer;
    bool                        mOutputStreamIsOpen;
};

#endif 

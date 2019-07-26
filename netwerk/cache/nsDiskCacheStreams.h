






#ifndef _nsDiskCacheStreams_h_
#define _nsDiskCacheStreams_h_

#include "mozilla/MemoryReporting.h"
#include "nsDiskCacheBinding.h"

#include "nsCache.h"

#include "nsIInputStream.h"
#include "nsIOutputStream.h"

#include "mozilla/Atomics.h"

class nsDiskCacheInputStream;
class nsDiskCacheDevice;

class nsDiskCacheStreamIO : public nsIOutputStream {
public:
             nsDiskCacheStreamIO(nsDiskCacheBinding *   binding);
    virtual ~nsDiskCacheStreamIO();
    
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIOUTPUTSTREAM

    nsresult    GetInputStream(uint32_t offset, nsIInputStream ** inputStream);
    nsresult    GetOutputStream(uint32_t offset, nsIOutputStream ** outputStream);

    nsresult    ClearBinding();
    
    void        IncrementInputStreamCount() { mInStreamCount++; }
    void        DecrementInputStreamCount()
                {
                    mInStreamCount--;
                    NS_ASSERTION(mInStreamCount >= 0, "mInStreamCount has gone negative");
                }

    size_t     SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf);

    
    
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
    mozilla::Atomic<int32_t>                     mInStreamCount;
    PRFileDesc *                mFD;

    uint32_t                    mStreamEnd;     
    uint32_t                    mBufSize;       
    char *                      mBuffer;
    bool                        mOutputStreamIsOpen;
};

#endif 

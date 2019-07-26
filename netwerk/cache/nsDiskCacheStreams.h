






#ifndef _nsDiskCacheStreams_h_
#define _nsDiskCacheStreams_h_

#include "nsDiskCacheBinding.h"

#include "nsCache.h"

#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIDiskCacheStreamInternal.h"

#include "pratom.h"

class nsDiskCacheInputStream;
class nsDiskCacheDevice;

class nsDiskCacheStreamIO : public nsIOutputStream, nsIDiskCacheStreamInternal {
public:
             nsDiskCacheStreamIO(nsDiskCacheBinding *   binding);
    virtual ~nsDiskCacheStreamIO();
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOUTPUTSTREAM
    NS_DECL_NSIDISKCACHESTREAMINTERNAL

    nsresult    GetInputStream(uint32_t offset, nsIInputStream ** inputStream);
    nsresult    GetOutputStream(uint32_t offset, nsIOutputStream ** outputStream);

    nsresult    Seek(int32_t whence, int32_t offset);
    nsresult    Tell(uint32_t * position);    
    nsresult    SetEOF();

    nsresult    ClearBinding();
    
    void        IncrementInputStreamCount() { PR_ATOMIC_INCREMENT(&mInStreamCount); }
    void        DecrementInputStreamCount()
                {
                    PR_ATOMIC_DECREMENT(&mInStreamCount);
                    NS_ASSERTION(mInStreamCount >= 0, "mInStreamCount has gone negative");
                }

    
    
    nsDiskCacheStreamIO() { NS_NOTREACHED("oops"); }
private:
    nsresult    OpenCacheFile(int flags, PRFileDesc ** fd);
    nsresult    ReadCacheBlocks();
    nsresult    FlushBufferToFile();
    void        UpdateFileSize();
    void        DeleteBuffer();

    nsDiskCacheBinding *        mBinding;       
    nsDiskCacheDevice *         mDevice;
    int32_t                     mInStreamCount;
    nsCOMPtr<nsIFile>           mLocalFile;
    PRFileDesc *                mFD;

    uint32_t                    mStreamPos;     
    uint32_t                    mStreamEnd;
    uint32_t                    mBufPos;        
    uint32_t                    mBufEnd;        
    uint32_t                    mBufSize;       
    bool                        mBufDirty;      
    bool                        mOutputStreamIsOpen; 
    char *                      mBuffer;
    
};

#endif 










































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

    nsresult    GetInputStream(PRUint32 offset, nsIInputStream ** inputStream);
    nsresult    GetOutputStream(PRUint32 offset, nsIOutputStream ** outputStream);

    nsresult    CloseOutputStream(nsDiskCacheOutputStream * outputStream);
        
    nsresult    Write( const char * buffer,
                       PRUint32     count,
                       PRUint32 *   bytesWritten);

    nsresult    Seek(PRInt32 whence, PRInt32 offset);
    nsresult    Tell(PRUint32 * position);    
    nsresult    SetEOF();

    void        ClearBinding();
    
    void        IncrementInputStreamCount() { PR_AtomicIncrement(&mInStreamCount); }
    void        DecrementInputStreamCount()
                {
                    PR_AtomicDecrement(&mInStreamCount);
                    NS_ASSERTION(mInStreamCount >= 0, "mInStreamCount has gone negative");
                }

    
    
    nsDiskCacheStreamIO() { NS_NOTREACHED("oops"); }
private:


    void        Close();
    nsresult    OpenCacheFile(PRIntn flags, PRFileDesc ** fd);
    nsresult    ReadCacheBlocks();
    nsresult    FlushBufferToFile();
    void        UpdateFileSize();
    void        DeleteBuffer();
    nsresult    Flush();


    nsDiskCacheBinding *        mBinding;       
    nsDiskCacheDevice *         mDevice;
    nsDiskCacheOutputStream *   mOutStream;     
    PRInt32                     mInStreamCount;
    nsCOMPtr<nsILocalFile>      mLocalFile;
    PRFileDesc *                mFD;

    PRUint32                    mStreamPos;     
    PRUint32                    mStreamEnd;
    PRUint32                    mBufPos;        
    PRUint32                    mBufEnd;        
    PRUint32                    mBufSize;       
    PRBool                      mBufDirty;
    char *                      mBuffer;
    
};

#endif 

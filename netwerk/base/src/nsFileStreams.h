




#ifndef nsFileStreams_h__
#define nsFileStreams_h__

#include "nsAlgorithm.h"
#include "nsIFileStreams.h"
#include "nsIFile.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsISafeOutputStream.h"
#include "nsISeekableStream.h"
#include "nsILineInputStream.h"
#include "nsCOMPtr.h"
#include "prlog.h"
#include "prio.h"
#include "nsIIPCSerializableInputStream.h"

template<class CharType> class nsLineBuffer;



class nsFileStreamBase : public nsISeekableStream
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISEEKABLESTREAM

    nsFileStreamBase();
    virtual ~nsFileStreamBase();

protected:
    nsresult Close();
    nsresult Available(uint64_t* _retval);
    nsresult Read(char* aBuf, uint32_t aCount, uint32_t* _retval);
    nsresult ReadSegments(nsWriteSegmentFun aWriter, void* aClosure,
                          uint32_t aCount, uint32_t* _retval);
    nsresult IsNonBlocking(bool* _retval);
    nsresult Flush();
    nsresult Write(const char* aBuf, uint32_t aCount, uint32_t* _retval);
    nsresult WriteFrom(nsIInputStream* aFromStream, uint32_t aCount,
                       uint32_t* _retval);
    nsresult WriteSegments(nsReadSegmentFun aReader, void* aClosure,
                           uint32_t aCount, uint32_t* _retval);

    PRFileDesc* mFD;

    


    int32_t mBehaviorFlags;

    


    bool mDeferredOpen;

    struct OpenParams {
        nsCOMPtr<nsIFile> localFile;
        int32_t ioFlags;
        int32_t perm;
    };

    


    OpenParams mOpenParams;

    




    nsresult MaybeOpen(nsIFile* aFile, int32_t aIoFlags, int32_t aPerm,
                       bool aDeferred);

    


    void CleanUpOpen();

    





    virtual nsresult DoOpen();

    



    inline nsresult DoPendingOpen();
};



class nsFileInputStream : public nsFileStreamBase,
                          public nsIFileInputStream,
                          public nsILineInputStream,
                          public nsIIPCSerializableInputStream
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIFILEINPUTSTREAM
    NS_DECL_NSILINEINPUTSTREAM
    NS_DECL_NSIIPCSERIALIZABLEINPUTSTREAM

    NS_IMETHOD Close();
    NS_IMETHOD Tell(int64_t *aResult);
    NS_IMETHOD Available(uint64_t* _retval);
    NS_IMETHOD Read(char* aBuf, uint32_t aCount, uint32_t* _retval);
    NS_IMETHOD ReadSegments(nsWriteSegmentFun aWriter, void *aClosure,
                            uint32_t aCount, uint32_t* _retval)
    {
        return nsFileStreamBase::ReadSegments(aWriter, aClosure, aCount,
                                              _retval);
    }
    NS_IMETHOD IsNonBlocking(bool* _retval)
    {
        return nsFileStreamBase::IsNonBlocking(_retval);
    } 
    
    
    NS_IMETHOD Seek(int32_t aWhence, int64_t aOffset);

    nsFileInputStream()
      : mLineBuffer(nullptr), mIOFlags(0), mPerm(0), mCachedPosition(0)
    {}

    virtual ~nsFileInputStream()
    {
        Close();
    }

    static nsresult
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

protected:
    nsLineBuffer<char> *mLineBuffer;

    


    nsCOMPtr<nsIFile> mFile;
    


    int32_t mIOFlags;
    


    int32_t mPerm;

    


    int64_t mCachedPosition;

protected:
    



    nsresult Open(nsIFile* file, int32_t ioFlags, int32_t perm);
};



class nsPartialFileInputStream : public nsFileInputStream,
                                 public nsIPartialFileInputStream
{
public:
    using nsFileInputStream::Init;
    using nsFileInputStream::Read;
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIPARTIALFILEINPUTSTREAM
    NS_DECL_NSIIPCSERIALIZABLEINPUTSTREAM

    nsPartialFileInputStream()
      : mStart(0), mLength(0), mPosition(0)
    { }

    NS_IMETHOD Tell(int64_t *aResult);
    NS_IMETHOD Available(uint64_t *aResult);
    NS_IMETHOD Read(char* aBuf, uint32_t aCount, uint32_t* aResult);
    NS_IMETHOD Seek(int32_t aWhence, int64_t aOffset);

    static nsresult
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

private:
    uint64_t TruncateSize(uint64_t aSize) {
          return NS_MIN<uint64_t>(mLength - mPosition, aSize);
    }

    uint64_t mStart;
    uint64_t mLength;
    uint64_t mPosition;
};



class nsFileOutputStream : public nsFileStreamBase,
                           public nsIFileOutputStream
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIFILEOUTPUTSTREAM
    NS_FORWARD_NSIOUTPUTSTREAM(nsFileStreamBase::)

    virtual ~nsFileOutputStream()
    {
        Close();
    }

    static nsresult
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);
};



class nsSafeFileOutputStream : public nsFileOutputStream,
                               public nsISafeOutputStream
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSISAFEOUTPUTSTREAM

    nsSafeFileOutputStream() :
        mTargetFileExists(true),
        mWriteResult(NS_OK) {}

    virtual ~nsSafeFileOutputStream()
    {
        Close();
    }

    virtual nsresult DoOpen();

    NS_IMETHODIMP Close();
    NS_IMETHODIMP Write(const char *buf, uint32_t count, uint32_t *result);
    NS_IMETHODIMP Init(nsIFile* file, int32_t ioFlags, int32_t perm, int32_t behaviorFlags);

protected:
    nsCOMPtr<nsIFile>         mTargetFile;
    nsCOMPtr<nsIFile>         mTempFile;

    bool     mTargetFileExists;
    nsresult mWriteResult; 
};



class nsFileStream : public nsFileStreamBase,
                     public nsIInputStream,
                     public nsIOutputStream,
                     public nsIFileStream,
                     public nsIFileMetadata
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIFILESTREAM
    NS_DECL_NSIFILEMETADATA
    NS_FORWARD_NSIINPUTSTREAM(nsFileStreamBase::)

    
    
    NS_IMETHOD Flush()
    {
        return nsFileStreamBase::Flush();
    }
    NS_IMETHOD Write(const char* aBuf, uint32_t aCount, uint32_t* _retval)
    {
        return nsFileStreamBase::Write(aBuf, aCount, _retval);
    }
    NS_IMETHOD WriteFrom(nsIInputStream* aFromStream, uint32_t aCount,
                         uint32_t* _retval)
    {
        return nsFileStreamBase::WriteFrom(aFromStream, aCount, _retval);
    }
    NS_IMETHOD WriteSegments(nsReadSegmentFun aReader, void* aClosure,
                             uint32_t aCount, uint32_t* _retval)
    {
        return nsFileStreamBase::WriteSegments(aReader, aClosure, aCount,
                                               _retval);
    }

    virtual ~nsFileStream()
    {
        Close();
    }
};



#endif 

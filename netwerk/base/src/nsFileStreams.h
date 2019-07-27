




#ifndef nsFileStreams_h__
#define nsFileStreams_h__

#include "nsAutoPtr.h"
#include "nsIFileStreams.h"
#include "nsIFile.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsISafeOutputStream.h"
#include "nsISeekableStream.h"
#include "nsILineInputStream.h"
#include "nsCOMPtr.h"
#include "nsIIPCSerializableInputStream.h"
#include "nsReadLine.h"
#include <algorithm>




class nsFileStreamBase : public nsISeekableStream,
                         public nsIFileMetadata
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSISEEKABLESTREAM
    NS_DECL_NSIFILEMETADATA

    nsFileStreamBase();

protected:
    virtual ~nsFileStreamBase();

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

    NS_IMETHOD Close() MOZ_OVERRIDE;
    NS_IMETHOD Tell(int64_t *aResult) MOZ_OVERRIDE;
    NS_IMETHOD Available(uint64_t* _retval) MOZ_OVERRIDE;
    NS_IMETHOD Read(char* aBuf, uint32_t aCount, uint32_t* _retval) MOZ_OVERRIDE;
    NS_IMETHOD ReadSegments(nsWriteSegmentFun aWriter, void *aClosure,
                            uint32_t aCount, uint32_t* _retval) MOZ_OVERRIDE
    {
        return nsFileStreamBase::ReadSegments(aWriter, aClosure, aCount,
                                              _retval);
    }
    NS_IMETHOD IsNonBlocking(bool* _retval) MOZ_OVERRIDE
    {
        return nsFileStreamBase::IsNonBlocking(_retval);
    }

    
    NS_IMETHOD Seek(int32_t aWhence, int64_t aOffset) MOZ_OVERRIDE;

    nsFileInputStream()
      : mLineBuffer(nullptr), mIOFlags(0), mPerm(0), mCachedPosition(0)
    {}

    static nsresult
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

protected:
    virtual ~nsFileInputStream()
    {
        Close();
    }

    nsAutoPtr<nsLineBuffer<char> > mLineBuffer;

    


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

    NS_IMETHOD Tell(int64_t *aResult) MOZ_OVERRIDE;
    NS_IMETHOD Available(uint64_t *aResult) MOZ_OVERRIDE;
    NS_IMETHOD Read(char* aBuf, uint32_t aCount, uint32_t* aResult) MOZ_OVERRIDE;
    NS_IMETHOD Seek(int32_t aWhence, int64_t aOffset) MOZ_OVERRIDE;

    static nsresult
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

protected:
    ~nsPartialFileInputStream()
    { }

private:
    uint64_t TruncateSize(uint64_t aSize) {
          return std::min<uint64_t>(mLength - mPosition, aSize);
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

    static nsresult
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

protected:
    virtual ~nsFileOutputStream()
    {
        Close();
    }
};








class nsAtomicFileOutputStream : public nsFileOutputStream,
                                 public nsISafeOutputStream
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSISAFEOUTPUTSTREAM

    nsAtomicFileOutputStream() :
        mTargetFileExists(true),
        mWriteResult(NS_OK) {}

    virtual nsresult DoOpen() MOZ_OVERRIDE;

    NS_IMETHODIMP Close() MOZ_OVERRIDE;
    NS_IMETHODIMP Write(const char *buf, uint32_t count, uint32_t *result) MOZ_OVERRIDE;
    NS_IMETHODIMP Init(nsIFile* file, int32_t ioFlags, int32_t perm, int32_t behaviorFlags) MOZ_OVERRIDE;

protected:
    virtual ~nsAtomicFileOutputStream()
    {
        Close();
    }

    nsCOMPtr<nsIFile>         mTargetFile;
    nsCOMPtr<nsIFile>         mTempFile;

    bool     mTargetFileExists;
    nsresult mWriteResult; 

};









class nsSafeFileOutputStream : public nsAtomicFileOutputStream
{
public:

    NS_IMETHOD Finish() MOZ_OVERRIDE;
};



class nsFileStream : public nsFileStreamBase,
                     public nsIInputStream,
                     public nsIOutputStream,
                     public nsIFileStream
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIFILESTREAM
    NS_FORWARD_NSIINPUTSTREAM(nsFileStreamBase::)

    
    
    NS_IMETHOD Flush() MOZ_OVERRIDE
    {
        return nsFileStreamBase::Flush();
    }
    NS_IMETHOD Write(const char* aBuf, uint32_t aCount, uint32_t* _retval) MOZ_OVERRIDE
    {
        return nsFileStreamBase::Write(aBuf, aCount, _retval);
    }
    NS_IMETHOD WriteFrom(nsIInputStream* aFromStream, uint32_t aCount,
                         uint32_t* _retval) MOZ_OVERRIDE
    {
        return nsFileStreamBase::WriteFrom(aFromStream, aCount, _retval);
    }
    NS_IMETHOD WriteSegments(nsReadSegmentFun aReader, void* aClosure,
                             uint32_t aCount, uint32_t* _retval) MOZ_OVERRIDE
    {
        return nsFileStreamBase::WriteSegments(aReader, aClosure, aCount,
                                               _retval);
    }

protected:
    virtual ~nsFileStream()
    {
        Close();
    }
};



#endif 

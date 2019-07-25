




































#ifndef nsFileStreams_h__
#define nsFileStreams_h__

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
#include "nsIIPCSerializable.h"

template<class CharType> class nsLineBuffer;



class nsFileStream : public nsISeekableStream
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISEEKABLESTREAM

    nsFileStream();
    virtual ~nsFileStream();

    nsresult Close();
    nsresult InitWithFileDescriptor(PRFileDesc* fd, nsISupports* parent);

protected:
    PRFileDesc*           mFD;
    nsCOMPtr<nsISupports> mParent; 
                                   
    PRBool                mCloseFD;
};



class nsFileInputStream : public nsFileStream,
                          public nsIFileInputStream,
                          public nsILineInputStream,
                          public nsIIPCSerializable
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIINPUTSTREAM
    NS_DECL_NSIFILEINPUTSTREAM
    NS_DECL_NSILINEINPUTSTREAM
    NS_DECL_NSIIPCSERIALIZABLE
    
    
    NS_IMETHOD Seek(PRInt32 aWhence, PRInt64 aOffset);

    nsFileInputStream() : nsFileStream() 
    {
        mLineBuffer = nsnull;
        mBehaviorFlags = 0;
    }
    virtual ~nsFileInputStream() 
    {
        Close();
    }

    static nsresult
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

protected:
    nsLineBuffer<char> *mLineBuffer;

    


    nsCOMPtr<nsIFile> mFile;
    


    PRInt32 mIOFlags;
    


    PRInt32 mPerm;
    


    PRInt32 mBehaviorFlags;

protected:
    



    nsresult Open(nsIFile* file, PRInt32 ioFlags, PRInt32 perm);
    


    nsresult Reopen() { return Open(mFile, mIOFlags, mPerm); }
};



class nsPartialFileInputStream : public nsFileInputStream,
                                 public nsIPartialFileInputStream
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIPARTIALFILEINPUTSTREAM

    NS_IMETHOD Tell(PRInt64 *aResult);
    NS_IMETHOD Available(PRUint32 *aResult);
    NS_IMETHOD Read(char* aBuf, PRUint32 aCount, PRUint32* aResult);
    NS_IMETHOD Seek(PRInt32 aWhence, PRInt64 aOffset);

    static nsresult
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

private:
    PRUint32 TruncateSize(PRUint32 aSize) {
          return (PRUint32)PR_MIN(mLength - mPosition, (PRUint64)aSize);
    }

    PRUint64 mStart;
    PRUint64 mLength;
    PRUint64 mPosition;
};



class nsFileOutputStream : public nsFileStream,
                           public nsIFileOutputStream
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIOUTPUTSTREAM
    NS_DECL_NSIFILEOUTPUTSTREAM

    nsFileOutputStream() : nsFileStream() {}
    virtual ~nsFileOutputStream() { nsFileOutputStream::Close(); }
    
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
        mTargetFileExists(PR_TRUE),
        mWriteResult(NS_OK) {}

    virtual ~nsSafeFileOutputStream() { nsSafeFileOutputStream::Close(); }

    NS_IMETHODIMP Close();
    NS_IMETHODIMP Write(const char *buf, PRUint32 count, PRUint32 *result);
    NS_IMETHODIMP Init(nsIFile* file, PRInt32 ioFlags, PRInt32 perm, PRInt32 behaviorFlags);

protected:
    nsCOMPtr<nsIFile>         mTargetFile;
    nsCOMPtr<nsIFile>         mTempFile;

    PRBool   mTargetFileExists;
    nsresult mWriteResult; 
};



#endif 

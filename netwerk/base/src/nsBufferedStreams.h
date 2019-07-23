




































#ifndef nsBufferedStreams_h__
#define nsBufferedStreams_h__

#include "nsIBufferedStreams.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsISafeOutputStream.h"
#include "nsISeekableStream.h"
#include "nsIStreamBufferAccess.h"
#include "nsCOMPtr.h"
#include "nsInt64.h"


class nsBufferedStream : public nsISeekableStream
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISEEKABLESTREAM

    nsBufferedStream();
    virtual ~nsBufferedStream();

    nsresult Close();

protected:
    nsresult Init(nsISupports* stream, PRUint32 bufferSize);
    NS_IMETHOD Fill() = 0;
    NS_IMETHOD Flush() = 0;

    PRUint32                    mBufferSize;
    char*                       mBuffer;

    
    nsInt64                     mBufferStartOffset;

    
    
    PRUint32                    mCursor;

    
    
    
    PRUint32                    mFillPoint;

    nsISupports*                mStream;        

    PRPackedBool                mBufferDisabled;
    PRUint8                     mGetBufferCount;
};



class nsBufferedInputStream : public nsBufferedStream,
                              public nsIBufferedInputStream,
                              public nsIStreamBufferAccess
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIINPUTSTREAM
    NS_DECL_NSIBUFFEREDINPUTSTREAM
    NS_DECL_NSISTREAMBUFFERACCESS

    nsBufferedInputStream() : nsBufferedStream() {}
    virtual ~nsBufferedInputStream() {}

    static NS_METHOD
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

    nsIInputStream* Source() { 
        return (nsIInputStream*)mStream;
    }

protected:
    NS_IMETHOD Fill();
    NS_IMETHOD Flush() { return NS_OK; } 
};



class nsBufferedOutputStream : public nsBufferedStream, 
                               public nsISafeOutputStream,
                               public nsIBufferedOutputStream,
                               public nsIStreamBufferAccess
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIOUTPUTSTREAM
    NS_DECL_NSISAFEOUTPUTSTREAM
    NS_DECL_NSIBUFFEREDOUTPUTSTREAM
    NS_DECL_NSISTREAMBUFFERACCESS

    nsBufferedOutputStream() : nsBufferedStream() {}
    virtual ~nsBufferedOutputStream() { nsBufferedOutputStream::Close(); }

    static NS_METHOD
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

    nsIOutputStream* Sink() { 
        return (nsIOutputStream*)mStream;
    }

protected:
    NS_IMETHOD Fill() { return NS_OK; } 

    nsCOMPtr<nsISafeOutputStream> mSafeStream; 
};



#endif 

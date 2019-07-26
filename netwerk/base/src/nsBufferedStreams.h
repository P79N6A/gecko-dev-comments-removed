




#ifndef nsBufferedStreams_h__
#define nsBufferedStreams_h__

#include "nsIBufferedStreams.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsISafeOutputStream.h"
#include "nsISeekableStream.h"
#include "nsIStreamBufferAccess.h"
#include "nsCOMPtr.h"
#include "nsIIPCSerializableObsolete.h"



class nsBufferedStream : public nsISeekableStream
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISEEKABLESTREAM

    nsBufferedStream();
    virtual ~nsBufferedStream();

    nsresult Close();

protected:
    nsresult Init(nsISupports* stream, uint32_t bufferSize);
    NS_IMETHOD Fill() = 0;
    NS_IMETHOD Flush() = 0;

    uint32_t                    mBufferSize;
    char*                       mBuffer;

    
    int64_t                     mBufferStartOffset;

    
    
    uint32_t                    mCursor;

    
    
    
    uint32_t                    mFillPoint;

    nsISupports*                mStream;        

    bool                        mBufferDisabled;
    bool                        mEOF;  
    uint8_t                     mGetBufferCount;
};



class nsBufferedInputStream : public nsBufferedStream,
                              public nsIBufferedInputStream,
                              public nsIStreamBufferAccess,
                              public nsIIPCSerializableObsolete
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIINPUTSTREAM
    NS_DECL_NSIBUFFEREDINPUTSTREAM
    NS_DECL_NSISTREAMBUFFERACCESS
    NS_DECL_NSIIPCSERIALIZABLEOBSOLETE

    nsBufferedInputStream() : nsBufferedStream() {}
    virtual ~nsBufferedInputStream() {}

    static nsresult
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

    static nsresult
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

    nsIOutputStream* Sink() { 
        return (nsIOutputStream*)mStream;
    }

protected:
    NS_IMETHOD Fill() { return NS_OK; } 

    nsCOMPtr<nsISafeOutputStream> mSafeStream; 
};



#endif 

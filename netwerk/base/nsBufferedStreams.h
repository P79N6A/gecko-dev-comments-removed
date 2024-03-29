




#ifndef nsBufferedStreams_h__
#define nsBufferedStreams_h__

#include "nsIBufferedStreams.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsISafeOutputStream.h"
#include "nsISeekableStream.h"
#include "nsIStreamBufferAccess.h"
#include "nsCOMPtr.h"
#include "nsIIPCSerializableInputStream.h"



class nsBufferedStream : public nsISeekableStream
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSISEEKABLESTREAM

    nsBufferedStream();

    nsresult Close();

protected:
    virtual ~nsBufferedStream();

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
                              public nsIIPCSerializableInputStream
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIINPUTSTREAM
    NS_DECL_NSIBUFFEREDINPUTSTREAM
    NS_DECL_NSISTREAMBUFFERACCESS
    NS_DECL_NSIIPCSERIALIZABLEINPUTSTREAM

    nsBufferedInputStream() : nsBufferedStream() {}

    static nsresult
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

    nsIInputStream* Source() { 
        return (nsIInputStream*)mStream;
    }

protected:
    virtual ~nsBufferedInputStream() {}

    NS_IMETHOD Fill() override;
    NS_IMETHOD Flush() override { return NS_OK; } 
};



class nsBufferedOutputStream final : public nsBufferedStream,
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

    static nsresult
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

    nsIOutputStream* Sink() { 
        return (nsIOutputStream*)mStream;
    }

protected:
    virtual ~nsBufferedOutputStream() { nsBufferedOutputStream::Close(); }

    NS_IMETHOD Fill() override { return NS_OK; } 

    nsCOMPtr<nsISafeOutputStream> mSafeStream; 
};



#endif 

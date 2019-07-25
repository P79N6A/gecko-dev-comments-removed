










































#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsIMultiplexInputStream.h"
#include "nsIMIMEInputStream.h"
#include "nsISeekableStream.h"
#include "nsIStringStream.h"
#include "nsString.h"

class nsMIMEInputStream : public nsIMIMEInputStream,
                          public nsISeekableStream
{
public:
    nsMIMEInputStream();
    virtual ~nsMIMEInputStream();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIINPUTSTREAM
    NS_DECL_NSIMIMEINPUTSTREAM
    NS_DECL_NSISEEKABLESTREAM
    
    NS_METHOD Init();

private:

    void InitStreams();

    struct ReadSegmentsState {
        nsIInputStream* mThisStream;
        nsWriteSegmentFun mWriter;
        void* mClosure;
    };
    static NS_METHOD ReadSegCb(nsIInputStream* aIn, void* aClosure,
                               const char* aFromRawSegment, PRUint32 aToOffset,
                               PRUint32 aCount, PRUint32 *aWriteCount);

    nsCString mHeaders;
    nsCOMPtr<nsIStringInputStream> mHeaderStream;
    
    nsCString mContentLength;
    nsCOMPtr<nsIStringInputStream> mCLStream;
    
    nsCOMPtr<nsIInputStream> mData;
    nsCOMPtr<nsIMultiplexInputStream> mStream;
    PRPackedBool mAddContentLength;
    PRPackedBool mStartedReading;
};

NS_IMPL_THREADSAFE_ISUPPORTS3(nsMIMEInputStream,
                              nsIMIMEInputStream,
                              nsIInputStream,
                              nsISeekableStream)

nsMIMEInputStream::nsMIMEInputStream() : mAddContentLength(PR_FALSE),
                                         mStartedReading(PR_FALSE)
{
}

nsMIMEInputStream::~nsMIMEInputStream()
{
}

NS_METHOD nsMIMEInputStream::Init()
{
    nsresult rv = NS_OK;
    mStream = do_CreateInstance("@mozilla.org/io/multiplex-input-stream;1",
                                &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    mHeaderStream = do_CreateInstance("@mozilla.org/io/string-input-stream;1",
                                      &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    mCLStream = do_CreateInstance("@mozilla.org/io/string-input-stream;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mStream->AppendStream(mHeaderStream);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mStream->AppendStream(mCLStream);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}



NS_IMETHODIMP
nsMIMEInputStream::GetAddContentLength(PRBool *aAddContentLength)
{
    *aAddContentLength = mAddContentLength;
    return NS_OK;
}
NS_IMETHODIMP
nsMIMEInputStream::SetAddContentLength(PRBool aAddContentLength)
{
    NS_ENSURE_FALSE(mStartedReading, NS_ERROR_FAILURE);
    mAddContentLength = aAddContentLength;
    return NS_OK;
}


NS_IMETHODIMP
nsMIMEInputStream::AddHeader(const char *aName, const char *aValue)
{
    NS_ENSURE_FALSE(mStartedReading, NS_ERROR_FAILURE);
    mHeaders.Append(aName);
    mHeaders.AppendLiteral(": ");
    mHeaders.Append(aValue);
    mHeaders.AppendLiteral("\r\n");

    
    
    
    mHeaderStream->ShareData(mHeaders.get(), 0);

    return NS_OK;
}


NS_IMETHODIMP
nsMIMEInputStream::SetData(nsIInputStream *aStream)
{
    NS_ENSURE_FALSE(mStartedReading, NS_ERROR_FAILURE);
    
    if (mData)
        mStream->RemoveStream(2);

    mData = aStream;
    if (aStream)
        mStream->AppendStream(mData);
    return NS_OK;
}


void nsMIMEInputStream::InitStreams()
{
    NS_ASSERTION(!mStartedReading,
                 "Don't call initStreams twice without rewinding");

    mStartedReading = PR_TRUE;

    
    if (mAddContentLength) {
        PRUint32 cl = 0;
        if (mData) {
            mData->Available(&cl);
        }
        mContentLength.AssignLiteral("Content-Length: ");
        mContentLength.AppendInt((PRInt32)cl);
        mContentLength.AppendLiteral("\r\n\r\n");
    }
    else {
        mContentLength.AssignLiteral("\r\n");
    }
    mCLStream->ShareData(mContentLength.get(), -1);
    mHeaderStream->ShareData(mHeaders.get(), -1);
}



#define INITSTREAMS         \
if (!mStartedReading) {     \
    InitStreams();          \
}


NS_IMETHODIMP
nsMIMEInputStream::Seek(PRInt32 whence, PRInt64 offset)
{
    nsresult rv;
    nsCOMPtr<nsISeekableStream> stream = do_QueryInterface(mStream);
    if (whence == NS_SEEK_SET && LL_EQ(offset, LL_Zero())) {
        rv = stream->Seek(whence, offset);
        if (NS_SUCCEEDED(rv))
            mStartedReading = PR_FALSE;
    }
    else {
        INITSTREAMS;
        rv = stream->Seek(whence, offset);
    }

    return rv;
}


NS_IMETHODIMP nsMIMEInputStream::ReadSegments(nsWriteSegmentFun aWriter,
                                              void *aClosure, PRUint32 aCount,
                                              PRUint32 *_retval)
{
    INITSTREAMS;
    ReadSegmentsState state;
    state.mThisStream = this;
    state.mWriter = aWriter;
    state.mClosure = aClosure;
    return mStream->ReadSegments(ReadSegCb, &state, aCount, _retval);
}

NS_METHOD
nsMIMEInputStream::ReadSegCb(nsIInputStream* aIn, void* aClosure,
                             const char* aFromRawSegment,
                             PRUint32 aToOffset, PRUint32 aCount,
                             PRUint32 *aWriteCount)
{
    ReadSegmentsState* state = (ReadSegmentsState*)aClosure;
    return  (state->mWriter)(state->mThisStream,
                             state->mClosure,
                             aFromRawSegment,
                             aToOffset,
                             aCount,
                             aWriteCount);
}






NS_IMETHODIMP nsMIMEInputStream::Close(void) { INITSTREAMS; return mStream->Close(); }
NS_IMETHODIMP nsMIMEInputStream::Available(PRUint32 *_retval) { INITSTREAMS; return mStream->Available(_retval); }
NS_IMETHODIMP nsMIMEInputStream::Read(char * buf, PRUint32 count, PRUint32 *_retval) { INITSTREAMS; return mStream->Read(buf, count, _retval); }
NS_IMETHODIMP nsMIMEInputStream::IsNonBlocking(PRBool *aNonBlocking) { INITSTREAMS; return mStream->IsNonBlocking(aNonBlocking); }


NS_IMETHODIMP nsMIMEInputStream::Tell(PRInt64 *_retval)
{
    INITSTREAMS;
    nsCOMPtr<nsISeekableStream> stream = do_QueryInterface(mStream);
    return stream->Tell(_retval);
}
NS_IMETHODIMP nsMIMEInputStream::SetEOF(void) {
    INITSTREAMS;
    nsCOMPtr<nsISeekableStream> stream = do_QueryInterface(mStream);
    return stream->SetEOF();
}






NS_METHOD
nsMIMEInputStreamConstructor(nsISupports *outer, REFNSIID iid, void **result)
{
    *result = nsnull;

    if (outer)
        return NS_ERROR_NO_AGGREGATION;

    nsMIMEInputStream *inst;
    NS_NEWXPCOM(inst, nsMIMEInputStream);
    if (!inst)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(inst);

    nsresult rv = inst->Init();
    if (NS_FAILED(rv)) {
        NS_RELEASE(inst);
        return rv;
    }

    rv = inst->QueryInterface(iid, result);
    NS_RELEASE(inst);

    return rv;
}

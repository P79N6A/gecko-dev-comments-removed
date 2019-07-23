





































#include "nsIInputStreamTee.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsCOMPtr.h"

class nsInputStreamTee : public nsIInputStreamTee
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIINPUTSTREAM
    NS_DECL_NSIINPUTSTREAMTEE

    nsInputStreamTee();

private:
    ~nsInputStreamTee() {}

    nsresult TeeSegment(const char *buf, PRUint32 count);

    static NS_METHOD WriteSegmentFun(nsIInputStream *, void *, const char *,
                                     PRUint32, PRUint32, PRUint32 *);

private:
    nsCOMPtr<nsIInputStream>  mSource;
    nsCOMPtr<nsIOutputStream> mSink;
    nsWriteSegmentFun         mWriter;  
    void                     *mClosure; 
};

nsInputStreamTee::nsInputStreamTee()
{
}

nsresult
nsInputStreamTee::TeeSegment(const char *buf, PRUint32 count)
{
    if (!mSink)
        return NS_OK; 
    nsresult rv;
    PRUint32 bytesWritten = 0;
    while (count) {
        rv = mSink->Write(buf + bytesWritten, count, &bytesWritten);
        if (NS_FAILED(rv)) {
            
            
            NS_WARNING("Write failed (non-fatal)");
            
            NS_ASSERTION(rv != NS_BASE_STREAM_WOULD_BLOCK, "sink must be a blocking stream");
            mSink = 0;
            break;
        }
        NS_ASSERTION(bytesWritten <= count, "wrote too much");
        count -= bytesWritten;
    }
    return NS_OK;
}

NS_METHOD
nsInputStreamTee::WriteSegmentFun(nsIInputStream *in, void *closure, const char *fromSegment,
                                  PRUint32 offset, PRUint32 count, PRUint32 *writeCount)
{
    nsInputStreamTee *tee = reinterpret_cast<nsInputStreamTee *>(closure);

    nsresult rv = tee->mWriter(in, tee->mClosure, fromSegment, offset, count, writeCount);
    if (NS_FAILED(rv) || (*writeCount == 0)) {
        NS_ASSERTION((NS_FAILED(rv) ? (*writeCount == 0) : PR_TRUE),
                "writer returned an error with non-zero writeCount");
        return rv;
    }

    return tee->TeeSegment(fromSegment, *writeCount);
}

NS_IMPL_ISUPPORTS2(nsInputStreamTee,
                   nsIInputStreamTee,
                   nsIInputStream)

NS_IMETHODIMP
nsInputStreamTee::Close()
{
    NS_ENSURE_TRUE(mSource, NS_ERROR_NOT_INITIALIZED);
    nsresult rv = mSource->Close();
    mSource = 0;
    mSink = 0;
    return rv;
}

NS_IMETHODIMP
nsInputStreamTee::Available(PRUint32 *avail)
{
    NS_ENSURE_TRUE(mSource, NS_ERROR_NOT_INITIALIZED);
    return mSource->Available(avail);
}

NS_IMETHODIMP
nsInputStreamTee::Read(char *buf, PRUint32 count, PRUint32 *bytesRead)
{
    NS_ENSURE_TRUE(mSource, NS_ERROR_NOT_INITIALIZED);

    nsresult rv = mSource->Read(buf, count, bytesRead);
    if (NS_FAILED(rv) || (*bytesRead == 0))
        return rv;

    return TeeSegment(buf, *bytesRead);
}

NS_IMETHODIMP
nsInputStreamTee::ReadSegments(nsWriteSegmentFun writer, 
                               void *closure,
                               PRUint32 count,
                               PRUint32 *bytesRead)
{
    NS_ENSURE_TRUE(mSource, NS_ERROR_NOT_INITIALIZED);

    mWriter = writer;
    mClosure = closure;

    return mSource->ReadSegments(WriteSegmentFun, this, count, bytesRead);
}

NS_IMETHODIMP
nsInputStreamTee::IsNonBlocking(PRBool *result)
{
    NS_ENSURE_TRUE(mSource, NS_ERROR_NOT_INITIALIZED);
    return mSource->IsNonBlocking(result);
}

NS_IMETHODIMP
nsInputStreamTee::SetSource(nsIInputStream *source)
{
    mSource = source;
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamTee::GetSource(nsIInputStream **source)
{
    NS_IF_ADDREF(*source = mSource);
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamTee::SetSink(nsIOutputStream *sink)
{
#ifdef DEBUG
    if (sink) {
        PRBool nonBlocking;
        nsresult rv = sink->IsNonBlocking(&nonBlocking);
        if (NS_FAILED(rv) || nonBlocking)
            NS_ERROR("sink should be a blocking stream");
    }
#endif
    mSink = sink;
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamTee::GetSink(nsIOutputStream **sink)
{
    NS_IF_ADDREF(*sink = mSink);
    return NS_OK;
}



NS_COM nsresult 
NS_NewInputStreamTee(nsIInputStream **result,
                     nsIInputStream *source,
                     nsIOutputStream *sink)
{
    nsresult rv;
    
    nsCOMPtr<nsIInputStreamTee> tee;
    NS_NEWXPCOM(tee, nsInputStreamTee);
    if (!tee)
        return NS_ERROR_OUT_OF_MEMORY;

    rv = tee->SetSource(source);
    if (NS_FAILED(rv)) return rv;

    rv = tee->SetSink(sink);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*result = tee);
    return rv;
}

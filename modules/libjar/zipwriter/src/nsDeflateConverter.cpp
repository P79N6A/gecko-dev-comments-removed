






































#include "StreamFunctions.h"
#include "nsDeflateConverter.h"
#include "nsIStringStream.h"
#include "nsIInputStreamPump.h"
#include "nsComponentManagerUtils.h"
#include "nsMemory.h"
#include "nsAutoPtr.h"





NS_IMPL_ISUPPORTS3(nsDeflateConverter, nsIStreamConverter,
                                       nsIStreamListener,
                                       nsIRequestObserver)

nsresult nsDeflateConverter::Init()
{
    int zerr;

    mOffset = 0;

    mZstream.zalloc = Z_NULL;
    mZstream.zfree = Z_NULL;
    mZstream.opaque = Z_NULL;

    zerr = deflateInit2(&mZstream, mLevel, Z_DEFLATED, -MAX_WBITS, 8,
                        Z_DEFAULT_STRATEGY);
    if (zerr != Z_OK) return NS_ERROR_OUT_OF_MEMORY;

    mZstream.next_out = mWriteBuffer;
    mZstream.avail_out = sizeof(mWriteBuffer);

    return NS_OK;
}



NS_IMETHODIMP nsDeflateConverter::Convert(nsIInputStream *aFromStream,
                                          const char *aFromType,
                                          const char *aToType,
                                          nsISupports *aCtxt,
                                          nsIInputStream **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}




NS_IMETHODIMP nsDeflateConverter::AsyncConvertData(const char *aFromType,
                                                   const char *aToType,
                                                   nsIStreamListener *aListener,
                                                   nsISupports *aCtxt)
{
    if (mListener)
        return NS_ERROR_ALREADY_INITIALIZED;

    NS_ENSURE_ARG_POINTER(aListener);

    nsresult rv = Init();
    NS_ENSURE_SUCCESS(rv, rv);

    mListener = aListener;
    mContext = aCtxt;
    return rv;
}




NS_IMETHODIMP nsDeflateConverter::OnDataAvailable(nsIRequest *aRequest,
                                                  nsISupports *aContext,
                                                  nsIInputStream *aInputStream,
                                                  PRUint32 aOffset,
                                                  PRUint32 aCount)
{
    if (!mListener)
        return NS_ERROR_NOT_INITIALIZED;

    nsAutoArrayPtr<char> buffer(new char[aCount]);
    NS_ENSURE_TRUE(buffer, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = ZW_ReadData(aInputStream, buffer.get(), aCount);
    NS_ENSURE_SUCCESS(rv, rv);

    
    mZstream.avail_in = aCount;
    mZstream.next_in = (unsigned char*)buffer.get();

    int zerr = Z_OK;
    
    while (mZstream.avail_in > 0 && zerr == Z_OK) {
        zerr = deflate(&mZstream, Z_NO_FLUSH);

        while (mZstream.avail_out == 0) {
            
            rv = PushAvailableData(aRequest, aContext);
            NS_ENSURE_SUCCESS(rv, rv);
            zerr = deflate(&mZstream, Z_NO_FLUSH);
        }
    }

    return NS_OK;
}


NS_IMETHODIMP nsDeflateConverter::OnStartRequest(nsIRequest *aRequest,
                                                 nsISupports *aContext)
{
    if (!mListener)
        return NS_ERROR_NOT_INITIALIZED;

    return mListener->OnStartRequest(aRequest, mContext);
}



NS_IMETHODIMP nsDeflateConverter::OnStopRequest(nsIRequest *aRequest,
                                                nsISupports *aContext,
                                                nsresult aStatusCode)
{
    if (!mListener)
        return NS_ERROR_NOT_INITIALIZED;

    nsresult rv;

    int zerr;
    do {
        zerr = deflate(&mZstream, Z_FINISH);
        rv = PushAvailableData(aRequest, aContext);
        NS_ENSURE_SUCCESS(rv, rv);
    } while (zerr == Z_OK);

    deflateEnd(&mZstream);

    return mListener->OnStopRequest(aRequest, mContext, aStatusCode);
}

nsresult nsDeflateConverter::PushAvailableData(nsIRequest *aRequest,
                                               nsISupports *aContext)
{
    nsresult rv;
    nsCOMPtr<nsIStringInputStream> stream =
             do_CreateInstance("@mozilla.org/io/string-input-stream;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 bytesToWrite = ZIP_BUFLEN - mZstream.avail_out;
    stream->ShareData((char*)mWriteBuffer, bytesToWrite);
    rv = mListener->OnDataAvailable(aRequest, mContext, stream, mOffset,
                                    bytesToWrite);

    
    mZstream.next_out = mWriteBuffer;
    mZstream.avail_out = sizeof(mWriteBuffer);

    mOffset += bytesToWrite;
    return rv;
}






#include "StreamFunctions.h"
#include "nsDeflateConverter.h"
#include "nsStringStream.h"
#include "nsIInputStreamPump.h"
#include "nsComponentManagerUtils.h"
#include "nsMemory.h"
#include "nsAutoPtr.h"
#include "plstr.h"

#define ZLIB_TYPE "deflate"
#define GZIP_TYPE "gzip"
#define X_GZIP_TYPE "x-gzip"





NS_IMPL_ISUPPORTS(nsDeflateConverter, nsIStreamConverter,
                  nsIStreamListener,
                  nsIRequestObserver)

nsresult nsDeflateConverter::Init()
{
    int zerr;

    mOffset = 0;

    mZstream.zalloc = Z_NULL;
    mZstream.zfree = Z_NULL;
    mZstream.opaque = Z_NULL;
    
    int32_t window = MAX_WBITS;
    switch (mWrapMode) {
        case WRAP_NONE:
            window = -window;
            break;
        case WRAP_GZIP:
            window += 16;
            break;
        default:
            break;
    }

    zerr = deflateInit2(&mZstream, mLevel, Z_DEFLATED, window, 8,
                        Z_DEFAULT_STRATEGY);
    if (zerr != Z_OK) return NS_ERROR_OUT_OF_MEMORY;

    mZstream.next_out = mWriteBuffer;
    mZstream.avail_out = sizeof(mWriteBuffer);

    
    mZstream.avail_in = 0;
    mZstream.next_in = Z_NULL;

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

    if (!PL_strncasecmp(aToType, ZLIB_TYPE, sizeof(ZLIB_TYPE)-1))
        mWrapMode = WRAP_ZLIB;
    else if (!PL_strcasecmp(aToType, GZIP_TYPE) ||
             !PL_strcasecmp(aToType, X_GZIP_TYPE))
        mWrapMode = WRAP_GZIP;
    else
        mWrapMode = WRAP_NONE;

    nsresult rv = Init();
    NS_ENSURE_SUCCESS(rv, rv);

    mListener = aListener;
    mContext = aCtxt;
    return rv;
}





NS_IMETHODIMP nsDeflateConverter::OnDataAvailable(nsIRequest *aRequest,
                                                  nsISupports *aContext,
                                                  nsIInputStream *aInputStream,
                                                  uint64_t aOffset,
                                                  uint32_t aCount)
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
    uint32_t bytesToWrite = sizeof(mWriteBuffer) - mZstream.avail_out;
    
    if (bytesToWrite == 0)
        return NS_OK;

    MOZ_ASSERT(bytesToWrite <= INT32_MAX);
    nsCOMPtr<nsIInputStream> stream;
    nsresult rv = NS_NewByteInputStream(getter_AddRefs(stream),
					(char*)mWriteBuffer, bytesToWrite);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mListener->OnDataAvailable(aRequest, mContext, stream, mOffset,
                                    bytesToWrite);

    
    mZstream.next_out = mWriteBuffer;
    mZstream.avail_out = sizeof(mWriteBuffer);

    mOffset += bytesToWrite;
    return rv;
}

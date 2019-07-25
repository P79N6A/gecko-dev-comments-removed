#include "Converters.h"
#include "nsIStringStream.h"
#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"

#include <stdio.h>





NS_IMPL_ISUPPORTS3(TestConverter,
                   nsIStreamConverter,
                   nsIStreamListener,
                   nsIRequestObserver)

TestConverter::TestConverter() {
}




NS_IMETHODIMP
TestConverter::Convert(nsIInputStream *aFromStream, 
                       const char *aFromType, 
                       const char *aToType, 
                       nsISupports *ctxt, 
                       nsIInputStream **_retval) {
    char buf[1024+1];
    PRUint32 read;
    nsresult rv = aFromStream->Read(buf, 1024, &read);
    if (NS_FAILED(rv) || read == 0) return rv;

    
    
    char fromChar = *aFromType;

    if (fromChar != buf[0]) {
        printf("We're receiving %c, but are supposed to have %c.\n", buf[0], fromChar);
        return NS_ERROR_FAILURE;
    }


    
    char toChar = *aToType;

    for (PRUint32 i = 0; i < read; i++) 
        buf[i] = toChar;

    buf[read] = '\0';

    nsCOMPtr<nsIStringInputStream> str
      (do_CreateInstance("@mozilla.org/io/string-input-stream;1", &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = str->SetData(buf, read);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ADDREF(*_retval = str);
    return NS_OK;
}



NS_IMETHODIMP
TestConverter::AsyncConvertData(const char *aFromType,
                                const char *aToType, 
                                nsIStreamListener *aListener, 
                                nsISupports *ctxt) {
    NS_ASSERTION(aListener, "null listener");

    mListener = aListener;

    
    fromType = aFromType;
    toType = aToType;

    return NS_OK; 
}

static inline PRUint32
saturated(PRUint64 aValue)
{
    return (PRUint32) NS_MIN(aValue, (PRUint64) PR_UINT32_MAX);
}



NS_IMETHODIMP
TestConverter::OnDataAvailable(nsIRequest* request,
                               nsISupports *ctxt, 
                               nsIInputStream *inStr, 
                               PRUint32 sourceOffset, 
                               PRUint32 count) {
    nsresult rv;
    nsCOMPtr<nsIInputStream> convertedStream;
    
    
    
    rv = Convert(inStr, fromType.get(), toType.get(), ctxt, getter_AddRefs(convertedStream));
    if (NS_FAILED(rv)) return rv;

    PRUint64 len = 0;
    convertedStream->Available(&len);

    PRUint64 offset = sourceOffset;
    while (len > 0) {
        PRUint32 count = saturated(len);
        rv = mListener->OnDataAvailable(request, ctxt, convertedStream, saturated(offset), count);
        if (NS_FAILED(rv)) return rv;

        offset += count;
        len -= count;
    }
    return NS_OK;
}



NS_IMETHODIMP
TestConverter::OnStartRequest(nsIRequest* request, nsISupports *ctxt) {
    return mListener->OnStartRequest(request, ctxt);
}

NS_IMETHODIMP
TestConverter::OnStopRequest(nsIRequest* request, nsISupports *ctxt, 
                             nsresult aStatus) {
    return mListener->OnStopRequest(request, ctxt, aStatus);
}

nsresult
CreateTestConverter(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  nsCOMPtr<nsISupports> conv = new TestConverter();
  return conv->QueryInterface(aIID, aResult);
}






































#include "nsScriptableInputStream.h"
#include "nsMemory.h"
#include "nsString.h"

NS_IMPL_ISUPPORTS1(nsScriptableInputStream, nsIScriptableInputStream)


NS_IMETHODIMP
nsScriptableInputStream::Close(void) {
    if (!mInputStream) return NS_ERROR_NOT_INITIALIZED;
    return mInputStream->Close();
}


NS_IMETHODIMP
nsScriptableInputStream::Init(nsIInputStream *aInputStream) {
    if (!aInputStream) return NS_ERROR_NULL_POINTER;
    mInputStream = aInputStream;
    return NS_OK;
}

NS_IMETHODIMP
nsScriptableInputStream::Available(PRUint32 *_retval) {
    if (!mInputStream) return NS_ERROR_NOT_INITIALIZED;
    return mInputStream->Available(_retval);
}

NS_IMETHODIMP
nsScriptableInputStream::Read(PRUint32 aCount, char **_retval) {
    nsresult rv = NS_OK;
    PRUint32 count = 0;
    char *buffer = nsnull;

    if (!mInputStream) return NS_ERROR_NOT_INITIALIZED;

    rv = mInputStream->Available(&count);
    if (NS_FAILED(rv)) return rv;

    count = NS_MIN(count, aCount);
    buffer = (char*)nsMemory::Alloc(count+1); 
    if (!buffer) return NS_ERROR_OUT_OF_MEMORY;

    PRUint32 amtRead = 0;
    rv = mInputStream->Read(buffer, count, &amtRead);
    if (NS_FAILED(rv)) {
        nsMemory::Free(buffer);
        return rv;
    }

    buffer[amtRead] = '\0';
    *_retval = buffer;
    return NS_OK;
}

NS_IMETHODIMP
nsScriptableInputStream::ReadBytes(PRUint32 aCount, nsACString &_retval) {
    if (!mInputStream) {
      return NS_ERROR_NOT_INITIALIZED;
    }

    _retval.SetLength(aCount);
    if (_retval.Length() != aCount) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    char *ptr = _retval.BeginWriting();
    PRUint32 totalBytesRead = 0;
    while (1) {
      PRUint32 bytesRead;
      nsresult rv = mInputStream->Read(ptr + totalBytesRead,
                                       aCount - totalBytesRead,
                                       &bytesRead);
      if (NS_FAILED(rv)) {
        return rv;
      }

      totalBytesRead += bytesRead;
      if (totalBytesRead == aCount) {
        break;
      }

      
      if (bytesRead == 0) {
        _retval.Truncate();
        return NS_ERROR_FAILURE;
      }

    }
    return NS_OK;
}

nsresult
nsScriptableInputStream::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult) {
    if (aOuter) return NS_ERROR_NO_AGGREGATION;

    nsScriptableInputStream *sis = new nsScriptableInputStream();
    if (!sis) return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(sis);
    nsresult rv = sis->QueryInterface(aIID, aResult);
    NS_RELEASE(sis);
    return rv;
}

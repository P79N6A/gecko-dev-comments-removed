





































#include "nsStartupCacheUtils.h"

#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsIInputStream.h"
#include "nsIStorageStream.h"
#include "nsIStringStream.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"

nsresult
NS_NewObjectInputStreamFromBuffer(char* buffer, int len, 
                                  nsIObjectInputStream** stream)
{
  nsCOMPtr<nsIStringInputStream> stringStream
    = do_CreateInstance("@mozilla.org/io/string-input-stream;1");
  if (!stringStream)
    return NS_ERROR_OUT_OF_MEMORY;
  nsCOMPtr<nsIObjectInputStream> objectInput 
    = do_CreateInstance("@mozilla.org/binaryinputstream;1");
  if (!objectInput)
    return NS_ERROR_OUT_OF_MEMORY;

  stringStream->AdoptData(buffer, len);
  objectInput->SetInputStream(stringStream);

  NS_ADDREF(*stream = objectInput);
  return NS_OK;
}



nsresult
NS_NewObjectOutputWrappedStorageStream(nsIObjectOutputStream **wrapperStream,
                                       nsIStorageStream** stream)
{
  nsCOMPtr<nsIStorageStream> storageStream;
  nsresult rv = NS_NewStorageStream(256, (PRUint32)-1, 
                                    getter_AddRefs(storageStream));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIObjectOutputStream> objectOutput
    = do_CreateInstance("@mozilla.org/binaryoutputstream;1");
  if (!objectOutput)
    return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsIOutputStream> outputStream
    = do_QueryInterface(storageStream);

  objectOutput->SetOutputStream(outputStream);
  NS_ADDREF(*wrapperStream = objectOutput);
  NS_ADDREF(*stream = storageStream);
  return NS_OK;
}

nsresult
NS_NewBufferFromStorageStream(nsIStorageStream *storageStream, 
                              char** buffer, int* len)
{
  nsresult rv;
  nsCOMPtr<nsIInputStream> inputStream;
  rv = storageStream->NewInputStream(0, getter_AddRefs(inputStream));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 avail, read;
  rv = inputStream->Available(&avail);
  NS_ENSURE_SUCCESS(rv, rv);

  char* temp = new char[avail];
  if (!temp)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = inputStream->Read(temp, avail, &read);
  if (NS_SUCCEEDED(rv) && avail != read)
    rv = NS_ERROR_UNEXPECTED;

  if (NS_FAILED(rv)) {
    delete temp;
    return rv;
  }

  *len = avail;
  *buffer = temp;
  return NS_OK;
}


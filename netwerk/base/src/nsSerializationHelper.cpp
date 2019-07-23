




































#include "nsSerializationHelper.h"

#include "plbase64.h"
#include "prmem.h"

#include "nsISerializable.h"
#include "nsIObjectOutputStream.h"
#include "nsIObjectInputStream.h"
#include "nsString.h"
#include "nsBase64Encoder.h"
#include "nsAutoPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsStringStream.h"

nsresult
NS_SerializeToString(nsISerializable* obj, nsCSubstring& str)
{
  nsRefPtr<nsBase64Encoder> stream(new nsBase64Encoder());
  if (!stream)
    return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsIObjectOutputStream> objstream =
      do_CreateInstance("@mozilla.org/binaryoutputstream;1");
  if (!objstream)
    return NS_ERROR_OUT_OF_MEMORY;

  objstream->SetOutputStream(stream);
  nsresult rv =
      objstream->WriteCompoundObject(obj, NS_GET_IID(nsISupports), PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);
  return stream->Finish(str);
}

nsresult
NS_DeserializeObject(const nsCSubstring& str, nsISupports** obj)
{
  
  
  
  
  
  
  

  PRUint32 size = str.Length();
  if (size > 0 && str[size-1] == '=') {
    if (size > 1 && str[size-2] == '=') {
      size -= 2;
    } else {
      size -= 1;
    }
  }
  size = (size * 3) / 4;
  char* buf = PL_Base64Decode(str.BeginReading(), str.Length(), nsnull);
  if (!buf)
    return NS_ERROR_OUT_OF_MEMORY;
  nsCOMPtr<nsIInputStream> stream;
  nsresult rv = NS_NewCStringInputStream(getter_AddRefs(stream),
                                         Substring(buf, buf + size));
  PR_Free(buf);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIObjectInputStream> objstream =
      do_CreateInstance("@mozilla.org/binaryinputstream;1");
  if (!objstream)
    return NS_ERROR_OUT_OF_MEMORY;

  objstream->SetInputStream(stream);
  return objstream->ReadObject(PR_TRUE, obj);
}

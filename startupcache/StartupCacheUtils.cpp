
#include "nsCOMPtr.h"
#include "nsIInputStream.h"
#include "nsIStringStream.h"
#include "nsNetUtil.h"
#include "nsIJARURI.h"
#include "nsIResProtocolHandler.h"
#include "nsAutoPtr.h"
#include "StartupCacheUtils.h"
#include "mozilla/scache/StartupCache.h"
#include "mozilla/Omnijar.h"

namespace mozilla {
namespace scache {

NS_EXPORT nsresult
NS_NewObjectInputStreamFromBuffer(char* buffer, PRUint32 len, 
                                  nsIObjectInputStream** stream)
{
  nsCOMPtr<nsIStringInputStream> stringStream
    = do_CreateInstance("@mozilla.org/io/string-input-stream;1");
  nsCOMPtr<nsIObjectInputStream> objectInput 
    = do_CreateInstance("@mozilla.org/binaryinputstream;1");
  
  stringStream->AdoptData(buffer, len);
  objectInput->SetInputStream(stringStream);
  
  objectInput.forget(stream);
  return NS_OK;
}

NS_EXPORT nsresult
NS_NewObjectOutputWrappedStorageStream(nsIObjectOutputStream **wrapperStream,
                                       nsIStorageStream** stream)
{
  nsCOMPtr<nsIStorageStream> storageStream;

  nsresult rv = NS_NewStorageStream(256, PR_UINT32_MAX, getter_AddRefs(storageStream));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIObjectOutputStream> objectOutput
    = do_CreateInstance("@mozilla.org/binaryoutputstream;1");
  nsCOMPtr<nsIOutputStream> outputStream
    = do_QueryInterface(storageStream);
  
  objectOutput->SetOutputStream(outputStream);
  
#ifdef DEBUG
  
  
  StartupCache* sc = StartupCache::GetSingleton();
  NS_ENSURE_TRUE(sc, NS_ERROR_UNEXPECTED);
  nsCOMPtr<nsIObjectOutputStream> debugStream;
  sc->GetDebugObjectOutputStream(objectOutput, getter_AddRefs(debugStream));
  debugStream.forget(wrapperStream);
#else
  objectOutput.forget(wrapperStream);
#endif
  
  storageStream.forget(stream);
  return NS_OK;
}

NS_EXPORT nsresult
NS_NewBufferFromStorageStream(nsIStorageStream *storageStream, 
                              char** buffer, PRUint32* len) 
{
  nsresult rv;
  nsCOMPtr<nsIInputStream> inputStream;
  rv = storageStream->NewInputStream(0, getter_AddRefs(inputStream));
  NS_ENSURE_SUCCESS(rv, rv);
  
  PRUint32 avail, read;
  rv = inputStream->Available(&avail);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsAutoArrayPtr<char> temp (new char[avail]);
  rv = inputStream->Read(temp, avail, &read);
  if (NS_SUCCEEDED(rv) && avail != read)
    rv = NS_ERROR_UNEXPECTED;
  
  if (NS_FAILED(rv)) {
    return rv;
  }
  
  *len = avail;
  *buffer = temp.forget();
  return NS_OK;
}

static const char baseName[2][5] = { "gre/", "app/" };

static inline PRBool
canonicalizeBase(nsCAutoString &spec,
                 nsACString &out,
                 mozilla::Omnijar::Type aType)
{
    nsCAutoString base;
    nsresult rv = mozilla::Omnijar::GetURIString(aType, base);

    if (NS_FAILED(rv) || !base.Length())
        return PR_FALSE;

    if (base.Compare(spec.get(), PR_FALSE, base.Length()))
        return PR_FALSE;

    out.Append("/resource/");
    out.Append(baseName[aType]);
    out.Append(Substring(spec, base.Length()));
    return PR_TRUE;
}


























NS_EXPORT nsresult
NS_PathifyURI(nsIURI *in, nsACString &out)
{
    PRBool equals;
    nsresult rv;
    nsCOMPtr<nsIURI> uri = in;
    nsCAutoString spec;

    
    
    if (NS_SUCCEEDED(in->SchemeIs("resource", &equals)) && equals) {
        nsCOMPtr<nsIIOService> ioService = do_GetIOService(&rv);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<nsIProtocolHandler> ph;
        rv = ioService->GetProtocolHandler("resource", getter_AddRefs(ph));
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<nsIResProtocolHandler> irph(do_QueryInterface(ph, &rv));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = irph->ResolveURI(in, spec);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = ioService->NewURI(spec, nsnull, nsnull, getter_AddRefs(uri));
        NS_ENSURE_SUCCESS(rv, rv);
    } else {
        rv = in->GetSpec(spec);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    if (!canonicalizeBase(spec, out, mozilla::Omnijar::GRE) &&
        !canonicalizeBase(spec, out, mozilla::Omnijar::APP)) {
        if (NS_SUCCEEDED(uri->SchemeIs("file", &equals)) && equals) {
            nsCOMPtr<nsIFileURL> baseFileURL;
            baseFileURL = do_QueryInterface(uri, &rv);
            NS_ENSURE_SUCCESS(rv, rv);

            nsCAutoString path;
            rv = baseFileURL->GetPath(path);
            NS_ENSURE_SUCCESS(rv, rv);

            out.Append(path);
        } else if (NS_SUCCEEDED(uri->SchemeIs("jar", &equals)) && equals) {
            nsCOMPtr<nsIJARURI> jarURI = do_QueryInterface(uri, &rv);
            NS_ENSURE_SUCCESS(rv, rv);

            nsCOMPtr<nsIURI> jarFileURI;
            rv = jarURI->GetJARFile(getter_AddRefs(jarFileURI));
            NS_ENSURE_SUCCESS(rv, rv);

            nsCOMPtr<nsIFileURL> jarFileURL;
            jarFileURL = do_QueryInterface(jarFileURI, &rv);
            NS_ENSURE_SUCCESS(rv, rv);

            nsCAutoString path;
            rv = jarFileURL->GetPath(path);
            NS_ENSURE_SUCCESS(rv, rv);
            out.Append(path);

            rv = jarURI->GetJAREntry(path);
            NS_ENSURE_SUCCESS(rv, rv);
            out.Append("/");
            out.Append(path);
        } else { 
            nsCAutoString spec;
            rv = uri->GetSpec(spec);
            NS_ENSURE_SUCCESS(rv, rv);

            out.Append("/");
            out.Append(spec);
        }
    }

    out.Append(".bin");
    return NS_OK;
}

}
}

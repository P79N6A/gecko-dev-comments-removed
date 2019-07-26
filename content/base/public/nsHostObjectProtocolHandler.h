



#ifndef nsHostObjectProtocolHandler_h
#define nsHostObjectProtocolHandler_h

#include "nsIProtocolHandler.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"

#define BLOBURI_SCHEME "blob"

class nsIDOMBlob;
class nsIPrincipal;
class nsIInputStream;

class nsHostObjectProtocolHandler : public nsIProtocolHandler
{
public:
  NS_DECL_ISUPPORTS

  
  
  NS_IMETHOD GetDefaultPort(int32_t *aDefaultPort);
  NS_IMETHOD GetProtocolFlags(uint32_t *aProtocolFlags);
  NS_IMETHOD NewURI(const nsACString & aSpec, const char * aOriginCharset, nsIURI *aBaseURI, nsIURI * *_retval);
  NS_IMETHOD NewChannel(nsIURI *aURI, nsIChannel * *_retval);
  NS_IMETHOD AllowPort(int32_t port, const char * scheme, bool *_retval);

  
  
  static nsresult AddDataEntry(const nsACString& aScheme,
                               nsISupports* aObject,
                               nsIPrincipal* aPrincipal,
                               nsACString& aUri);
  static void RemoveDataEntry(const nsACString& aUri);
  static nsIPrincipal* GetDataEntryPrincipal(const nsACString& aUri);
};

class nsBlobProtocolHandler : public nsHostObjectProtocolHandler
{
public:
  NS_IMETHOD GetScheme(nsACString &result);
};

inline bool IsBlobURI(nsIURI* aUri)
{
  bool isBlob;
  return NS_SUCCEEDED(aUri->SchemeIs(BLOBURI_SCHEME, &isBlob)) && isBlob;
}

extern nsresult
NS_GetStreamForBlobURI(nsIURI* aURI, nsIInputStream** aStream);

#define NS_BLOBPROTOCOLHANDLER_CID \
{ 0xb43964aa, 0xa078, 0x44b2, \
  { 0xb0, 0x6b, 0xfd, 0x4d, 0x1b, 0x17, 0x2e, 0x66 } }

#endif 

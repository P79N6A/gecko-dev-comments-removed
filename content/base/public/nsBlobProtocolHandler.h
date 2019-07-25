



#ifndef nsBlobProtocolHandler_h
#define nsBlobProtocolHandler_h

#include "nsIProtocolHandler.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"

#define BLOBURI_SCHEME "blob"

class nsIDOMBlob;
class nsIPrincipal;
class nsIInputStream;

inline bool IsBlobURI(nsIURI* aUri)
{
  bool isBlob;
  return NS_SUCCEEDED(aUri->SchemeIs(BLOBURI_SCHEME, &isBlob)) && isBlob;
}

extern nsresult
NS_GetStreamForBlobURI(nsIURI* aURI, nsIInputStream** aStream);

class nsBlobProtocolHandler : public nsIProtocolHandler
{
public:
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIPROTOCOLHANDLER

  
  nsBlobProtocolHandler() {}
  virtual ~nsBlobProtocolHandler() {}

  
  static void AddFileDataEntry(nsACString& aUri,
                               nsIDOMBlob* aFile,
                               nsIPrincipal* aPrincipal);
  static void RemoveFileDataEntry(nsACString& aUri);
  static nsIPrincipal* GetFileDataEntryPrincipal(nsACString& aUri);
};

#define NS_BLOBPROTOCOLHANDLER_CID \
{ 0xb43964aa, 0xa078, 0x44b2, \
  { 0xb0, 0x6b, 0xfd, 0x4d, 0x1b, 0x17, 0x2e, 0x66 } }

#endif 

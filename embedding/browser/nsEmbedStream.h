




#ifndef nsEmbedStream_h__
#define nsEmbedStream_h__

#include "nsCOMPtr.h"
#include "nsIOutputStream.h"
#include "nsIURI.h"
#include "nsIWebBrowser.h"

class nsEmbedStream : public nsISupports
{
 public:

  nsEmbedStream();

  void      InitOwner      (nsIWebBrowser *aOwner);
  NS_METHOD Init           (void);

  NS_METHOD OpenStream     (nsIURI *aBaseURI, const nsACString& aContentType);
  NS_METHOD AppendToStream (const uint8_t *aData, uint32_t aLen);
  NS_METHOD CloseStream    (void);

  NS_DECL_ISUPPORTS

 protected:
  virtual ~nsEmbedStream();

 private:
  nsIWebBrowser            *mOwner;
  nsCOMPtr<nsIOutputStream> mOutputStream;
};

#endif 

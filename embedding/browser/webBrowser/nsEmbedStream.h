




#include "nsCOMPtr.h"
#include "nsIOutputStream.h"
#include "nsIURI.h"
#include "nsIWebBrowser.h"

class nsEmbedStream : public nsISupports
{
 public:

  nsEmbedStream();
  virtual ~nsEmbedStream();

  void      InitOwner      (nsIWebBrowser *aOwner);
  NS_METHOD Init           (void);

  NS_METHOD OpenStream     (nsIURI *aBaseURI, const nsACString& aContentType);
  NS_METHOD AppendToStream (const uint8_t *aData, uint32_t aLen);
  NS_METHOD CloseStream    (void);

  NS_DECL_ISUPPORTS

 private:
  nsIWebBrowser            *mOwner;
  nsCOMPtr<nsIOutputStream> mOutputStream;

};

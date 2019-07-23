




































#include <nsISupports.h>
#include <nsCOMPtr.h>
#include <nsIOutputStream.h>
#include <nsIInputStream.h>
#include <nsILoadGroup.h>
#include <nsIChannel.h>
#include <nsIStreamListener.h>
#include <nsIWebBrowser.h>

class nsEmbedStream : public nsIInputStream 
{
 public:

  nsEmbedStream();
  virtual ~nsEmbedStream();

  void      InitOwner      (nsIWebBrowser *aOwner);
  NS_METHOD Init           (void);

  NS_METHOD OpenStream     (nsIURI *aBaseURI, const nsACString& aContentType);
  NS_METHOD AppendToStream (const PRUint8 *aData, PRUint32 aLen);
  NS_METHOD CloseStream    (void);

  NS_METHOD Append         (const PRUint8 *aData, PRUint32 aLen);

  
  NS_DECL_ISUPPORTS
  
  NS_DECL_NSIINPUTSTREAM

 private:
  nsCOMPtr<nsIOutputStream>   mOutputStream;
  nsCOMPtr<nsIInputStream>    mInputStream;

  nsCOMPtr<nsILoadGroup>      mLoadGroup;
  nsCOMPtr<nsIChannel>        mChannel;
  nsCOMPtr<nsIStreamListener> mStreamListener;

  PRUint32                    mOffset;
  PRBool                      mDoingStream;

  nsIWebBrowser              *mOwner;

};

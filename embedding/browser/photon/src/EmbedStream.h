




































#include <nsISupports.h>
#include <nsCOMPtr.h>
#include <nsIOutputStream.h>
#include <nsIInputStream.h>
#include <nsILoadGroup.h>
#include <nsIChannel.h>
#include <nsIStreamListener.h>

class EmbedPrivate;
  
class EmbedStream : public nsIInputStream 
{
 public:

  EmbedStream();
  virtual ~EmbedStream();

  void      InitOwner      (EmbedPrivate *aOwner);
  NS_METHOD Init           (void);

  NS_METHOD OpenStream     (const char *aBaseURI, const char *aContentType);
  NS_METHOD AppendToStream (const char *aData, PRInt32 aLen);
  NS_METHOD CloseStream    (void);

  NS_METHOD Append         (const char *aData, PRUint32 aLen);

  
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

  EmbedPrivate *mOwner;

};

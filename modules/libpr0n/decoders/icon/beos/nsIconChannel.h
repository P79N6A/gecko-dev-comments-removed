






































#ifndef nsIconChannel_h___
#define nsIconChannel_h___

#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "nsIChannel.h"
#include "nsILoadGroup.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIURI.h"
#include "nsIInputStreamPump.h"
#include "nsIStreamListener.h"

class nsIFile;

class nsIconChannel : public nsIChannel, public nsIStreamListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUEST
  NS_DECL_NSICHANNEL
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

  nsIconChannel();
  ~nsIconChannel();

  nsresult Init(nsIURI* uri);

protected:
  nsCOMPtr<nsIURI> mUrl;
  nsCOMPtr<nsIURI> mOriginalURI;
  PRInt64          mContentLength;
  nsCOMPtr<nsILoadGroup> mLoadGroup;
  nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
  nsCOMPtr<nsISupports>  mOwner; 

  nsCOMPtr<nsIInputStreamPump> mPump;
  nsCOMPtr<nsIStreamListener>  mListener;

  nsresult ExtractIconInfoFromUrl(nsIFile ** aLocalFile, PRUint32 * aDesiredImageSize, nsACString &aContentType, nsACString &aFileExtension);
  nsresult MakeInputStream(nsIInputStream** _retval, PRBool nonBlocking);
};

#endif 

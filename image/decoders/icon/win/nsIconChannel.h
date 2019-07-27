





#ifndef nsIconChannel_h___
#define nsIconChannel_h___

#include "mozilla/Attributes.h"

#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "nsIChannel.h"
#include "nsILoadGroup.h"
#include "nsILoadInfo.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIURI.h"
#include "nsIInputStreamPump.h"
#include "nsIStreamListener.h"
#include "nsIIconURI.h"

#include <windows.h>

class nsIFile;

class nsIconChannel MOZ_FINAL : public nsIChannel, public nsIStreamListener
{
  ~nsIconChannel();

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIREQUEST
  NS_DECL_NSICHANNEL
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

  nsIconChannel();

  nsresult Init(nsIURI* uri);

protected:
  nsCOMPtr<nsIURI> mUrl;
  nsCOMPtr<nsIURI> mOriginalURI;
  int64_t          mContentLength;
  nsCOMPtr<nsILoadGroup> mLoadGroup;
  nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
  nsCOMPtr<nsISupports>  mOwner; 
  nsCOMPtr<nsILoadInfo>  mLoadInfo;

  nsCOMPtr<nsIInputStreamPump> mPump;
  nsCOMPtr<nsIStreamListener>  mListener;

  nsresult ExtractIconInfoFromUrl(nsIFile ** aLocalFile, uint32_t * aDesiredImageSize, nsCString &aContentType, nsCString &aFileExtension);
  nsresult GetHIconFromFile(HICON *hIcon);
  nsresult MakeInputStream(nsIInputStream** _retval, bool nonBlocking);

  
protected:
  nsresult GetStockHIcon(nsIMozIconURI *aIconURI, HICON *hIcon);
};

#endif 








































#include "imgILoader.h"
#include "nsIContentSniffer.h"

#ifdef LOADER_THREADSAFE
#include "prlock.h"
#endif

class imgRequest;
class imgRequestProxy;
class imgIRequest;
class imgIDecoderObserver;
class nsILoadGroup;

#define NS_IMGLOADER_CID \
{ /* 9f6a0d2e-1dd1-11b2-a5b8-951f13c846f7 */         \
     0x9f6a0d2e,                                     \
     0x1dd1,                                         \
     0x11b2,                                         \
    {0xa5, 0xb8, 0x95, 0x1f, 0x13, 0xc8, 0x46, 0xf7} \
}

class imgLoader : public imgILoader, public nsIContentSniffer
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGILOADER
  NS_DECL_NSICONTENTSNIFFER

  imgLoader();
  virtual ~imgLoader();

  static nsresult GetMimeTypeFromContent(const char* aContents, PRUint32 aLength, nsACString& aContentType);

private:
  nsresult CreateNewProxyForRequest(imgRequest *aRequest, nsILoadGroup *aLoadGroup,
                                    imgIDecoderObserver *aObserver,
                                    nsLoadFlags aLoadFlags, imgIRequest *aRequestProxy,
                                    imgIRequest **_retval);
};







#include "nsCOMPtr.h"
#include "nsIStreamListener.h"

class ProxyListener : public nsIStreamListener
{
public:
  ProxyListener(nsIStreamListener *dest);
  virtual ~ProxyListener();

  
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER

private:
  nsCOMPtr<nsIStreamListener> mDestListener;
};






#include "nsCOMArray.h"

class imgCacheValidator : public nsIStreamListener
{
public:
  imgCacheValidator(imgRequest *request, void *aContext);
  virtual ~imgCacheValidator();

  void AddProxy(imgRequestProxy *aProxy);

  
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER

private:
  nsCOMPtr<nsIStreamListener> mDestListener;

  imgRequest *mRequest;
  nsCOMArray<imgIRequest> mProxies;

  void *mContext;
};

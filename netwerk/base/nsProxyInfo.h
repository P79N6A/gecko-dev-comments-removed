





#ifndef nsProxyInfo_h__
#define nsProxyInfo_h__

#include "nsIProxyInfo.h"
#include "nsString.h"
#include "mozilla/Attributes.h"


#define NS_PROXYINFO_IID \
{ /* ed42f751-825e-4cc2-abeb-3670711a8b85 */         \
    0xed42f751,                                      \
    0x825e,                                          \
    0x4cc2,                                          \
    {0xab, 0xeb, 0x36, 0x70, 0x71, 0x1a, 0x8b, 0x85} \
}



class nsProxyInfo MOZ_FINAL : public nsIProxyInfo
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_PROXYINFO_IID)

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIPROXYINFO

  
  const nsCString &Host()  { return mHost; }
  int32_t          Port()  { return mPort; }
  const char      *Type()  { return mType; }
  uint32_t         Flags() { return mFlags; }

  bool IsDirect();
  bool IsHTTP();
  bool IsHTTPS();
  bool IsSOCKS();

private:
  friend class nsProtocolProxyService;

  explicit nsProxyInfo(const char *type = nullptr)
    : mType(type)
    , mPort(-1)
    , mFlags(0)
    , mResolveFlags(0)
    , mTimeout(UINT32_MAX)
    , mNext(nullptr)
  {}

  ~nsProxyInfo()
  {
    NS_IF_RELEASE(mNext);
  }

  const char  *mType;  
  nsCString    mHost;
  int32_t      mPort;
  uint32_t     mFlags;
  uint32_t     mResolveFlags;
  uint32_t     mTimeout;
  nsProxyInfo *mNext;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsProxyInfo, NS_PROXYINFO_IID)

#endif 

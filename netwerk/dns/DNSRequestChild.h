





#ifndef mozilla_net_DNSRequestChild_h
#define mozilla_net_DNSRequestChild_h

#include "mozilla/net/PDNSRequestChild.h"
#include "nsICancelable.h"
#include "nsIDNSRecord.h"
#include "nsIDNSListener.h"
#include "nsIEventTarget.h"

namespace mozilla {
namespace net {

class DNSRequestChild
  : public PDNSRequestChild
  , public nsICancelable
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSICANCELABLE

  DNSRequestChild(const nsCString& aHost, const uint32_t& aFlags,
                  nsIDNSListener *aListener, nsIEventTarget *target);

  void AddIPDLReference() {
    AddRef();
  }
  void ReleaseIPDLReference() {
    
    
    
    Release();
  }

  
  void StartRequest();
  void CallOnLookupComplete();

private:
  virtual ~DNSRequestChild() {}

  virtual bool Recv__delete__(const DNSRequestResponse& reply) MOZ_OVERRIDE;

  nsCOMPtr<nsIDNSListener>  mListener;
  nsCOMPtr<nsIEventTarget>  mTarget;
  nsCOMPtr<nsIDNSRecord>    mResultRecord;
  nsresult                  mResultStatus;
  nsCString                 mHost;
  uint16_t                  mFlags;
};

} 
} 
#endif 

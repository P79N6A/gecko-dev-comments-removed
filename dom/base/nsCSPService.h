




#ifndef nsCSPService_h___
#define nsCSPService_h___

#include "nsXPCOM.h"
#include "nsIContentPolicy.h"
#include "nsIChannel.h"
#include "nsIChannelEventSink.h"
#include "nsDataHashtable.h"

#define CSPSERVICE_CONTRACTID "@mozilla.org/cspservice;1"
#define CSPSERVICE_CID \
  { 0x8d2f40b2, 0x4875, 0x4c95, \
    { 0x97, 0xd9, 0x3f, 0x7d, 0xca, 0x2c, 0xb4, 0x60 } }
class CSPService : public nsIContentPolicy,
                   public nsIChannelEventSink
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICONTENTPOLICY
  NS_DECL_NSICHANNELEVENTSINK

  CSPService();
  static bool sCSPEnabled;

protected:
  virtual ~CSPService();

private:
  
  nsDataHashtable<nsCStringHashKey, uint16_t> mAppStatusCache;
};
#endif 

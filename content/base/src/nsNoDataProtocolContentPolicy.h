










































#ifndef nsNoDataProtocolContentPolicy_h__
#define nsNoDataProtocolContentPolicy_h__


#define NS_NODATAPROTOCOLCONTENTPOLICY_CID \
 {0xac9e3e82, 0xbfbd, 0x4f26, {0x94, 0x1e, 0xf5, 0x8c, 0x8e, 0xe1, 0x78, 0xc1}}
#define NS_NODATAPROTOCOLCONTENTPOLICY_CONTRACTID \
 "@mozilla.org/no-data-protocol-content-policy;1"


#include "nsIContentPolicy.h"

class nsNoDataProtocolContentPolicy : public nsIContentPolicy
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICONTENTPOLICY

  nsNoDataProtocolContentPolicy()
  {}
  ~nsNoDataProtocolContentPolicy()
  {}
};


#endif 

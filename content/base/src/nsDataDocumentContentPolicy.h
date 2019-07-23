










































#ifndef nsDataDocumentContentPolicy_h__
#define nsDataDocumentContentPolicy_h__


#define NS_DATADOCUMENTCONTENTPOLICY_CID \
 {0x1147d32c, 0x215b, 0x4014, {0xb1, 0x80, 0x07, 0xfe, 0x7a, 0xed, 0xf9, 0x15}}
#define NS_DATADOCUMENTCONTENTPOLICY_CONTRACTID \
 "@mozilla.org/data-document-content-policy;1"


#include "nsIContentPolicy.h"

class nsDataDocumentContentPolicy : public nsIContentPolicy
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICONTENTPOLICY

  nsDataDocumentContentPolicy()
  {}
  ~nsDataDocumentContentPolicy()
  {}
};


#endif 






#ifndef nsMixedContentBlocker_h___
#define nsMixedContentBlocker_h___

#define NS_MIXEDCONTENTBLOCKER_CONTRACTID "@mozilla.org/mixedcontentblocker;1"

#define NS_MIXEDCONTENTBLOCKER_CID \
{ 0xdaf1461b, 0xbf29, 0x4f88, \
  { 0x8d, 0x0e, 0x4b, 0xcd, 0xf3, 0x32, 0xc8, 0x62 } }



enum MixedContentBlockedTypes {
  
  
  eBlockedMixedScript,
  
  eBlockedMixedDisplay
};

#include "nsIContentPolicy.h"

class nsMixedContentBlocker : public nsIContentPolicy
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICONTENTPOLICY

  nsMixedContentBlocker();
  virtual ~nsMixedContentBlocker();
  static bool sBlockMixedScript;
  static bool sBlockMixedDisplay;
};

#endif 

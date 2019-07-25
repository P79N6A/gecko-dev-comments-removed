



#ifndef nsParserBase_h_
#define nsParserBase_h_

#include "nsIChannel.h"

class nsParserBase : public nsISupports
{
  public:
    NS_IMETHOD_(bool) IsParserEnabled() { return true; }
    NS_IMETHOD GetChannel(nsIChannel** aChannel) {
      *aChannel = nullptr;
      return NS_OK;
    }
};

#endif 

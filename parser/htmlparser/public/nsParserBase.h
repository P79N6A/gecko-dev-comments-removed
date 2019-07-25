




































#ifndef nsParserBase_h_
#define nsParserBase_h_

#include "nsIChannel.h"

class nsParserBase : public nsISupports
{
  public:
    virtual bool IsParserEnabled() { return true; }
    NS_IMETHOD GetChannel(nsIChannel** aChannel) {
      *aChannel = nsnull;
      return NS_OK;
    };
};

#endif 

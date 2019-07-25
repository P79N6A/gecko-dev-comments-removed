



































#ifndef nsParserUtils_h_
#define nsParserUtils_h_

#include "nsIScriptableUnescapeHTML.h"
#include "nsIParserUtils.h"

class nsParserUtils : public nsIScriptableUnescapeHTML,
                               public nsIParserUtils
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISCRIPTABLEUNESCAPEHTML
  NS_DECL_NSIPARSERUTILS
};

#endif

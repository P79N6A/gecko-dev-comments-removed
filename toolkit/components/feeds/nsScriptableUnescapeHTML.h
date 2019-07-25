



































#ifndef nsScriptableHTMLUnescape_h__
#define nsScriptableHTMLUnescape_h__

#include "nsIScriptableUnescapeHTML.h"
#include "nsIParserUtils.h"

class nsScriptableUnescapeHTML : public nsIScriptableUnescapeHTML,
                                 public nsIParserUtils
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISCRIPTABLEUNESCAPEHTML
  NS_DECL_NSIPARSERUTILS
};

#endif






#ifndef nsParserUtils_h
#define nsParserUtils_h

#include "nsIScriptableUnescapeHTML.h"
#include "nsIParserUtils.h"
#include "mozilla/Attributes.h"

class nsParserUtils MOZ_FINAL : public nsIScriptableUnescapeHTML,
                                public nsIParserUtils
{
  ~nsParserUtils() {}
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISCRIPTABLEUNESCAPEHTML
  NS_DECL_NSIPARSERUTILS
};

#endif

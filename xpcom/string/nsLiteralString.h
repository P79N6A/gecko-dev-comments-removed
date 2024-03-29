





#ifndef nsLiteralString_h___
#define nsLiteralString_h___

#include "nscore.h"
#include "nsString.h"


#include "string-template-def-unichar.h"
#include "nsTLiteralString.h"
#include "string-template-undef.h"


#include "string-template-def-char.h"
#include "nsTLiteralString.h"
#include "string-template-undef.h"

#include "mozilla/Char16.h"

#define NS_MULTILINE_LITERAL_STRING(s)            static_cast<const nsLiteralString&>(nsLiteralString(s))
#define NS_MULTILINE_LITERAL_STRING_INIT(n,s)     n(s)
#define NS_NAMED_MULTILINE_LITERAL_STRING(n,s)    const nsLiteralString n(s)

#define NS_LITERAL_STRING(s)                      static_cast<const nsLiteralString&>(nsLiteralString(MOZ_UTF16(s)))
#define NS_LITERAL_STRING_INIT(n,s)               n(MOZ_UTF16(s))
#define NS_NAMED_LITERAL_STRING(n,s)              const nsLiteralString n(MOZ_UTF16(s))

#define NS_LITERAL_CSTRING(s)                     static_cast<const nsLiteralCString&>(nsLiteralCString("" s))
#define NS_LITERAL_CSTRING_INIT(n,s)              n("" s)
#define NS_NAMED_LITERAL_CSTRING(n,s)             const nsLiteralCString n("" s)

#endif 

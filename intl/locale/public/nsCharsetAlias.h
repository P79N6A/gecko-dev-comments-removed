




































#ifndef nsCharsetAlias_h___
#define nsCharsetAlias_h___

#include "nscore.h"
#include "nsStringGlue.h"

class nsCharsetAlias
{
public:
   static nsresult GetPreferred(const nsACString& aAlias, nsACString& aResult);
   static nsresult Equals(const nsACString& aCharset1, const nsACString& aCharset2, bool* aResult);
};

#endif 

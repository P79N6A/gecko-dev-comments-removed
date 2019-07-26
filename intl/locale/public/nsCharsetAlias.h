




#ifndef nsCharsetAlias_h___
#define nsCharsetAlias_h___

#include "nscore.h"
#include "nsStringGlue.h"

class nsCharsetConverterManager;

class nsCharsetAlias
{
   friend class nsCharsetConverterManager;
   static nsresult GetPreferredInternal(const nsACString& aAlias, nsACString& aResult);
public:
   static nsresult GetPreferred(const nsACString& aAlias, nsACString& aResult);
   static nsresult Equals(const nsACString& aCharset1, const nsACString& aCharset2, bool* aResult);
};

#endif 

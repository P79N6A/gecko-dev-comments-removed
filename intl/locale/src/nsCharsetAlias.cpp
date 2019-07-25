




#include "mozilla/Util.h"

#include "nsCharsetAlias.h"
#include "pratom.h"


#include "nsEncoderDecoderUtils.h"
#include "nsCharsetConverterManager.h"


#include "nsIPlatformCharset.h"

#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsUConvPropertySearch.h"

using namespace mozilla;


static const char* kAliases[][3] = {
#include "charsetalias.properties.h"
};



nsresult
nsCharsetAlias::GetPreferredInternal(const nsACString& aAlias,
                                     nsACString& oResult)
{
   nsCAutoString key(aAlias);
   ToLowerCase(key);

   return nsUConvPropertySearch::SearchPropertyValue(kAliases,
      ArrayLength(kAliases), key, oResult);
}



nsresult
nsCharsetAlias::GetPreferred(const nsACString& aAlias,
                             nsACString& oResult)
{
   if (aAlias.IsEmpty()) return NS_ERROR_NULL_POINTER;

   nsresult res = GetPreferredInternal(aAlias, oResult);
   if (NS_FAILED(res))
      return res;

   if (nsCharsetConverterManager::IsInternal(oResult))
      return NS_ERROR_UCONV_NOCONV;

   return res;
}



nsresult
nsCharsetAlias::Equals(const nsACString& aCharset1,
                       const nsACString& aCharset2, bool* oResult)
{
   nsresult res = NS_OK;

   if(aCharset1.Equals(aCharset2, nsCaseInsensitiveCStringComparator())) {
      *oResult = true;
      return res;
   }

   if(aCharset1.IsEmpty() || aCharset2.IsEmpty()) {
      *oResult = false;
      return res;
   }

   *oResult = false;
   nsCAutoString name1;
   res = GetPreferredInternal(aCharset1, name1);
   if (NS_FAILED(res))
     return res;

   nsCAutoString name2;
   res = GetPreferredInternal(aCharset2, name2);
   if (NS_FAILED(res))
     return res;

   *oResult = name1.Equals(name2);
   return NS_OK;
}

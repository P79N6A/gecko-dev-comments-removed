





































#include "nsICharsetAlias.h"
#include "pratom.h"


#include "nsIPlatformCharset.h"

#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsUConvPropertySearch.h"
#include "nsCharsetAlias.h"


NS_IMPL_THREADSAFE_ISUPPORTS1(nsCharsetAlias2, nsICharsetAlias)


nsCharsetAlias2::nsCharsetAlias2()
{
}

nsCharsetAlias2::~nsCharsetAlias2()
{
}


static const char* kAliases[][3] = {
#include "charsetalias.properties.h"
};


NS_IMETHODIMP nsCharsetAlias2::GetPreferred(const nsACString& aAlias,
                                            nsACString& oResult)
{
   if (aAlias.IsEmpty()) return NS_ERROR_NULL_POINTER;

   nsCAutoString key(aAlias);
   ToLowerCase(key);

   nsresult rv = nsUConvPropertySearch::SearchPropertyValue(kAliases,
      NS_ARRAY_LENGTH(kAliases), key, oResult);

  return rv;
}


NS_IMETHODIMP
nsCharsetAlias2::Equals(const nsACString& aCharset1,
                        const nsACString& aCharset2, bool* oResult)
{
   nsresult res = NS_OK;

   if(aCharset1.Equals(aCharset2, nsCaseInsensitiveCStringComparator())) {
      *oResult = PR_TRUE;
      return res;
   }

   if(aCharset1.IsEmpty() || aCharset2.IsEmpty()) {
      *oResult = PR_FALSE;
      return res;
   }

   *oResult = PR_FALSE;
   nsCAutoString name1;
   nsCAutoString name2;
   res = this->GetPreferred(aCharset1, name1);
   if(NS_SUCCEEDED(res)) {
      res = this->GetPreferred(aCharset2, name2);
      if(NS_SUCCEEDED(res)) {
        *oResult = name1.Equals(name2);
      }
   }
   
   return res;
}


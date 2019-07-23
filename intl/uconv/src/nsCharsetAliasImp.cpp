





































#include "nsICharsetAlias.h"
#include "pratom.h"


#include "nsIPlatformCharset.h"

#include "nsUConvDll.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsGREResProperties.h"
#include "nsITimelineService.h"
#include "nsCharsetAlias.h"


NS_IMPL_ISUPPORTS1(nsCharsetAlias2, nsICharsetAlias)


nsCharsetAlias2::nsCharsetAlias2()
{
  mDelegate = nsnull; 
}

nsCharsetAlias2::~nsCharsetAlias2()
{
  if(mDelegate)
     delete mDelegate;
}

NS_IMETHODIMP nsCharsetAlias2::GetPreferred(const nsACString& aAlias,
                                            nsACString& oResult)
{
   if (aAlias.IsEmpty()) return NS_ERROR_NULL_POINTER;
   NS_TIMELINE_START_TIMER("nsCharsetAlias2:GetPreferred");

   nsCAutoString aKey(aAlias);
   ToLowerCase(aKey);
   oResult.Truncate();

   
   
   
   
   
   if(aKey.EqualsLiteral("utf-8")) {
     oResult.AssignLiteral("UTF-8");
     NS_TIMELINE_STOP_TIMER("nsCharsetAlias2:GetPreferred");
     return NS_OK;
   } 
   if(aKey.EqualsLiteral("iso-8859-1")) {
     oResult.AssignLiteral("ISO-8859-1");
     NS_TIMELINE_STOP_TIMER("nsCharsetAlias2:GetPreferred");
     return NS_OK;
   } 
   if(aKey.EqualsLiteral("x-sjis") ||
      aKey.EqualsLiteral("shift_jis")) {
     oResult.AssignLiteral("Shift_JIS");
     NS_TIMELINE_STOP_TIMER("nsCharsetAlias2:GetPreferred");
     return NS_OK;
   } 

   if(!mDelegate) {
     
     
     
     mDelegate = new nsGREResProperties( NS_LITERAL_CSTRING("charsetalias.properties") );
     NS_ASSERTION(mDelegate, "cannot create nsGREResProperties");
     if(nsnull == mDelegate)
       return NS_ERROR_OUT_OF_MEMORY;
   }

   NS_TIMELINE_STOP_TIMER("nsCharsetAlias2:GetPreferred");
   NS_TIMELINE_MARK_TIMER("nsCharsetAlias2:GetPreferred");

   
   
   nsAutoString result;
   nsresult rv = mDelegate->Get(NS_ConvertASCIItoUTF16(aKey), result);
   LossyAppendUTF16toASCII(result, oResult);
   return rv;
}


NS_IMETHODIMP
nsCharsetAlias2::Equals(const nsACString& aCharset1,
                        const nsACString& aCharset2, PRBool* oResult)
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
        *oResult = name1.Equals(name2, nsCaseInsensitiveCStringComparator());
      }
   }
   
   return res;
}


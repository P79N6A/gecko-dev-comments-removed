




































#include "nsWrapUtils.h"

#include "nsCOMPtr.h"


#include "nsIServiceManager.h"
#include "nsILineBreaker.h"
#include "nsLWBrkCIID.h"




nsresult
nsWrapUtils::Rewrap(const nsAString& aInString,
                         PRUint32 aWrapCol, PRUint32 aFirstLineOffset,
                         PRBool aRespectNewlines,
                         const nsAString &aLineStartStr,
                         nsAString& aOutString)
{
  PRInt32 i;

  nsresult rv;
  nsCOMPtr<nsILineBreaker> lineBreaker = do_GetService(NS_LBRK_CONTRACTID, &rv);

  aOutString.Truncate();

  
  const nsPromiseFlatString &tString = PromiseFlatString(aInString);
  PRInt32 length = tString.Length();
  const PRUnichar* unicodeStr = tString.get();
  for (i = 0; i < length; )    
  {
    nsAutoString remaining(unicodeStr + i, length - i);

    
    
    if (!aFirstLineOffset)
      aOutString.Append(aLineStartStr);

    PRInt32 eol = i + aWrapCol - aFirstLineOffset;
    if (eol > length)
    {
      aOutString.Append(unicodeStr + i, length - i);
      aOutString.Append(PRUnichar('\n'));  
      break;
    }
    if (i > 0) aFirstLineOffset = 0;
    
    
    PRInt32 breakPt;
    rv = NS_ERROR_BASE;
    if (lineBreaker)
    {
      breakPt = lineBreaker->Prev(unicodeStr + i, length - i, eol - i);
      if (breakPt == NS_LINEBREAKER_NEED_MORE_TEXT)
      {
        breakPt = lineBreaker->Next(unicodeStr + i, length - i, eol - i);
        if (breakPt == NS_LINEBREAKER_NEED_MORE_TEXT) rv = NS_ERROR_BASE;
        else rv = NS_OK;
      }
      else rv = NS_OK;
    }
    
    
    
    if (NS_FAILED(rv))
    {
#ifdef DEBUG_akkana
      printf("nsILineBreaker not working -- breaking hard\n");
#endif
      breakPt = eol+1;
    }
    else breakPt += i;
    nsAutoString appending(unicodeStr + i, breakPt - i);
    aOutString.Append(unicodeStr + i, breakPt - i);
    aOutString.Append(PRUnichar('\n'));  

    i = breakPt;
  } 

  return NS_OK;
}


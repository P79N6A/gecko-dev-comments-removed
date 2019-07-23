






































#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsInternetCiter.h"
#include "nsCRT.h"

#include "nsCOMPtr.h"


#include "nsIServiceManager.h"
#include "nsLWBrkCIID.h"
#include "nsILineBreaker.h"

const PRUnichar gt ('>');
const PRUnichar space (' ');
const PRUnichar nbsp (0xa0);
const PRUnichar nl ('\n');
const PRUnichar cr('\r');





nsInternetCiter::nsInternetCiter()
{
}

nsInternetCiter::~nsInternetCiter()
{
}

NS_IMPL_ISUPPORTS1(nsInternetCiter, nsICiter)

NS_IMETHODIMP
nsInternetCiter::GetCiteString(const nsAString& aInString, nsAString& aOutString)
{
  aOutString.Truncate();
  PRUnichar uch = nl;

  
  
  nsReadingIterator <PRUnichar> beginIter,endIter;
  aInString.BeginReading(beginIter);
  aInString.EndReading(endIter);
  while(beginIter!= endIter &&
        (*endIter == cr ||
         *endIter == nl))
  {
    --endIter;
  }

  
  while (beginIter != endIter)
  {
    if (uch == nl)
    {
      aOutString.Append(gt);
      
      
      if (*beginIter != gt)
        aOutString.Append(space);
    }

    uch = *beginIter;
    ++beginIter;

    aOutString += uch;
  }

  if (uch != nl)
    aOutString += nl;

  return NS_OK;
}

nsresult
nsInternetCiter::StripCitesAndLinebreaks(const nsAString& aInString,
                                         nsAString& aOutString,
                                         PRBool aLinebreaksToo,
                                         PRInt32* aCiteLevel)
{
  if (aCiteLevel)
    *aCiteLevel = 0;

  aOutString.Truncate();
  nsReadingIterator <PRUnichar> beginIter,endIter;
  aInString.BeginReading(beginIter);
  aInString.EndReading(endIter);
  while (beginIter!= endIter)  
  {
    
    PRInt32 thisLineCiteLevel = 0;
    while (beginIter!= endIter && (*beginIter == gt || nsCRT::IsAsciiSpace(*beginIter)))
    {
      if (*beginIter == gt) ++thisLineCiteLevel;
      ++beginIter;
    }

    
    while (beginIter != endIter && (*beginIter != '\r' && *beginIter != '\n'))
    {
      aOutString.Append(*beginIter);
      ++beginIter;
    }
    if (aLinebreaksToo)
      aOutString.Append(PRUnichar(' '));
    else
      aOutString.Append(PRUnichar('\n'));    
      
    while (beginIter != endIter && (*beginIter == '\r' || *beginIter == '\n'))
      ++beginIter;

    
    if (aCiteLevel && (thisLineCiteLevel > *aCiteLevel))
      *aCiteLevel = thisLineCiteLevel;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsInternetCiter::StripCites(const nsAString& aInString, nsAString& aOutString)
{
  return StripCitesAndLinebreaks(aInString, aOutString, PR_FALSE, 0);
}

static void AddCite(nsAString& aOutString, PRInt32 citeLevel)
{
  for (PRInt32 i = 0; i < citeLevel; ++i)
    aOutString.Append(gt);
  if (citeLevel > 0)
    aOutString.Append(space);
}

static inline void
BreakLine(nsAString& aOutString, PRUint32& outStringCol,
          PRUint32 citeLevel)
{
  aOutString.Append(nl);
  if (citeLevel > 0)
  {
    AddCite(aOutString, citeLevel);
    outStringCol = citeLevel + 1;
  }
  else
    outStringCol = 0;
}

static inline PRBool IsSpace(PRUnichar c)
{
  return (nsCRT::IsAsciiSpace(c) || (c == nl) || (c == cr) || (c == nbsp));
}

NS_IMETHODIMP
nsInternetCiter::Rewrap(const nsAString& aInString,
                        PRUint32 aWrapCol, PRUint32 aFirstLineOffset,
                        PRBool aRespectNewlines,
                        nsAString& aOutString)
{
  
  
#ifdef DEBUG
  PRInt32 cr = aInString.FindChar(PRUnichar('\r'));
  NS_ASSERTION((cr < 0), "Rewrap: CR in string gotten from DOM!\n");
#endif 

  aOutString.Truncate();

  nsresult rv;
  nsCOMPtr<nsILineBreaker> lineBreaker = do_GetService(NS_LBRK_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRUint32 length;
  PRUint32 posInString = 0;
  PRUint32 outStringCol = 0;
  PRUint32 citeLevel = 0;
  const nsPromiseFlatString &tString = PromiseFlatString(aInString);
  length = tString.Length();
#ifdef DEBUG_wrapping
  int loopcount = 0;
#endif
  while (posInString < length)
  {
#ifdef DEBUG_wrapping
    printf("Outer loop: '%s'\n",
           NS_LossyConvertUTF16toASCII(Substring(tString, posInString,
                                                length-posInString)).get());
    printf("out string is now: '%s'\n",
           NS_LossyConvertUTF16toASCII(aOutString).get());

#endif

    
    PRUint32 newCiteLevel = 0;
    while (posInString < length && tString[posInString] == gt)
    {
      ++newCiteLevel;
      ++posInString;
      while (posInString < length && tString[posInString] == space)
        ++posInString;
    }
    if (posInString >= length)
      break;

    
    
    if (tString[posInString] == nl && !aOutString.IsEmpty())
    {
      if (aOutString.Last() != nl)
        aOutString.Append(nl);
      AddCite(aOutString, newCiteLevel);
      aOutString.Append(nl);

      ++posInString;
      outStringCol = 0;
      continue;
    }

    
    
    
    if (newCiteLevel != citeLevel && posInString > newCiteLevel+1
        && outStringCol != 0)
    {
      BreakLine(aOutString, outStringCol, 0);
    }
    citeLevel = newCiteLevel;

    
    if (outStringCol == 0)
    {
      AddCite(aOutString, citeLevel);
      outStringCol = citeLevel + (citeLevel ? 1 : 0);
    }
    
    
    
    else if (outStringCol > citeLevel)
    {
      aOutString.Append(space);
      ++outStringCol;
    }

    
    PRInt32 nextNewline = tString.FindChar(nl, posInString);
    if (nextNewline < 0) nextNewline = length;

    
    
    
    
    
    
    
    if (citeLevel == 0)
    {
      aOutString.Append(Substring(tString, posInString,
                                  nextNewline-posInString));
      outStringCol += nextNewline - posInString;
      if (nextNewline != (PRInt32)length)
      {
        aOutString.Append(nl);
        outStringCol = 0;
      }
      posInString = nextNewline+1;
      continue;
    }

    
    
    while ((PRInt32)posInString < nextNewline)
    {
#ifdef DEBUG_wrapping
      if (++loopcount > 1000)
        NS_ASSERTION(PR_FALSE, "possible infinite loop in nsInternetCiter\n");

      printf("Inner loop: '%s'\n",
             NS_LossyConvertUTF16toASCII(Substring(tString, posInString,
                                              nextNewline-posInString)).get());
#endif

      
      while ((PRInt32)posInString < nextNewline
             && nsCRT::IsAsciiSpace(tString[posInString]))
        ++posInString;

      
      if (outStringCol + nextNewline - posInString <= aWrapCol-citeLevel-1)
      {
        
        
        if (nextNewline+1 == (PRInt32)length && tString[nextNewline-1] == nl)
          ++nextNewline;

        
        PRInt32 lastRealChar = nextNewline;
        while ((PRUint32)lastRealChar > posInString
               && nsCRT::IsAsciiSpace(tString[lastRealChar-1]))
          --lastRealChar;

        aOutString += Substring(tString,
                                posInString, lastRealChar - posInString);
        outStringCol += lastRealChar - posInString;
        posInString = nextNewline + 1;
        continue;
      }

      PRInt32 eol = posInString + aWrapCol - citeLevel - outStringCol;
      
      
      
      
      if (eol <= (PRInt32)posInString)
      {
        BreakLine(aOutString, outStringCol, citeLevel);
        continue;    
      }

      PRInt32 breakPt;
      rv = NS_ERROR_BASE;
      if (lineBreaker)
      {
        breakPt = lineBreaker->Prev(tString.get() + posInString,
                                 length - posInString, eol + 1 - posInString);
        if (breakPt == NS_LINEBREAKER_NEED_MORE_TEXT)
        {
          
          
          
          if (outStringCol > citeLevel + 1)
          {
            BreakLine(aOutString, outStringCol, citeLevel);
            continue;    
          }

          
          breakPt = lineBreaker->Next(tString.get() + posInString,
                                      length - posInString, eol - posInString);
          if (breakPt == NS_LINEBREAKER_NEED_MORE_TEXT) rv = NS_ERROR_BASE;
          else rv = NS_OK;
        }
        else rv = NS_OK;
      }
      
      
      
      if (NS_FAILED(rv))
      {
#ifdef DEBUG_akkana
        printf("nsInternetCiter: LineBreaker not working -- breaking hard\n");
#endif
        breakPt = eol;
      }

      
      
      
      
      
      
      
      const int SLOP = 6;
      if (outStringCol + breakPt > aWrapCol + SLOP
          && outStringCol > citeLevel+1)
      {
        BreakLine(aOutString, outStringCol, citeLevel);
        continue;
      }

      nsAutoString sub (Substring(tString, posInString, breakPt));
      
      PRInt32 subend = sub.Length();
      while (subend > 0 && IsSpace(sub[subend-1]))
        --subend;
      sub.Left(sub, subend);
      aOutString += sub;
      outStringCol += sub.Length();
      
      posInString += breakPt;
      while (posInString < length && IsSpace(tString[posInString]))
        ++posInString;

      
      if (posInString < length)    
        BreakLine(aOutString, outStringCol, citeLevel);

    } 
#ifdef DEBUG_wrapping
    printf("---------\nEnd inner loop: out string is now '%s'\n-----------\n",
           NS_LossyConvertUTF16toASCII(aOutString).get());
#endif
  } 

#ifdef DEBUG_wrapping
  printf("Final out string is now: '%s'\n",
         NS_LossyConvertUTF16toASCII(aOutString).get());

#endif
  return NS_OK;
}



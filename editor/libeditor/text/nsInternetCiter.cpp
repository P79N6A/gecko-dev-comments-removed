





#include "nsAString.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsDebug.h"
#include "nsDependentSubstring.h"
#include "nsError.h"
#include "nsILineBreaker.h"
#include "nsInternetCiter.h"
#include "nsLWBrkCIID.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "nsStringIterator.h"

const char16_t gt ('>');
const char16_t space (' ');
const char16_t nbsp (0xa0);
const char16_t nl ('\n');
const char16_t cr('\r');




nsresult
nsInternetCiter::GetCiteString(const nsAString& aInString, nsAString& aOutString)
{
  aOutString.Truncate();
  char16_t uch = nl;

  
  
  nsReadingIterator <char16_t> beginIter,endIter;
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
                                         bool aLinebreaksToo,
                                         int32_t* aCiteLevel)
{
  if (aCiteLevel)
    *aCiteLevel = 0;

  aOutString.Truncate();
  nsReadingIterator <char16_t> beginIter,endIter;
  aInString.BeginReading(beginIter);
  aInString.EndReading(endIter);
  while (beginIter!= endIter)  
  {
    
    int32_t thisLineCiteLevel = 0;
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
      aOutString.Append(char16_t(' '));
    else
      aOutString.Append(char16_t('\n'));    
      
    while (beginIter != endIter && (*beginIter == '\r' || *beginIter == '\n'))
      ++beginIter;

    
    if (aCiteLevel && (thisLineCiteLevel > *aCiteLevel))
      *aCiteLevel = thisLineCiteLevel;
  }
  return NS_OK;
}

nsresult
nsInternetCiter::StripCites(const nsAString& aInString, nsAString& aOutString)
{
  return StripCitesAndLinebreaks(aInString, aOutString, false, 0);
}

static void AddCite(nsAString& aOutString, int32_t citeLevel)
{
  for (int32_t i = 0; i < citeLevel; ++i)
    aOutString.Append(gt);
  if (citeLevel > 0)
    aOutString.Append(space);
}

static inline void
BreakLine(nsAString& aOutString, uint32_t& outStringCol,
          uint32_t citeLevel)
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

static inline bool IsSpace(char16_t c)
{
  return (nsCRT::IsAsciiSpace(c) || (c == nl) || (c == cr) || (c == nbsp));
}

nsresult
nsInternetCiter::Rewrap(const nsAString& aInString,
                        uint32_t aWrapCol, uint32_t aFirstLineOffset,
                        bool aRespectNewlines,
                        nsAString& aOutString)
{
  
  
#ifdef DEBUG
  int32_t cr = aInString.FindChar(char16_t('\r'));
  NS_ASSERTION((cr < 0), "Rewrap: CR in string gotten from DOM!\n");
#endif 

  aOutString.Truncate();

  nsresult rv;
  nsCOMPtr<nsILineBreaker> lineBreaker = do_GetService(NS_LBRK_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  uint32_t length;
  uint32_t posInString = 0;
  uint32_t outStringCol = 0;
  uint32_t citeLevel = 0;
  const nsPromiseFlatString &tString = PromiseFlatString(aInString);
  length = tString.Length();
  while (posInString < length)
  {
    
    uint32_t newCiteLevel = 0;
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

    
    int32_t nextNewline = tString.FindChar(nl, posInString);
    if (nextNewline < 0) nextNewline = length;

    
    
    
    
    
    
    
    if (citeLevel == 0)
    {
      aOutString.Append(Substring(tString, posInString,
                                  nextNewline-posInString));
      outStringCol += nextNewline - posInString;
      if (nextNewline != (int32_t)length)
      {
        aOutString.Append(nl);
        outStringCol = 0;
      }
      posInString = nextNewline+1;
      continue;
    }

    
    
    while ((int32_t)posInString < nextNewline)
    {
      
      while ((int32_t)posInString < nextNewline
             && nsCRT::IsAsciiSpace(tString[posInString]))
        ++posInString;

      
      if (outStringCol + nextNewline - posInString <= aWrapCol-citeLevel-1)
      {
        
        
        if (nextNewline+1 == (int32_t)length && tString[nextNewline-1] == nl)
          ++nextNewline;

        
        int32_t lastRealChar = nextNewline;
        while ((uint32_t)lastRealChar > posInString
               && nsCRT::IsAsciiSpace(tString[lastRealChar-1]))
          --lastRealChar;

        aOutString += Substring(tString,
                                posInString, lastRealChar - posInString);
        outStringCol += lastRealChar - posInString;
        posInString = nextNewline + 1;
        continue;
      }

      int32_t eol = posInString + aWrapCol - citeLevel - outStringCol;
      
      
      
      
      if (eol <= (int32_t)posInString)
      {
        BreakLine(aOutString, outStringCol, citeLevel);
        continue;    
      }

      int32_t breakPt = 0;
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
      
      int32_t subend = sub.Length();
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
  } 

  return NS_OK;
}



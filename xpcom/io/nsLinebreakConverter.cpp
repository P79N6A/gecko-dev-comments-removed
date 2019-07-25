




#include "nsLinebreakConverter.h"

#include "nsMemory.h"
#include "nsCRT.h"







static const char* GetLinebreakString(nsLinebreakConverter::ELinebreakType aBreakType)
{
  static const char* const sLinebreaks[] = {
    "",             
    NS_LINEBREAK,   
    LFSTR,          
    CRLF,           
    CRSTR,          
    LFSTR,          
    CRLF,           
    " ",            
    nullptr  
  };
  
  return sLinebreaks[aBreakType];
}







template<class T>
void AppendLinebreak(T*& ioDest, const char* lineBreakStr)
{
  *ioDest++ = *lineBreakStr;

  if (lineBreakStr[1])
    *ioDest++ = lineBreakStr[1];
}






template<class T>
int32_t CountLinebreaks(const T* aSrc, int32_t inLen, const char* breakStr)
{
  const T* src = aSrc;
  const T* srcEnd = aSrc + inLen;
  int32_t theCount = 0;

  while (src < srcEnd)
  {
    if (*src == *breakStr)
    {
      src++;

      if (breakStr[1])
      {
        if (src < srcEnd && *src == breakStr[1])
        {
          src++;
          theCount++;
        }
      }
      else
      {
        theCount++;
      }
    }
    else
    {
      src++;
    }
  }
  
  return theCount;
}







template<class T>
static T* ConvertBreaks(const T* inSrc, int32_t& ioLen, const char* srcBreak, const char* destBreak)
{
  NS_ASSERTION(inSrc && srcBreak && destBreak, "Got a null string");
  
  T* resultString = nullptr;
   
  
  if (nsCRT::strcmp(srcBreak, destBreak) == 0)
  {
    resultString = (T *)nsMemory::Alloc(sizeof(T) * ioLen);
    if (!resultString) return nullptr;
    memcpy(resultString, inSrc, sizeof(T) * ioLen); 
    return resultString;
  }
    
  int32_t srcBreakLen = strlen(srcBreak);
  int32_t destBreakLen = strlen(destBreak);

  
  
  if (srcBreakLen == destBreakLen && srcBreakLen == 1)
  {
    resultString = (T *)nsMemory::Alloc(sizeof(T) * ioLen);
    if (!resultString) return nullptr;
    
    const T* src = inSrc;
    const T* srcEnd = inSrc + ioLen;		
    T*       dst = resultString;
    
    char srcBreakChar = *srcBreak;        
    char dstBreakChar = *destBreak;
    
    while (src < srcEnd)
    {
      if (*src == srcBreakChar)
      {
        *dst++ = dstBreakChar;
        src++;
      }
      else
      {
        *dst++ = *src++;
      }
    }

    
  }
  else
  {
    
    
    
    int32_t numLinebreaks = CountLinebreaks(inSrc, ioLen, srcBreak);
    
    int32_t newBufLen = ioLen - (numLinebreaks * srcBreakLen) + (numLinebreaks * destBreakLen);
    resultString = (T *)nsMemory::Alloc(sizeof(T) * newBufLen);
    if (!resultString) return nullptr;
    
    const T* src = inSrc;
    const T* srcEnd = inSrc + ioLen;		
    T*       dst = resultString;
    
    while (src < srcEnd)
    {
      if (*src == *srcBreak)
      {
        *dst++ = *destBreak;
        if (destBreak[1])
          *dst++ = destBreak[1];
      
        src++;
        if (src < srcEnd && srcBreak[1] && *src == srcBreak[1])
          src++;
      }
      else
      {
        *dst++ = *src++;
      }
    }
    
    ioLen = newBufLen;
  }
  
  return resultString;
}








template<class T>
static void ConvertBreaksInSitu(T* inSrc, int32_t inLen, char srcBreak, char destBreak)
{
  T* src = inSrc;
  T* srcEnd = inSrc + inLen;

  while (src < srcEnd)
  {
    if (*src == srcBreak)
      *src = destBreak;
    
    src++;
  }
}









template<class T>
static T* ConvertUnknownBreaks(const T* inSrc, int32_t& ioLen, const char* destBreak)
{
  const T* src = inSrc;
  const T* srcEnd = inSrc + ioLen;		
  
  int32_t destBreakLen = strlen(destBreak);
  int32_t finalLen = 0;

  while (src < srcEnd)
  {
    if (*src == nsCRT::CR)
    {
      if (src < srcEnd && src[1] == nsCRT::LF)
      {
        
        finalLen += destBreakLen;
        src++;
      }
      else
      {
        
        finalLen += destBreakLen;
      }
    }
    else if (*src == nsCRT::LF)
    {
      
      finalLen += destBreakLen;
    }
    else
    {
      finalLen++;
    }
    src++;
  }
  
  T* resultString = (T *)nsMemory::Alloc(sizeof(T) * finalLen);
  if (!resultString) return nullptr;

  src = inSrc;
  srcEnd = inSrc + ioLen;		

  T* dst = resultString;
  
  while (src < srcEnd)
  {
    if (*src == nsCRT::CR)
    {
      if (src < srcEnd && src[1] == nsCRT::LF)
      {
        
        AppendLinebreak(dst, destBreak);
        src++;
      }
      else
      {
        
        AppendLinebreak(dst, destBreak);
      }
    }
    else if (*src == nsCRT::LF)
    {
      
      AppendLinebreak(dst, destBreak);
    }
    else
    {
      *dst++ = *src;
    }
    src++;
  }

  ioLen = finalLen;
  return resultString;
}






char* nsLinebreakConverter::ConvertLineBreaks(const char* aSrc,
            ELinebreakType aSrcBreaks, ELinebreakType aDestBreaks, int32_t aSrcLen, int32_t* outLen)
{
  NS_ASSERTION(aDestBreaks != eLinebreakAny &&
               aSrcBreaks != eLinebreakSpace, "Invalid parameter");
  if (!aSrc) return nullptr;
  
  int32_t sourceLen = (aSrcLen == kIgnoreLen) ? strlen(aSrc) + 1 : aSrcLen;

  char* resultString;
  if (aSrcBreaks == eLinebreakAny)
    resultString = ConvertUnknownBreaks(aSrc, sourceLen, GetLinebreakString(aDestBreaks));
  else
    resultString = ConvertBreaks(aSrc, sourceLen, GetLinebreakString(aSrcBreaks), GetLinebreakString(aDestBreaks));
  
  if (outLen)
    *outLen = sourceLen;
  return resultString;
}






nsresult nsLinebreakConverter::ConvertLineBreaksInSitu(char **ioBuffer, ELinebreakType aSrcBreaks,
            ELinebreakType aDestBreaks, int32_t aSrcLen, int32_t* outLen)
{
  NS_ASSERTION(ioBuffer && *ioBuffer, "Null pointer passed");
  if (!ioBuffer || !*ioBuffer) return NS_ERROR_NULL_POINTER;
  
  NS_ASSERTION(aDestBreaks != eLinebreakAny &&
               aSrcBreaks != eLinebreakSpace, "Invalid parameter");

  int32_t sourceLen = (aSrcLen == kIgnoreLen) ? strlen(*ioBuffer) + 1 : aSrcLen;
  
  
  const char* srcBreaks = GetLinebreakString(aSrcBreaks);
  const char* dstBreaks = GetLinebreakString(aDestBreaks);
  
  if ( (aSrcBreaks != eLinebreakAny) &&
       (strlen(srcBreaks) == 1) &&
       (strlen(dstBreaks) == 1) )
  {
    ConvertBreaksInSitu(*ioBuffer, sourceLen, *srcBreaks, *dstBreaks);
    if (outLen)
      *outLen = sourceLen;
  }
  else
  {
    char* destBuffer;
    
    if (aSrcBreaks == eLinebreakAny)
      destBuffer = ConvertUnknownBreaks(*ioBuffer, sourceLen, dstBreaks);
    else
      destBuffer = ConvertBreaks(*ioBuffer, sourceLen, srcBreaks, dstBreaks);

    if (!destBuffer) return NS_ERROR_OUT_OF_MEMORY;
    *ioBuffer = destBuffer;
    if (outLen)
      *outLen = sourceLen;
  }
  
  return NS_OK;
}






PRUnichar* nsLinebreakConverter::ConvertUnicharLineBreaks(const PRUnichar* aSrc,
            ELinebreakType aSrcBreaks, ELinebreakType aDestBreaks, int32_t aSrcLen, int32_t* outLen)
{
  NS_ASSERTION(aDestBreaks != eLinebreakAny &&
               aSrcBreaks != eLinebreakSpace, "Invalid parameter");
  if (!aSrc) return nullptr;
  
  int32_t bufLen = (aSrcLen == kIgnoreLen) ? NS_strlen(aSrc) + 1 : aSrcLen;

  PRUnichar* resultString;
  if (aSrcBreaks == eLinebreakAny)
    resultString = ConvertUnknownBreaks(aSrc, bufLen, GetLinebreakString(aDestBreaks));
  else
    resultString = ConvertBreaks(aSrc, bufLen, GetLinebreakString(aSrcBreaks), GetLinebreakString(aDestBreaks));
  
  if (outLen)
    *outLen = bufLen;
  return resultString;
}






nsresult nsLinebreakConverter::ConvertUnicharLineBreaksInSitu(PRUnichar **ioBuffer,
            ELinebreakType aSrcBreaks, ELinebreakType aDestBreaks, int32_t aSrcLen, int32_t* outLen)
{
  NS_ASSERTION(ioBuffer && *ioBuffer, "Null pointer passed");
  if (!ioBuffer || !*ioBuffer) return NS_ERROR_NULL_POINTER;
  NS_ASSERTION(aDestBreaks != eLinebreakAny &&
               aSrcBreaks != eLinebreakSpace, "Invalid parameter");

  int32_t sourceLen = (aSrcLen == kIgnoreLen) ? NS_strlen(*ioBuffer) + 1 : aSrcLen;

  
  const char* srcBreaks = GetLinebreakString(aSrcBreaks);
  const char* dstBreaks = GetLinebreakString(aDestBreaks);
  
  if ( (aSrcBreaks != eLinebreakAny) &&
       (strlen(srcBreaks) == 1) &&
       (strlen(dstBreaks) == 1) )
  {
    ConvertBreaksInSitu(*ioBuffer, sourceLen, *srcBreaks, *dstBreaks);
    if (outLen)
      *outLen = sourceLen;
  }
  else
  {
    PRUnichar* destBuffer;
    
    if (aSrcBreaks == eLinebreakAny)
      destBuffer = ConvertUnknownBreaks(*ioBuffer, sourceLen, dstBreaks);
    else
      destBuffer = ConvertBreaks(*ioBuffer, sourceLen, srcBreaks, dstBreaks);

    if (!destBuffer) return NS_ERROR_OUT_OF_MEMORY;
    *ioBuffer = destBuffer;
    if (outLen)
      *outLen = sourceLen;
  }
  
  return NS_OK;
}





nsresult nsLinebreakConverter::ConvertStringLineBreaks(nsString& ioString,
          ELinebreakType aSrcBreaks, ELinebreakType aDestBreaks)
{

  NS_ASSERTION(aDestBreaks != eLinebreakAny &&
               aSrcBreaks != eLinebreakSpace, "Invalid parameter");

  
  if (ioString.IsEmpty()) return NS_OK;
  
  nsresult rv;
  
  
  
  nsString::char_iterator stringBuf;
  ioString.BeginWriting(stringBuf);
  
  int32_t    newLen;
    
  rv = ConvertUnicharLineBreaksInSitu(&stringBuf,
                                      aSrcBreaks, aDestBreaks,
                                      ioString.Length() + 1, &newLen);
  if (NS_FAILED(rv)) return rv;

  if (stringBuf != ioString.get())
    ioString.Adopt(stringBuf);
  
  return NS_OK;
}




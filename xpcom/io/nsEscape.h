







#ifndef _ESCAPE_H_
#define _ESCAPE_H_

#include "nscore.h"
#include "nsError.h"
#include "nsString.h"






typedef enum {
  url_All       = 0,       
  url_XAlphas   = 1u << 0, 
  url_XPAlphas  = 1u << 1, 
  url_Path      = 1u << 2  
} nsEscapeMask;

#ifdef __cplusplus
extern "C" {
#endif








char* nsEscape(const char* aStr, nsEscapeMask aMask);

char* nsUnescape(char* aStr);




int32_t nsUnescapeCount(char* aStr);





char*
nsEscapeHTML(const char* aString);

char16_t*
nsEscapeHTML2(const char16_t* aSourceBuffer,
              int32_t aSourceBufferLen = -1);





#ifdef __cplusplus
}
#endif








enum EscapeMask {
  
  esc_Scheme         = 1u << 0,
  esc_Username       = 1u << 1,
  esc_Password       = 1u << 2,
  esc_Host           = 1u << 3,
  esc_Directory      = 1u << 4,
  esc_FileBaseName   = 1u << 5,
  esc_FileExtension  = 1u << 6,
  esc_FilePath       = esc_Directory | esc_FileBaseName | esc_FileExtension,
  esc_Param          = 1u << 7,
  esc_Query          = 1u << 8,
  esc_Ref            = 1u << 9,
  
  esc_Minimal        = esc_Scheme | esc_Username | esc_Password | esc_Host | esc_FilePath | esc_Param | esc_Query | esc_Ref,
  esc_Forced         = 1u << 10, 
  esc_OnlyASCII      = 1u << 11, 
  esc_OnlyNonASCII   = 1u << 12, 


  esc_AlwaysCopy     = 1u << 13, 
  esc_Colon          = 1u << 14, 
  esc_SkipControl    = 1u << 15  
};















bool NS_EscapeURL(const char* aStr,
                  int32_t aLen,
                  uint32_t aFlags,
                  nsACString& aResult);












bool NS_UnescapeURL(const char* aStr,
                    int32_t aLen,
                    uint32_t aFlags,
                    nsACString& aResult);


inline int32_t
NS_UnescapeURL(char* aStr)
{
  return nsUnescapeCount(aStr);
}




inline const nsCSubstring&
NS_EscapeURL(const nsCSubstring& aStr, uint32_t aFlags, nsCSubstring& aResult)
{
  if (NS_EscapeURL(aStr.Data(), aStr.Length(), aFlags, aResult)) {
    return aResult;
  }
  return aStr;
}
inline const nsCSubstring&
NS_UnescapeURL(const nsCSubstring& aStr, uint32_t aFlags, nsCSubstring& aResult)
{
  if (NS_UnescapeURL(aStr.Data(), aStr.Length(), aFlags, aResult)) {
    return aResult;
  }
  return aStr;
}





inline bool
NS_Escape(const nsCString& aOriginal, nsCString& aEscaped,
          nsEscapeMask aMask)
{
  char* esc = nsEscape(aOriginal.get(), aMask);
  if (! esc) {
    return false;
  }
  aEscaped.Adopt(esc);
  return true;
}




inline nsCString&
NS_UnescapeURL(nsCString& aStr)
{
  aStr.SetLength(nsUnescapeCount(aStr.BeginWriting()));
  return aStr;
}

#endif

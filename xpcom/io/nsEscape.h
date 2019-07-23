






































#ifndef _ESCAPE_H_
#define _ESCAPE_H_

#include "prtypes.h"
#include "nscore.h"
#include "nsError.h"
#include "nsString.h"






typedef enum {
 	url_All       = 0         
,	url_XAlphas   = PR_BIT(0) 
,	url_XPAlphas  = PR_BIT(1) 
,	url_Path      = PR_BIT(2) 
} nsEscapeMask;

#ifdef __cplusplus
extern "C" {
#endif








NS_COM char * nsEscape(const char * str, nsEscapeMask mask);

NS_COM char * nsUnescape(char * str);
	



NS_COM PRInt32 nsUnescapeCount (char * str);
	




NS_COM char *
nsEscapeHTML(const char * string);

NS_COM PRUnichar *
nsEscapeHTML2(const PRUnichar *aSourceBuffer,
              PRInt32 aSourceBufferLen = -1);
 




#ifdef __cplusplus
}
#endif








enum EscapeMask {
  
  esc_Scheme         = PR_BIT(0),
  esc_Username       = PR_BIT(1),
  esc_Password       = PR_BIT(2),
  esc_Host           = PR_BIT(3),
  esc_Directory      = PR_BIT(4),
  esc_FileBaseName   = PR_BIT(5),
  esc_FileExtension  = PR_BIT(6),
  esc_FilePath       = esc_Directory | esc_FileBaseName | esc_FileExtension,
  esc_Param          = PR_BIT(7),
  esc_Query          = PR_BIT(8),
  esc_Ref            = PR_BIT(9),
  
  esc_Minimal        = esc_Scheme | esc_Username | esc_Password | esc_Host | esc_FilePath | esc_Param | esc_Query | esc_Ref, 
  esc_Forced         = PR_BIT(10), 
  esc_OnlyASCII      = PR_BIT(11), 
  esc_OnlyNonASCII   = PR_BIT(12), 


  esc_AlwaysCopy     = PR_BIT(13), 
  esc_Colon          = PR_BIT(14), 
  esc_SkipControl    = PR_BIT(15)  
};















NS_COM PRBool NS_EscapeURL(const char *str,
                           PRInt32 len,
                           PRUint32 flags,
                           nsACString &result);












NS_COM PRBool NS_UnescapeURL(const char *str,
                             PRInt32 len,
                             PRUint32 flags,
                             nsACString &result);


inline PRInt32 NS_UnescapeURL(char *str) {
    return nsUnescapeCount(str);
}




inline const nsCSubstring &
NS_EscapeURL(const nsCSubstring &str, PRUint32 flags, nsCSubstring &result) {
    if (NS_EscapeURL(str.Data(), str.Length(), flags, result))
        return result;
    return str;
}
inline const nsCSubstring &
NS_UnescapeURL(const nsCSubstring &str, PRUint32 flags, nsCSubstring &result) {
    if (NS_UnescapeURL(str.Data(), str.Length(), flags, result))
        return result;
    return str;
}


#ifdef MOZ_V1_STRING_ABI
inline const nsACString &
NS_EscapeURL(const nsACString &str, PRUint32 flags, nsACString &result) {
    
    
    nsACString::const_iterator iter;
    str.BeginReading(iter);
    if (NS_EscapeURL(iter.get(), iter.size_forward(), flags, result))
        return result;
    return str;
}
inline const nsACString &
NS_EscapeURL(const nsCSubstring &str, PRUint32 flags, nsACString &result) {
    if (NS_EscapeURL(str.Data(), str.Length(), flags, result))
        return result;
    return str;
}
inline const nsACString &
NS_UnescapeURL(const nsACString &str, PRUint32 flags, nsACString &result) {
    
    
    nsACString::const_iterator iter;
    str.BeginReading(iter);
    if (NS_UnescapeURL(iter.get(), iter.size_forward(), flags, result))
        return result;
    return str;
}
inline const nsACString &
NS_UnescapeURL(const nsCSubstring &str, PRUint32 flags, nsACString &result) {
    if (NS_UnescapeURL(str.Data(), str.Length(), flags, result))
        return result;
    return str;
}
#endif  





inline PRBool
NS_Escape(const nsCString& aOriginal, nsCString& aEscaped,
          nsEscapeMask aMask)
{
  char* esc = nsEscape(aOriginal.get(), aMask);
  if (! esc)
    return PR_FALSE;
  aEscaped.Adopt(esc);
  return PR_TRUE;
}




inline nsCString &
NS_UnescapeURL(nsCString &str)
{
    str.SetLength(nsUnescapeCount(str.BeginWriting()));
    return str;
}

#endif

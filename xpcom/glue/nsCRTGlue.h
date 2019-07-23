





































#ifndef nsCRTGlue_h__
#define nsCRTGlue_h__

#include "nscore.h"









NS_COM_GLUE const char*
NS_strspnp(const char *delims, const char *str);

















NS_COM_GLUE char*
NS_strtok(const char *delims, char **str);




NS_COM_GLUE PRUint32
NS_strlen(const PRUnichar *aString);




NS_COM_GLUE int
NS_strcmp(const PRUnichar *a, const PRUnichar *b);




NS_COM_GLUE PRUnichar*
NS_strdup(const PRUnichar *aString);




NS_COM_GLUE char*
NS_strdup(const char *aString);





NS_COM_GLUE PRUnichar*
NS_strndup(const PRUnichar *aString, PRUint32 aLen);





class NS_COM_GLUE nsLowerUpperUtils {
public:
  static const unsigned char kLower2Upper[256];
  static const unsigned char kUpper2Lower[256];
};

inline char NS_ToUpper(char aChar)
{
  return (char)nsLowerUpperUtils::kLower2Upper[(unsigned char)aChar];
}

inline char NS_ToLower(char aChar)
{
  return (char)nsLowerUpperUtils::kUpper2Lower[(unsigned char)aChar];
}
  
NS_COM_GLUE PRBool NS_IsUpper(char aChar);
NS_COM_GLUE PRBool NS_IsLower(char aChar);

NS_COM_GLUE PRBool NS_IsAscii(PRUnichar aChar);
NS_COM_GLUE PRBool NS_IsAscii(const PRUnichar* aString);
NS_COM_GLUE PRBool NS_IsAsciiAlpha(PRUnichar aChar);
NS_COM_GLUE PRBool NS_IsAsciiDigit(PRUnichar aChar);
NS_COM_GLUE PRBool NS_IsAsciiWhitespace(PRUnichar aChar);
NS_COM_GLUE PRBool NS_IsAscii(const char* aString);
NS_COM_GLUE PRBool NS_IsAscii(const char* aString, PRUint32 aLength);

#endif 

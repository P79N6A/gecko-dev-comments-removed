





































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
  
NS_COM_GLUE bool NS_IsUpper(char aChar);
NS_COM_GLUE bool NS_IsLower(char aChar);

NS_COM_GLUE bool NS_IsAscii(PRUnichar aChar);
NS_COM_GLUE bool NS_IsAscii(const PRUnichar* aString);
NS_COM_GLUE bool NS_IsAsciiAlpha(PRUnichar aChar);
NS_COM_GLUE bool NS_IsAsciiDigit(PRUnichar aChar);
NS_COM_GLUE bool NS_IsAsciiWhitespace(PRUnichar aChar);
NS_COM_GLUE bool NS_IsAscii(const char* aString);
NS_COM_GLUE bool NS_IsAscii(const char* aString, PRUint32 aLength);

#define FF '\f'
#define TAB '\t'

#define CRSTR "\015"
#define LFSTR "\012"
#define CRLF "\015\012"     /* A CR LF equivalent string */

#if defined(XP_MACOSX)
  #define FILE_PATH_SEPARATOR        "/"
  #define OS_FILE_ILLEGAL_CHARACTERS ":"
#elif defined(XP_WIN) || defined(XP_OS2)
  #define FILE_PATH_SEPARATOR        "\\"
  #define OS_FILE_ILLEGAL_CHARACTERS "/:*?\"<>|"
#elif defined(XP_UNIX)
  #define FILE_PATH_SEPARATOR        "/"
  #define OS_FILE_ILLEGAL_CHARACTERS ""
#else
  #error need_to_define_your_file_path_separator_and_illegal_characters
#endif



#define CONTROL_CHARACTERS     "\001\002\003\004\005\006\007" \
                           "\010\011\012\013\014\015\016\017" \
                           "\020\021\022\023\024\025\026\027" \
                           "\030\031\032\033\034\035\036\037"

#define FILE_ILLEGAL_CHARACTERS CONTROL_CHARACTERS OS_FILE_ILLEGAL_CHARACTERS

#endif 

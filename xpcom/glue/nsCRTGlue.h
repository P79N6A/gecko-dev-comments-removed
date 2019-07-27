





#ifndef nsCRTGlue_h__
#define nsCRTGlue_h__

#include "nscore.h"









NS_COM_GLUE const char* NS_strspnp(const char* aDelims, const char* aStr);

















NS_COM_GLUE char* NS_strtok(const char* aDelims, char** aStr);




NS_COM_GLUE uint32_t NS_strlen(const char16_t* aString);




NS_COM_GLUE int NS_strcmp(const char16_t* aStrA, const char16_t* aStrB);




NS_COM_GLUE char16_t* NS_strdup(const char16_t* aString);




NS_COM_GLUE char* NS_strdup(const char* aString);





NS_COM_GLUE char16_t* NS_strndup(const char16_t* aString, uint32_t aLen);





class NS_COM_GLUE nsLowerUpperUtils
{
public:
  static const unsigned char kLower2Upper[256];
  static const unsigned char kUpper2Lower[256];
};

inline char
NS_ToUpper(char aChar)
{
  return (char)nsLowerUpperUtils::kLower2Upper[(unsigned char)aChar];
}

inline char
NS_ToLower(char aChar)
{
  return (char)nsLowerUpperUtils::kUpper2Lower[(unsigned char)aChar];
}

NS_COM_GLUE bool NS_IsUpper(char aChar);
NS_COM_GLUE bool NS_IsLower(char aChar);

NS_COM_GLUE bool NS_IsAscii(char16_t aChar);
NS_COM_GLUE bool NS_IsAscii(const char16_t* aString);
NS_COM_GLUE bool NS_IsAsciiAlpha(char16_t aChar);
NS_COM_GLUE bool NS_IsAsciiDigit(char16_t aChar);
NS_COM_GLUE bool NS_IsAsciiWhitespace(char16_t aChar);
NS_COM_GLUE bool NS_IsAscii(const char* aString);
NS_COM_GLUE bool NS_IsAscii(const char* aString, uint32_t aLength);

#ifndef XPCOM_GLUE_AVOID_NSPR
NS_COM_GLUE void NS_MakeRandomString(char* aBuf, int32_t aBufLen);
#endif

#define FF '\f'
#define TAB '\t'

#define CRSTR "\015"
#define LFSTR "\012"
#define CRLF "\015\012"     /* A CR LF equivalent string */



#define OS_FILE_ILLEGAL_CHARACTERS "/:*?\"<>|"



#define KNOWN_PATH_SEPARATORS "\\/"

#if defined(XP_MACOSX)
  #define FILE_PATH_SEPARATOR        "/"
#elif defined(XP_WIN)
  #define FILE_PATH_SEPARATOR        "\\"
#elif defined(XP_UNIX)
  #define FILE_PATH_SEPARATOR        "/"
#else
  #error need_to_define_your_file_path_separator_and_maybe_illegal_characters
#endif



#define CONTROL_CHARACTERS     "\001\002\003\004\005\006\007" \
                           "\010\011\012\013\014\015\016\017" \
                           "\020\021\022\023\024\025\026\027" \
                           "\030\031\032\033\034\035\036\037"

#define FILE_ILLEGAL_CHARACTERS CONTROL_CHARACTERS OS_FILE_ILLEGAL_CHARACTERS

#endif 

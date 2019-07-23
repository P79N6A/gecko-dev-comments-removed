




































#ifndef nsStringSupport_h__
#define nsStringSupport_h__

#ifndef MOZILLA_INTERNAL_API

#include "nsStringAPI.h"
#include "nsMemory.h"
#include "prprf.h"
#include "plstr.h"

#ifndef kNotFound
#define kNotFound -1
#endif

inline void
AppendIntToString(nsCString &str, PRInt32 value)
{
  char buf[32];
  PR_snprintf(buf, sizeof(buf), "%d", value);
  str.Append(buf);
}

inline PRInt32
FindCharInString(nsCString &str, char c, PRUint32 offset = 0)
{
  NS_ASSERTION(offset <= str.Length(), "invalid offset");
  const char *data = str.get();
  for (const char *p = data + offset; *p; ++p)
    if (*p == c)
      return p - data;
  return kNotFound;
}

inline PRInt32
FindCharInString(nsString &str, PRUnichar c, PRUint32 offset = 0)
{
  NS_ASSERTION(offset <= str.Length(), "invalid offset");
  const PRUnichar *data = str.get();
  for (const PRUnichar *p = data + offset; *p; ++p)
    if (*p == c)
      return p - data;
  return kNotFound;
}

inline PRInt32
FindInString(nsCString &str, const char *needle, PRBool ignoreCase = PR_FALSE)
{
  const char *data = str.get(), *p;
  if (ignoreCase)
    p = PL_strcasestr(data, needle);
  else
    p = PL_strstr(data, needle);
  return p ? p - data : kNotFound;
}

inline void
NS_CopyUnicodeToNative(const nsAString &input, nsACString &output)
{
  NS_UTF16ToCString(input, NS_CSTRING_ENCODING_NATIVE_FILESYSTEM, output);
}

inline void
NS_CopyNativeToUnicode(const nsACString &input, nsAString &output)
{
  NS_CStringToUTF16(input, NS_CSTRING_ENCODING_NATIVE_FILESYSTEM, output);
}

typedef nsCString nsXPIDLCString;
typedef nsString nsXPIDLString;

#else 

#include "nsString.h"
#include "nsNativeCharsetUtils.h"
#include "nsReadableUtils.h"

inline void
AppendIntToString(nsCString &str, PRInt32 value)
{
  str.AppendInt(value);
}

inline PRInt32
FindCharInString(nsCString &str, char c, PRUint32 offset = 0)
{
  return str.FindChar(c, offset);
}

inline PRInt32
FindCharInString(nsString &str, PRUnichar c, PRUint32 offset = 0)
{
  return str.FindChar(c, offset);
}

inline PRInt32
FindInString(nsCString &str, const char *needle, PRBool ignoreCase = PR_FALSE)
{
  return str.Find(needle, ignoreCase);
}

#endif 

#endif 

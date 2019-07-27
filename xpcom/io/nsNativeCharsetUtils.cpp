





#include "xpcom-private.h"




#if defined(XP_MACOSX) || defined(ANDROID)

#include "nsAString.h"
#include "nsReadableUtils.h"
#include "nsString.h"

nsresult
NS_CopyNativeToUnicode(const nsACString& aInput, nsAString& aOutput)
{
  CopyUTF8toUTF16(aInput, aOutput);
  return NS_OK;
}

nsresult
NS_CopyUnicodeToNative(const nsAString&  aInput, nsACString& aOutput)
{
  CopyUTF16toUTF8(aInput, aOutput);
  return NS_OK;
}

void
NS_StartupNativeCharsetUtils()
{
}

void
NS_ShutdownNativeCharsetUtils()
{
}





#elif defined(XP_UNIX)

#include <stdlib.h>   
#include <locale.h>   
#include "mozilla/Mutex.h"
#include "nscore.h"
#include "nsAString.h"
#include "nsReadableUtils.h"

using namespace mozilla;









#if defined(HAVE_ICONV) && defined(HAVE_NL_TYPES_H) && defined(HAVE_LANGINFO_CODESET)
#define USE_ICONV 1
#else
#define USE_STDCONV 1
#endif

static void
isolatin1_to_utf16(const char** aInput, uint32_t* aInputLeft,
                   char16_t** aOutput, uint32_t* aOutputLeft)
{
  while (*aInputLeft && *aOutputLeft) {
    **aOutput = (unsigned char)** aInput;
    (*aInput)++;
    (*aInputLeft)--;
    (*aOutput)++;
    (*aOutputLeft)--;
  }
}

static void
utf16_to_isolatin1(const char16_t** aInput, uint32_t* aInputLeft,
                   char** aOutput, uint32_t* aOutputLeft)
{
  while (*aInputLeft && *aOutputLeft) {
    **aOutput = (unsigned char)**aInput;
    (*aInput)++;
    (*aInputLeft)--;
    (*aOutput)++;
    (*aOutputLeft)--;
  }
}




#if defined(USE_ICONV)
#include <nl_types.h> 
#include <langinfo.h> 
#include <iconv.h>    
#include <errno.h>
#include "plstr.h"

#if defined(HAVE_ICONV_WITH_CONST_INPUT)
#define ICONV_INPUT(x) (x)
#else
#define ICONV_INPUT(x) ((char **)x)
#endif




#if !defined(__GLIBC__)
#define ENABLE_UTF8_FALLBACK_SUPPORT
#endif

#define INVALID_ICONV_T ((iconv_t)-1)

static inline size_t
xp_iconv(iconv_t converter,
         const char** aInput, size_t* aInputLeft,
         char** aOutput, size_t* aOutputLeft)
{
  size_t res, outputAvail = aOutputLeft ? *aOutputLeft : 0;
  res = iconv(converter, ICONV_INPUT(aInput), aInputLeft, aOutput, aOutputLeft);
  if (res == (size_t)-1) {
    
    
    
    
    
    if ((errno == E2BIG) && (*aOutputLeft < outputAvail)) {
      res = 0;
    }
  }
  return res;
}

static inline void
xp_iconv_reset(iconv_t converter)
{
  
  
  

  const char* zero_char_in_ptr  = nullptr;
  char* zero_char_out_ptr = nullptr;
  size_t zero_size_in = 0;
  size_t zero_size_out = 0;

  xp_iconv(converter,
           &zero_char_in_ptr,
           &zero_size_in,
           &zero_char_out_ptr,
           &zero_size_out);
}

static inline iconv_t
xp_iconv_open(const char** to_list, const char** from_list)
{
  iconv_t res;
  const char** from_name;
  const char** to_name;

  
  to_name = to_list;
  while (*to_name) {
    if (**to_name) {
      from_name = from_list;
      while (*from_name) {
        if (**from_name) {
          res = iconv_open(*to_name, *from_name);
          if (res != INVALID_ICONV_T) {
            return res;
          }
        }
        from_name++;
      }
    }
    to_name++;
  }

  return INVALID_ICONV_T;
}















static const char* UTF_16_NAMES[] = {
#if defined(IS_LITTLE_ENDIAN)
  "UTF-16LE",
#if defined(__GLIBC__)
  "UNICODELITTLE",
#endif
  "UCS-2LE",
#else
  "UTF-16BE",
#if defined(__GLIBC__)
  "UNICODEBIG",
#endif
  "UCS-2BE",
#endif
  "UTF-16",
  "UCS-2",
  "UCS2",
  "UCS_2",
  "ucs-2",
  "ucs2",
  "ucs_2",
  nullptr
};

#if defined(ENABLE_UTF8_FALLBACK_SUPPORT)
static const char* UTF_8_NAMES[] = {
  "UTF-8",
  "UTF8",
  "UTF_8",
  "utf-8",
  "utf8",
  "utf_8",
  nullptr
};
#endif

static const char* ISO_8859_1_NAMES[] = {
  "ISO-8859-1",
#if !defined(__GLIBC__)
  "ISO8859-1",
  "ISO88591",
  "ISO_8859_1",
  "ISO8859_1",
  "iso-8859-1",
  "iso8859-1",
  "iso88591",
  "iso_8859_1",
  "iso8859_1",
#endif
  nullptr
};

class nsNativeCharsetConverter
{
public:
  nsNativeCharsetConverter();
  ~nsNativeCharsetConverter();

  nsresult NativeToUnicode(const char** aInput, uint32_t* aInputLeft,
                           char16_t** aOutput, uint32_t* aOutputLeft);
  nsresult UnicodeToNative(const char16_t** aInput, uint32_t* aInputLeft,
                           char** aOutput, uint32_t* aOutputLeft);

  static void GlobalInit();
  static void GlobalShutdown();
  static bool IsNativeUTF8();

private:
  static iconv_t gNativeToUnicode;
  static iconv_t gUnicodeToNative;
#if defined(ENABLE_UTF8_FALLBACK_SUPPORT)
  static iconv_t gNativeToUTF8;
  static iconv_t gUTF8ToNative;
  static iconv_t gUnicodeToUTF8;
  static iconv_t gUTF8ToUnicode;
#endif
  static Mutex*  gLock;
  static bool    gInitialized;
  static bool    gIsNativeUTF8;

  static void LazyInit();

  static void Lock()
  {
    if (gLock) {
      gLock->Lock();
    }
  }
  static void Unlock()
  {
    if (gLock) {
      gLock->Unlock();
    }
  }
};

iconv_t nsNativeCharsetConverter::gNativeToUnicode = INVALID_ICONV_T;
iconv_t nsNativeCharsetConverter::gUnicodeToNative = INVALID_ICONV_T;
#if defined(ENABLE_UTF8_FALLBACK_SUPPORT)
iconv_t nsNativeCharsetConverter::gNativeToUTF8    = INVALID_ICONV_T;
iconv_t nsNativeCharsetConverter::gUTF8ToNative    = INVALID_ICONV_T;
iconv_t nsNativeCharsetConverter::gUnicodeToUTF8   = INVALID_ICONV_T;
iconv_t nsNativeCharsetConverter::gUTF8ToUnicode   = INVALID_ICONV_T;
#endif
Mutex*  nsNativeCharsetConverter::gLock            = nullptr;
bool    nsNativeCharsetConverter::gInitialized     = false;
bool    nsNativeCharsetConverter::gIsNativeUTF8    = false;

void
nsNativeCharsetConverter::LazyInit()
{
  
  
  
  
  if (!gLock) {
    setlocale(LC_CTYPE, "");
  }
  const char* blank_list[] = { "", nullptr };
  const char** native_charset_list = blank_list;
  const char* native_charset = nl_langinfo(CODESET);
  if (!native_charset) {
    NS_ERROR("native charset is unknown");
    
    native_charset_list = ISO_8859_1_NAMES;
  } else {
    native_charset_list[0] = native_charset;
  }

  
  
  if (!PL_strcasecmp(native_charset, "UTF-8")) {
    gIsNativeUTF8 = true;
  }

  gNativeToUnicode = xp_iconv_open(UTF_16_NAMES, native_charset_list);
  gUnicodeToNative = xp_iconv_open(native_charset_list, UTF_16_NAMES);

#if defined(ENABLE_UTF8_FALLBACK_SUPPORT)
  if (gNativeToUnicode == INVALID_ICONV_T) {
    gNativeToUTF8 = xp_iconv_open(UTF_8_NAMES, native_charset_list);
    gUTF8ToUnicode = xp_iconv_open(UTF_16_NAMES, UTF_8_NAMES);
    NS_ASSERTION(gNativeToUTF8 != INVALID_ICONV_T, "no native to utf-8 converter");
    NS_ASSERTION(gUTF8ToUnicode != INVALID_ICONV_T, "no utf-8 to utf-16 converter");
  }
  if (gUnicodeToNative == INVALID_ICONV_T) {
    gUnicodeToUTF8 = xp_iconv_open(UTF_8_NAMES, UTF_16_NAMES);
    gUTF8ToNative = xp_iconv_open(native_charset_list, UTF_8_NAMES);
    NS_ASSERTION(gUnicodeToUTF8 != INVALID_ICONV_T, "no utf-16 to utf-8 converter");
    NS_ASSERTION(gUTF8ToNative != INVALID_ICONV_T, "no utf-8 to native converter");
  }
#else
  NS_ASSERTION(gNativeToUnicode != INVALID_ICONV_T, "no native to utf-16 converter");
  NS_ASSERTION(gUnicodeToNative != INVALID_ICONV_T, "no utf-16 to native converter");
#endif

  









  char dummy_input[1] = { ' ' };
  char dummy_output[4];

  if (gNativeToUnicode != INVALID_ICONV_T) {
    const char* input = dummy_input;
    size_t input_left = sizeof(dummy_input);
    char* output = dummy_output;
    size_t output_left = sizeof(dummy_output);

    xp_iconv(gNativeToUnicode, &input, &input_left, &output, &output_left);
  }
#if defined(ENABLE_UTF8_FALLBACK_SUPPORT)
  if (gUTF8ToUnicode != INVALID_ICONV_T) {
    const char* input = dummy_input;
    size_t input_left = sizeof(dummy_input);
    char* output = dummy_output;
    size_t output_left = sizeof(dummy_output);

    xp_iconv(gUTF8ToUnicode, &input, &input_left, &output, &output_left);
  }
#endif

  gInitialized = true;
}

void
nsNativeCharsetConverter::GlobalInit()
{
  gLock = new Mutex("nsNativeCharsetConverter.gLock");
}

void
nsNativeCharsetConverter::GlobalShutdown()
{
  delete gLock;
  gLock = nullptr;

  if (gNativeToUnicode != INVALID_ICONV_T) {
    iconv_close(gNativeToUnicode);
    gNativeToUnicode = INVALID_ICONV_T;
  }

  if (gUnicodeToNative != INVALID_ICONV_T) {
    iconv_close(gUnicodeToNative);
    gUnicodeToNative = INVALID_ICONV_T;
  }

#if defined(ENABLE_UTF8_FALLBACK_SUPPORT)
  if (gNativeToUTF8 != INVALID_ICONV_T) {
    iconv_close(gNativeToUTF8);
    gNativeToUTF8 = INVALID_ICONV_T;
  }
  if (gUTF8ToNative != INVALID_ICONV_T) {
    iconv_close(gUTF8ToNative);
    gUTF8ToNative = INVALID_ICONV_T;
  }
  if (gUnicodeToUTF8 != INVALID_ICONV_T) {
    iconv_close(gUnicodeToUTF8);
    gUnicodeToUTF8 = INVALID_ICONV_T;
  }
  if (gUTF8ToUnicode != INVALID_ICONV_T) {
    iconv_close(gUTF8ToUnicode);
    gUTF8ToUnicode = INVALID_ICONV_T;
  }
#endif

  gInitialized = false;
}

nsNativeCharsetConverter::nsNativeCharsetConverter()
{
  Lock();
  if (!gInitialized) {
    LazyInit();
  }
}

nsNativeCharsetConverter::~nsNativeCharsetConverter()
{
  
  if (gNativeToUnicode != INVALID_ICONV_T) {
    xp_iconv_reset(gNativeToUnicode);
  }
  if (gUnicodeToNative != INVALID_ICONV_T) {
    xp_iconv_reset(gUnicodeToNative);
  }
#if defined(ENABLE_UTF8_FALLBACK_SUPPORT)
  if (gNativeToUTF8 != INVALID_ICONV_T) {
    xp_iconv_reset(gNativeToUTF8);
  }
  if (gUTF8ToNative != INVALID_ICONV_T) {
    xp_iconv_reset(gUTF8ToNative);
  }
  if (gUnicodeToUTF8 != INVALID_ICONV_T) {
    xp_iconv_reset(gUnicodeToUTF8);
  }
  if (gUTF8ToUnicode != INVALID_ICONV_T) {
    xp_iconv_reset(gUTF8ToUnicode);
  }
#endif
  Unlock();
}

nsresult
nsNativeCharsetConverter::NativeToUnicode(const char** aInput,
                                          uint32_t* aInputLeft,
                                          char16_t** aOutput,
                                          uint32_t* aOutputLeft)
{
  size_t res = 0;
  size_t inLeft = (size_t)*aInputLeft;
  size_t outLeft = (size_t)*aOutputLeft * 2;

  if (gNativeToUnicode != INVALID_ICONV_T) {

    res = xp_iconv(gNativeToUnicode, aInput, &inLeft, (char**)aOutput, &outLeft);

    *aInputLeft = inLeft;
    *aOutputLeft = outLeft / 2;
    if (res != (size_t)-1) {
      return NS_OK;
    }

    NS_WARNING("conversion from native to utf-16 failed");

    
    xp_iconv_reset(gNativeToUnicode);
  }
#if defined(ENABLE_UTF8_FALLBACK_SUPPORT)
  else if ((gNativeToUTF8 != INVALID_ICONV_T) &&
           (gUTF8ToUnicode != INVALID_ICONV_T)) {
    
    const char* in = *aInput;

    char ubuf[1024];

    
    
    while (inLeft) {
      char* p = ubuf;
      size_t n = sizeof(ubuf);
      res = xp_iconv(gNativeToUTF8, &in, &inLeft, &p, &n);
      if (res == (size_t)-1) {
        NS_ERROR("conversion from native to utf-8 failed");
        break;
      }
      NS_ASSERTION(outLeft > 0, "bad assumption");
      p = ubuf;
      n = sizeof(ubuf) - n;
      res = xp_iconv(gUTF8ToUnicode, (const char**)&p, &n,
                     (char**)aOutput, &outLeft);
      if (res == (size_t)-1) {
        NS_ERROR("conversion from utf-8 to utf-16 failed");
        break;
      }
    }

    (*aInput) += (*aInputLeft - inLeft);
    *aInputLeft = inLeft;
    *aOutputLeft = outLeft / 2;

    if (res != (size_t)-1) {
      return NS_OK;
    }

    
    xp_iconv_reset(gNativeToUTF8);
    xp_iconv_reset(gUTF8ToUnicode);
  }
#endif

  
  
  isolatin1_to_utf16(aInput, aInputLeft, aOutput, aOutputLeft);

  return NS_OK;
}

nsresult
nsNativeCharsetConverter::UnicodeToNative(const char16_t** aInput,
                                          uint32_t* aInputLeft,
                                          char** aOutput,
                                          uint32_t* aOutputLeft)
{
  size_t res = 0;
  size_t inLeft = (size_t)*aInputLeft * 2;
  size_t outLeft = (size_t)*aOutputLeft;

  if (gUnicodeToNative != INVALID_ICONV_T) {
    res = xp_iconv(gUnicodeToNative, (const char**)aInput, &inLeft,
                   aOutput, &outLeft);

    *aInputLeft = inLeft / 2;
    *aOutputLeft = outLeft;
    if (res != (size_t)-1) {
      return NS_OK;
    }

    NS_ERROR("iconv failed");

    
    xp_iconv_reset(gUnicodeToNative);
  }
#if defined(ENABLE_UTF8_FALLBACK_SUPPORT)
  else if ((gUnicodeToUTF8 != INVALID_ICONV_T) &&
           (gUTF8ToNative != INVALID_ICONV_T)) {
    const char* in = (const char*)*aInput;

    char ubuf[6]; 

    
    while (inLeft && outLeft) {
      char* p = ubuf;
      size_t n = sizeof(ubuf), one_uchar = sizeof(char16_t);
      res = xp_iconv(gUnicodeToUTF8, &in, &one_uchar, &p, &n);
      if (res == (size_t)-1) {
        NS_ERROR("conversion from utf-16 to utf-8 failed");
        break;
      }
      p = ubuf;
      n = sizeof(ubuf) - n;
      res = xp_iconv(gUTF8ToNative, (const char**)&p, &n, aOutput, &outLeft);
      if (res == (size_t)-1) {
        if (errno == E2BIG) {
          
          in -= sizeof(char16_t);
          res = 0;
        } else {
          NS_ERROR("conversion from utf-8 to native failed");
        }
        break;
      }
      inLeft -= sizeof(char16_t);
    }

    (*aInput) += (*aInputLeft - inLeft / 2);
    *aInputLeft = inLeft / 2;
    *aOutputLeft = outLeft;
    if (res != (size_t)-1) {
      return NS_OK;
    }

    
    xp_iconv_reset(gUnicodeToUTF8);
    xp_iconv_reset(gUTF8ToNative);
  }
#endif

  
  
  utf16_to_isolatin1(aInput, aInputLeft, aOutput, aOutputLeft);

  return NS_OK;
}

bool
nsNativeCharsetConverter::IsNativeUTF8()
{
  if (!gInitialized) {
    Lock();
    if (!gInitialized) {
      LazyInit();
    }
    Unlock();
  }
  return gIsNativeUTF8;
}

#endif 




#if defined(USE_STDCONV)
#if defined(HAVE_WCRTOMB) || defined(HAVE_MBRTOWC)
#include <wchar.h>    
#endif

class nsNativeCharsetConverter
{
public:
  nsNativeCharsetConverter();

  nsresult NativeToUnicode(const char** aInput, uint32_t* aInputLeft,
                           char16_t** aOutput, uint32_t* aOutputLeft);
  nsresult UnicodeToNative(const char16_t** aInput, uint32_t* aInputLeft,
                           char** aOutput, uint32_t* aOutputLeft);

  static void GlobalInit();
  static void GlobalShutdown() { }
  static bool IsNativeUTF8();

private:
  static bool gWCharIsUnicode;

#if defined(HAVE_WCRTOMB) || defined(HAVE_MBRTOWC)
  mbstate_t ps;
#endif
};

bool nsNativeCharsetConverter::gWCharIsUnicode = false;

nsNativeCharsetConverter::nsNativeCharsetConverter()
{
#if defined(HAVE_WCRTOMB) || defined(HAVE_MBRTOWC)
  memset(&ps, 0, sizeof(ps));
#endif
}

void
nsNativeCharsetConverter::GlobalInit()
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  char a = 'a';
  unsigned int w = 0;

  int res = mbtowc((wchar_t*)&w, &a, 1);

  gWCharIsUnicode = (res != -1 && w == 'a');

#ifdef DEBUG
  if (!gWCharIsUnicode) {
    NS_WARNING("wchar_t is not unicode (unicode conversion will be lossy)");
  }
#endif
}

nsresult
nsNativeCharsetConverter::NativeToUnicode(const char** aInput,
                                          uint32_t* aInputLeft,
                                          char16_t** aOutput,
                                          uint32_t* aOutputLeft)
{
  if (gWCharIsUnicode) {
    int incr;

    
    
    unsigned int tmp = 0;
    while (*aInputLeft && *aOutputLeft) {
#ifdef HAVE_MBRTOWC
      incr = (int)mbrtowc((wchar_t*)&tmp, *aInput, *aInputLeft, &ps);
#else
      
      incr = (int)mbtowc((wchar_t*)&tmp, *aInput, *aInputLeft);
#endif
      if (incr < 0) {
        NS_WARNING("mbtowc failed: possible charset mismatch");
        
        tmp = (unsigned char)**aInput;
        incr = 1;
      }
      ** aOutput = (char16_t)tmp;
      (*aInput) += incr;
      (*aInputLeft) -= incr;
      (*aOutput)++;
      (*aOutputLeft)--;
    }
  } else {
    
    
    isolatin1_to_utf16(aInput, aInputLeft, aOutput, aOutputLeft);
  }

  return NS_OK;
}

nsresult
nsNativeCharsetConverter::UnicodeToNative(const char16_t** aInput,
                                          uint32_t* aInputLeft,
                                          char** aOutput,
                                          uint32_t* aOutputLeft)
{
  if (gWCharIsUnicode) {
    int incr;

    while (*aInputLeft && *aOutputLeft >= MB_CUR_MAX) {
#ifdef HAVE_WCRTOMB
      incr = (int)wcrtomb(*aOutput, (wchar_t)**aInput, &ps);
#else
      
      incr = (int)wctomb(*aOutput, (wchar_t)**aInput);
#endif
      if (incr < 0) {
        NS_WARNING("mbtowc failed: possible charset mismatch");
        ** aOutput = (unsigned char)**aInput; 
        incr = 1;
      }
      
      NS_ASSERTION(uint32_t(incr) <= *aOutputLeft, "wrote beyond end of string");
      (*aOutput) += incr;
      (*aOutputLeft) -= incr;
      (*aInput)++;
      (*aInputLeft)--;
    }
  } else {
    
    
    utf16_to_isolatin1(aInput, aInputLeft, aOutput, aOutputLeft);
  }

  return NS_OK;
}


bool
nsNativeCharsetConverter::IsNativeUTF8()
{
  return false;
}

#endif 





nsresult
NS_CopyNativeToUnicode(const nsACString& aInput, nsAString& aOutput)
{
  aOutput.Truncate();

  uint32_t inputLen = aInput.Length();

  nsACString::const_iterator iter;
  aInput.BeginReading(iter);

  
  
  
  
  
  
  
  if (!aOutput.SetLength(inputLen, fallible_t())) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nsAString::iterator out_iter;
  aOutput.BeginWriting(out_iter);

  char16_t* result = out_iter.get();
  uint32_t resultLeft = inputLen;

  const char* buf = iter.get();
  uint32_t bufLeft = inputLen;

  nsNativeCharsetConverter conv;
  nsresult rv = conv.NativeToUnicode(&buf, &bufLeft, &result, &resultLeft);
  if (NS_SUCCEEDED(rv)) {
    NS_ASSERTION(bufLeft == 0, "did not consume entire input buffer");
    aOutput.SetLength(inputLen - resultLeft);
  }
  return rv;
}

nsresult
NS_CopyUnicodeToNative(const nsAString& aInput, nsACString& aOutput)
{
  aOutput.Truncate();

  nsAString::const_iterator iter, end;
  aInput.BeginReading(iter);
  aInput.EndReading(end);

  
  char temp[4096];

  nsNativeCharsetConverter conv;

  const char16_t* buf = iter.get();
  uint32_t bufLeft = Distance(iter, end);
  while (bufLeft) {
    char* p = temp;
    uint32_t tempLeft = sizeof(temp);

    nsresult rv = conv.UnicodeToNative(&buf, &bufLeft, &p, &tempLeft);
    if (NS_FAILED(rv)) {
      return rv;
    }

    if (tempLeft < sizeof(temp)) {
      aOutput.Append(temp, sizeof(temp) - tempLeft);
    }
  }
  return NS_OK;
}

bool
NS_IsNativeUTF8()
{
  return nsNativeCharsetConverter::IsNativeUTF8();
}

void
NS_StartupNativeCharsetUtils()
{
  
  
  
  
  
  
  
  
  setlocale(LC_CTYPE, "");

  nsNativeCharsetConverter::GlobalInit();
}

void
NS_ShutdownNativeCharsetUtils()
{
  nsNativeCharsetConverter::GlobalShutdown();
}




#elif defined(XP_WIN)

#include <windows.h>
#include "nsString.h"
#include "nsAString.h"
#include "nsReadableUtils.h"

using namespace mozilla;

nsresult
NS_CopyNativeToUnicode(const nsACString& aInput, nsAString& aOutput)
{
  uint32_t inputLen = aInput.Length();

  nsACString::const_iterator iter;
  aInput.BeginReading(iter);

  const char* buf = iter.get();

  
  uint32_t resultLen = 0;
  int n = ::MultiByteToWideChar(CP_ACP, 0, buf, inputLen, nullptr, 0);
  if (n > 0) {
    resultLen += n;
  }

  
  if (!aOutput.SetLength(resultLen, fallible_t())) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  if (resultLen > 0) {
    nsAString::iterator out_iter;
    aOutput.BeginWriting(out_iter);

    char16_t* result = out_iter.get();

    ::MultiByteToWideChar(CP_ACP, 0, buf, inputLen, wwc(result), resultLen);
  }
  return NS_OK;
}

nsresult
NS_CopyUnicodeToNative(const nsAString&  aInput, nsACString& aOutput)
{
  uint32_t inputLen = aInput.Length();

  nsAString::const_iterator iter;
  aInput.BeginReading(iter);

  char16ptr_t buf = iter.get();

  
  uint32_t resultLen = 0;

  int n = ::WideCharToMultiByte(CP_ACP, 0, buf, inputLen, nullptr, 0,
                                nullptr, nullptr);
  if (n > 0) {
    resultLen += n;
  }

  
  if (!aOutput.SetLength(resultLen, fallible_t())) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  if (resultLen > 0) {
    nsACString::iterator out_iter;
    aOutput.BeginWriting(out_iter);

    
    
    const char defaultChar = '_';

    char* result = out_iter.get();

    ::WideCharToMultiByte(CP_ACP, 0, buf, inputLen, result, resultLen,
                          &defaultChar, nullptr);
  }
  return NS_OK;
}


int32_t
NS_ConvertAtoW(const char* aStrInA, int aBufferSize, char16_t* aStrOutW)
{
  return MultiByteToWideChar(CP_ACP, 0, aStrInA, -1, wwc(aStrOutW), aBufferSize);
}

int32_t
NS_ConvertWtoA(const char16_t* aStrInW, int aBufferSizeOut,
               char* aStrOutA, const char* aDefault)
{
  if ((!aStrInW) || (!aStrOutA) || (aBufferSizeOut <= 0)) {
    return 0;
  }

  int numCharsConverted = WideCharToMultiByte(CP_ACP, 0, char16ptr_t(aStrInW), -1,
                                              aStrOutA, aBufferSizeOut,
                                              aDefault, nullptr);

  if (!numCharsConverted) {
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
      
      aStrOutA[aBufferSizeOut - 1] = '\0';
    } else {
      
      aStrOutA[0] = '\0';
    }
  } else if (numCharsConverted < aBufferSizeOut) {
    
    aStrOutA[numCharsConverted] = '\0';
  }

  return numCharsConverted;
}

#else

#include "nsReadableUtils.h"

nsresult
NS_CopyNativeToUnicode(const nsACString& aInput, nsAString& aOutput)
{
  CopyASCIItoUTF16(aInput, aOutput);
  return NS_OK;
}

nsresult
NS_CopyUnicodeToNative(const nsAString& aInput, nsACString& aOutput)
{
  LossyCopyUTF16toASCII(aInput, aOutput);
  return NS_OK;
}

void
NS_StartupNativeCharsetUtils()
{
}

void
NS_ShutdownNativeCharsetUtils()
{
}

#endif

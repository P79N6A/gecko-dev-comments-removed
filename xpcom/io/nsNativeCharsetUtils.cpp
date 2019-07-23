









































#include "xpcom-private.h"




#if defined(XP_BEOS) || defined(XP_MACOSX)

#include "nsAString.h"
#include "nsReadableUtils.h"
#include "nsString.h"

NS_COM nsresult
NS_CopyNativeToUnicode(const nsACString &input, nsAString  &output)
{
    CopyUTF8toUTF16(input, output);
    return NS_OK;
}

NS_COM nsresult
NS_CopyUnicodeToNative(const nsAString  &input, nsACString &output)
{
    CopyUTF16toUTF8(input, output);
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
#include "nscore.h"
#include "prlock.h"
#include "nsAString.h"
#include "nsReadableUtils.h"









#if defined(HAVE_ICONV) && defined(HAVE_NL_TYPES_H) && defined(HAVE_LANGINFO_CODESET)
#define USE_ICONV 1
#else
#define USE_STDCONV 1
#endif

static void
isolatin1_to_utf16(const char **input, PRUint32 *inputLeft, PRUnichar **output, PRUint32 *outputLeft)
{
    while (*inputLeft && *outputLeft) {
        **output = (unsigned char) **input;
        (*input)++;
        (*inputLeft)--;
        (*output)++;
        (*outputLeft)--;
    }
}

static void
utf16_to_isolatin1(const PRUnichar **input, PRUint32 *inputLeft, char **output, PRUint32 *outputLeft)
{
    while (*inputLeft && *outputLeft) {
        **output = (unsigned char) **input;
        (*input)++;
        (*inputLeft)--;
        (*output)++;
        (*outputLeft)--;
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

#define INVALID_ICONV_T ((iconv_t) -1)

static inline size_t
xp_iconv(iconv_t converter,
         const char **input,
         size_t      *inputLeft,
         char       **output,
         size_t      *outputLeft)
{
    size_t res, outputAvail = outputLeft ? *outputLeft : 0;
    res = iconv(converter, ICONV_INPUT(input), inputLeft, output, outputLeft);
    if (res == (size_t) -1) {
        
        
        
        
        
        if ((errno == E2BIG) && (*outputLeft < outputAvail))
            res = 0;
    }
    return res;
}

static inline void
xp_iconv_reset(iconv_t converter)
{
    
    
    
    
    const char *zero_char_in_ptr  = NULL;
    char       *zero_char_out_ptr = NULL;
    size_t      zero_size_in      = 0,
                zero_size_out     = 0;

    xp_iconv(converter, &zero_char_in_ptr,
                        &zero_size_in,
                        &zero_char_out_ptr,
                        &zero_size_out);
}

static inline iconv_t
xp_iconv_open(const char **to_list, const char **from_list)
{
    iconv_t res;
    const char **from_name;
    const char **to_name;

    
    to_name = to_list;
    while (*to_name) {
        if (**to_name) {
            from_name = from_list;
            while (*from_name) {
                if (**from_name) {
                    res = iconv_open(*to_name, *from_name);
                    if (res != INVALID_ICONV_T)
                        return res;
                }
                from_name++;
            }
        }
        to_name++;
    }

    return INVALID_ICONV_T;
}















static const char *UTF_16_NAMES[] = {
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
    NULL
};

#if defined(ENABLE_UTF8_FALLBACK_SUPPORT)
static const char *UTF_8_NAMES[] = {
    "UTF-8",
    "UTF8",
    "UTF_8",
    "utf-8",
    "utf8",
    "utf_8",
    NULL
};
#endif

static const char *ISO_8859_1_NAMES[] = {
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
    NULL
};

class nsNativeCharsetConverter
{
public:
    nsNativeCharsetConverter();
   ~nsNativeCharsetConverter();

    nsresult NativeToUnicode(const char      **input , PRUint32 *inputLeft,
                             PRUnichar       **output, PRUint32 *outputLeft);
    nsresult UnicodeToNative(const PRUnichar **input , PRUint32 *inputLeft,
                             char            **output, PRUint32 *outputLeft);

    static void GlobalInit();
    static void GlobalShutdown();
    static PRBool IsNativeUTF8();

private:
    static iconv_t gNativeToUnicode;
    static iconv_t gUnicodeToNative;
#if defined(ENABLE_UTF8_FALLBACK_SUPPORT)
    static iconv_t gNativeToUTF8;
    static iconv_t gUTF8ToNative;
    static iconv_t gUnicodeToUTF8;
    static iconv_t gUTF8ToUnicode;
#endif
    static PRLock *gLock;
    static PRBool  gInitialized;
    static PRBool  gIsNativeUTF8;

    static void LazyInit();

    static void Lock()   { if (gLock) PR_Lock(gLock);   }
    static void Unlock() { if (gLock) PR_Unlock(gLock); }
};

iconv_t nsNativeCharsetConverter::gNativeToUnicode = INVALID_ICONV_T;
iconv_t nsNativeCharsetConverter::gUnicodeToNative = INVALID_ICONV_T;
#if defined(ENABLE_UTF8_FALLBACK_SUPPORT)
iconv_t nsNativeCharsetConverter::gNativeToUTF8    = INVALID_ICONV_T;
iconv_t nsNativeCharsetConverter::gUTF8ToNative    = INVALID_ICONV_T;
iconv_t nsNativeCharsetConverter::gUnicodeToUTF8   = INVALID_ICONV_T;
iconv_t nsNativeCharsetConverter::gUTF8ToUnicode   = INVALID_ICONV_T;
#endif
PRLock *nsNativeCharsetConverter::gLock            = nsnull;
PRBool  nsNativeCharsetConverter::gInitialized     = PR_FALSE;
PRBool  nsNativeCharsetConverter::gIsNativeUTF8    = PR_FALSE;

void
nsNativeCharsetConverter::LazyInit()
{
    const char  *blank_list[] = { "", NULL };
    const char **native_charset_list = blank_list;
    const char  *native_charset = nl_langinfo(CODESET);
    if (native_charset == nsnull) {
        NS_ERROR("native charset is unknown");
        
        native_charset_list = ISO_8859_1_NAMES;
    }
    else
        native_charset_list[0] = native_charset;

    
    
    if (!PL_strcasecmp(native_charset, "UTF-8"))
        gIsNativeUTF8 = PR_TRUE;

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
	const char *input = dummy_input;
	size_t input_left = sizeof(dummy_input);
	char *output = dummy_output;
	size_t output_left = sizeof(dummy_output);

	xp_iconv(gNativeToUnicode, &input, &input_left, &output, &output_left);
    }
#if defined(ENABLE_UTF8_FALLBACK_SUPPORT)
    if (gUTF8ToUnicode != INVALID_ICONV_T) {
	const char *input = dummy_input;
	size_t input_left = sizeof(dummy_input);
	char *output = dummy_output;
	size_t output_left = sizeof(dummy_output);

	xp_iconv(gUTF8ToUnicode, &input, &input_left, &output, &output_left);
    }
#endif

    gInitialized = PR_TRUE;
}

void
nsNativeCharsetConverter::GlobalInit()
{
    gLock = PR_NewLock();
    NS_ASSERTION(gLock, "lock creation failed");
}

void
nsNativeCharsetConverter::GlobalShutdown()
{
    if (gLock) {
        PR_DestroyLock(gLock);
        gLock = nsnull;
    }

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

    gInitialized = PR_FALSE;
}

nsNativeCharsetConverter::nsNativeCharsetConverter()
{
    Lock();
    if (!gInitialized)
        LazyInit();
}

nsNativeCharsetConverter::~nsNativeCharsetConverter()
{
    
    if (gNativeToUnicode != INVALID_ICONV_T)
        xp_iconv_reset(gNativeToUnicode);
    if (gUnicodeToNative != INVALID_ICONV_T)
        xp_iconv_reset(gUnicodeToNative);
#if defined(ENABLE_UTF8_FALLBACK_SUPPORT)
    if (gNativeToUTF8 != INVALID_ICONV_T)
        xp_iconv_reset(gNativeToUTF8);
    if (gUTF8ToNative != INVALID_ICONV_T)
        xp_iconv_reset(gUTF8ToNative);
    if (gUnicodeToUTF8 != INVALID_ICONV_T)
        xp_iconv_reset(gUnicodeToUTF8);
    if (gUTF8ToUnicode != INVALID_ICONV_T)
        xp_iconv_reset(gUTF8ToUnicode);
#endif
    Unlock();
}

nsresult
nsNativeCharsetConverter::NativeToUnicode(const char **input,
                                          PRUint32    *inputLeft,
                                          PRUnichar  **output,
                                          PRUint32    *outputLeft)
{
    size_t res = 0;
    size_t inLeft = (size_t) *inputLeft;
    size_t outLeft = (size_t) *outputLeft * 2;

    if (gNativeToUnicode != INVALID_ICONV_T) {

        res = xp_iconv(gNativeToUnicode, input, &inLeft, (char **) output, &outLeft);

        *inputLeft = inLeft;
        *outputLeft = outLeft / 2;
        if (res != (size_t) -1) 
            return NS_OK;

        NS_WARNING("conversion from native to utf-16 failed");

        
        xp_iconv_reset(gNativeToUnicode);
    }
#if defined(ENABLE_UTF8_FALLBACK_SUPPORT)
    else if ((gNativeToUTF8 != INVALID_ICONV_T) &&
             (gUTF8ToUnicode != INVALID_ICONV_T)) {
        
        const char *in = *input;

        char ubuf[1024];

        
        
        while (inLeft) {
            char *p = ubuf;
            size_t n = sizeof(ubuf);
            res = xp_iconv(gNativeToUTF8, &in, &inLeft, &p, &n);
            if (res == (size_t) -1) {
                NS_ERROR("conversion from native to utf-8 failed");
                break;
            }
            NS_ASSERTION(outLeft > 0, "bad assumption");
            p = ubuf;
            n = sizeof(ubuf) - n;
            res = xp_iconv(gUTF8ToUnicode, (const char **) &p, &n, (char **) output, &outLeft);
            if (res == (size_t) -1) {
                NS_ERROR("conversion from utf-8 to utf-16 failed");
                break;
            }
        }

        (*input) += (*inputLeft - inLeft);
        *inputLeft = inLeft;
        *outputLeft = outLeft / 2;

        if (res != (size_t) -1) 
            return NS_OK;

        
        xp_iconv_reset(gNativeToUTF8);
        xp_iconv_reset(gUTF8ToUnicode);
    }
#endif

    
    
    isolatin1_to_utf16(input, inputLeft, output, outputLeft);

    return NS_OK;
}

nsresult
nsNativeCharsetConverter::UnicodeToNative(const PRUnichar **input,
                                          PRUint32         *inputLeft,
                                          char            **output,
                                          PRUint32         *outputLeft)
{
    size_t res = 0;
    size_t inLeft = (size_t) *inputLeft * 2;
    size_t outLeft = (size_t) *outputLeft;

    if (gUnicodeToNative != INVALID_ICONV_T) {
        res = xp_iconv(gUnicodeToNative, (const char **) input, &inLeft, output, &outLeft);

        if (res != (size_t) -1) {
            *inputLeft = inLeft / 2;
            *outputLeft = outLeft;
            return NS_OK;
        }

        NS_ERROR("iconv failed");

        
        xp_iconv_reset(gUnicodeToNative);
    }
#if defined(ENABLE_UTF8_FALLBACK_SUPPORT)
    else if ((gUnicodeToUTF8 != INVALID_ICONV_T) &&
             (gUTF8ToNative != INVALID_ICONV_T)) {
        const char *in = (const char *) *input;

        char ubuf[6]; 

        
        while (inLeft && outLeft) {
            char *p = ubuf;
            size_t n = sizeof(ubuf), one_uchar = sizeof(PRUnichar);
            res = xp_iconv(gUnicodeToUTF8, &in, &one_uchar, &p, &n);
            if (res == (size_t) -1) {
                NS_ERROR("conversion from utf-16 to utf-8 failed");
                break;
            }
            p = ubuf;
            n = sizeof(ubuf) - n;
            res = xp_iconv(gUTF8ToNative, (const char **) &p, &n, output, &outLeft);
            if (res == (size_t) -1) {
                if (errno == E2BIG) {
                    
                    in -= sizeof(PRUnichar);
                    res = 0;
                }
                else
                    NS_ERROR("conversion from utf-8 to native failed");
                break;
            }
            inLeft -= sizeof(PRUnichar);
        }

        if (res != (size_t) -1) {
            (*input) += (*inputLeft - inLeft/2);
            *inputLeft = inLeft/2;
            *outputLeft = outLeft;
            return NS_OK;
        }

        
        xp_iconv_reset(gUnicodeToUTF8);
        xp_iconv_reset(gUTF8ToNative);
    }
#endif

    
    utf16_to_isolatin1(input, inputLeft, output, outputLeft);

    return NS_OK;
}

PRBool
nsNativeCharsetConverter::IsNativeUTF8()
{
    if (!gInitialized) {
        Lock();
        if (!gInitialized)
           LazyInit();
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

    nsresult NativeToUnicode(const char      **input , PRUint32 *inputLeft,
                             PRUnichar       **output, PRUint32 *outputLeft);
    nsresult UnicodeToNative(const PRUnichar **input , PRUint32 *inputLeft,
                             char            **output, PRUint32 *outputLeft);

    static void GlobalInit();
    static void GlobalShutdown() { }
    static PRBool IsNativeUTF8();

private:
    static PRBool gWCharIsUnicode;

#if defined(HAVE_WCRTOMB) || defined(HAVE_MBRTOWC)
    mbstate_t ps;
#endif
};

PRBool nsNativeCharsetConverter::gWCharIsUnicode = PR_FALSE;

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

    int res = mbtowc((wchar_t *) &w, &a, 1);

    gWCharIsUnicode = (res != -1 && w == 'a');

#ifdef DEBUG
    if (!gWCharIsUnicode)
        NS_WARNING("wchar_t is not unicode (unicode conversion will be lossy)");
#endif
}

nsresult
nsNativeCharsetConverter::NativeToUnicode(const char **input,
                                          PRUint32    *inputLeft,
                                          PRUnichar  **output,
                                          PRUint32    *outputLeft)
{
    if (gWCharIsUnicode) {
        int incr;

        
        
        unsigned int tmp = 0;
        while (*inputLeft && *outputLeft) {
#ifdef HAVE_MBRTOWC
            incr = (int) mbrtowc((wchar_t *) &tmp, *input, *inputLeft, &ps);
#else
            
            incr = (int) mbtowc((wchar_t *) &tmp, *input, *inputLeft);
#endif
            if (incr < 0) {
                NS_WARNING("mbtowc failed: possible charset mismatch");
                
                tmp = (unsigned char) **input;
                incr = 1;
            }
            **output = (PRUnichar) tmp;
            (*input) += incr;
            (*inputLeft) -= incr;
            (*output)++;
            (*outputLeft)--;
        }
    }
    else {
        
        
        isolatin1_to_utf16(input, inputLeft, output, outputLeft);
    }

    return NS_OK;
}

nsresult
nsNativeCharsetConverter::UnicodeToNative(const PRUnichar **input,
                                          PRUint32         *inputLeft,
                                          char            **output,
                                          PRUint32         *outputLeft)
{
    if (gWCharIsUnicode) {
        int incr;

        while (*inputLeft && *outputLeft >= MB_CUR_MAX) {
#ifdef HAVE_WCRTOMB
            incr = (int) wcrtomb(*output, (wchar_t) **input, &ps);
#else
            
            incr = (int) wctomb(*output, (wchar_t) **input);
#endif
            if (incr < 0) {
                NS_WARNING("mbtowc failed: possible charset mismatch");
                **output = (unsigned char) **input; 
                incr = 1;
            }
            
            NS_ASSERTION(PRUint32(incr) <= *outputLeft, "wrote beyond end of string");
            (*output) += incr;
            (*outputLeft) -= incr;
            (*input)++;
            (*inputLeft)--;
        }
    }
    else {
        
        
        utf16_to_isolatin1(input, inputLeft, output, outputLeft);
    }

    return NS_OK;
}


PRBool
nsNativeCharsetConverter::IsNativeUTF8()
{
    return PR_FALSE;
}

#endif 





NS_COM nsresult
NS_CopyNativeToUnicode(const nsACString &input, nsAString &output)
{
    output.Truncate();

    PRUint32 inputLen = input.Length();

    nsACString::const_iterator iter;
    input.BeginReading(iter);

    
    
    
    
    
    
    
    if (!EnsureStringLength(output, inputLen))
        return NS_ERROR_OUT_OF_MEMORY;
    nsAString::iterator out_iter;
    output.BeginWriting(out_iter);

    PRUnichar *result = out_iter.get();
    PRUint32 resultLeft = inputLen;

    const char *buf = iter.get();
    PRUint32 bufLeft = inputLen;

    nsNativeCharsetConverter conv;
    nsresult rv = conv.NativeToUnicode(&buf, &bufLeft, &result, &resultLeft);
    if (NS_SUCCEEDED(rv)) {
        NS_ASSERTION(bufLeft == 0, "did not consume entire input buffer");
        output.SetLength(inputLen - resultLeft);
    }
    return rv;
}

NS_COM nsresult
NS_CopyUnicodeToNative(const nsAString &input, nsACString &output)
{
    output.Truncate();

    nsAString::const_iterator iter, end;
    input.BeginReading(iter);
    input.EndReading(end);

    
    char temp[4096];

    nsNativeCharsetConverter conv;

    const PRUnichar *buf = iter.get();
    PRUint32 bufLeft = Distance(iter, end);
    while (bufLeft) {
        char *p = temp;
        PRUint32 tempLeft = sizeof(temp);

        nsresult rv = conv.UnicodeToNative(&buf, &bufLeft, &p, &tempLeft);
        if (NS_FAILED(rv)) return rv;

        if (tempLeft < sizeof(temp))
            output.Append(temp, sizeof(temp) - tempLeft);
    }
    return NS_OK;
}

NS_COM PRBool
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
#include "nsAString.h"
#include "nsReadableUtils.h"

NS_COM nsresult
NS_CopyNativeToUnicode(const nsACString &input, nsAString &output)
{
    PRUint32 inputLen = input.Length();

    nsACString::const_iterator iter;
    input.BeginReading(iter);

    const char *buf = iter.get();

    
    PRUint32 resultLen = 0;
    int n = ::MultiByteToWideChar(CP_ACP, 0, buf, inputLen, NULL, 0);
    if (n > 0)
        resultLen += n;

    
    if (!EnsureStringLength(output, resultLen))
        return NS_ERROR_OUT_OF_MEMORY;
    if (resultLen > 0) {
        nsAString::iterator out_iter;
        output.BeginWriting(out_iter);

        PRUnichar *result = out_iter.get();

        ::MultiByteToWideChar(CP_ACP, 0, buf, inputLen, result, resultLen);
    }
    return NS_OK;
}

NS_COM nsresult
NS_CopyUnicodeToNative(const nsAString  &input, nsACString &output)
{
    PRUint32 inputLen = input.Length();

    nsAString::const_iterator iter;
    input.BeginReading(iter);

    const PRUnichar *buf = iter.get();

    
    PRUint32 resultLen = 0;

    int n = ::WideCharToMultiByte(CP_ACP, 0, buf, inputLen, NULL, 0, NULL, NULL);
    if (n > 0)
        resultLen += n;

    
    if (!EnsureStringLength(output, resultLen))
        return NS_ERROR_OUT_OF_MEMORY;
    if (resultLen > 0) {
        nsACString::iterator out_iter;
        output.BeginWriting(out_iter);

        
        
        const char defaultChar = '_';

        char *result = out_iter.get();

        ::WideCharToMultiByte(CP_ACP, 0, buf, inputLen, result, resultLen,
                              &defaultChar, NULL);
    }
    return NS_OK;
}


NS_COM PRInt32 
NS_ConvertAtoW(const char *aStrInA, int aBufferSize, PRUnichar *aStrOutW)
{
    return MultiByteToWideChar(CP_ACP, 0, aStrInA, -1, aStrOutW, aBufferSize);
}

NS_COM PRInt32 
NS_ConvertWtoA(const PRUnichar *aStrInW, int aBufferSizeOut,
               char *aStrOutA, const char *aDefault)
{
    if ((!aStrInW) || (!aStrOutA) || (aBufferSizeOut <= 0))
        return 0;

    int numCharsConverted = WideCharToMultiByte(CP_ACP, 0, aStrInW, -1, 
                                                aStrOutA, aBufferSizeOut,
                                                aDefault, NULL);

    if (!numCharsConverted) {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            
            aStrOutA[aBufferSizeOut-1] = '\0';
        }
        else {
            
            aStrOutA[0] = '\0';
        }
    }
    else if (numCharsConverted < aBufferSizeOut) {
        
        aStrOutA[numCharsConverted] = '\0';
    }

    return numCharsConverted;
}




#elif defined(XP_OS2)

#define INCL_DOS
#include <os2.h>
#include <uconv.h>
#include "nsAString.h"
#include "nsReadableUtils.h"
#include <ulserrno.h>
#include "nsNativeCharsetUtils.h"

static UconvObject UnicodeConverter = NULL;

NS_COM nsresult
NS_CopyNativeToUnicode(const nsACString &input, nsAString  &output)
{
    PRUint32 inputLen = input.Length();

    nsACString::const_iterator iter;
    input.BeginReading(iter);
    const char *inputStr = iter.get();

    
    PRUint32 resultLen = inputLen;
    if (!EnsureStringLength(output, resultLen))
        return NS_ERROR_OUT_OF_MEMORY;

    nsAString::iterator out_iter;
    output.BeginWriting(out_iter);
    UniChar *result = (UniChar*)out_iter.get();

    size_t cSubs = 0;
    size_t resultLeft = resultLen;

    if (!UnicodeConverter)
      NS_StartupNativeCharsetUtils();

    int unirc = ::UniUconvToUcs(UnicodeConverter, (void**)&inputStr, &inputLen,
                                &result, &resultLeft, &cSubs);

    NS_ASSERTION(unirc != UCONV_E2BIG, "Path too big");

    if (unirc != ULS_SUCCESS) {
        output.Truncate();
        return NS_ERROR_FAILURE;
    }

    
    
    output.Truncate(resultLen - resultLeft);
    return NS_OK;
}

NS_COM nsresult
NS_CopyUnicodeToNative(const nsAString &input, nsACString &output)
{
    size_t inputLen = input.Length();

    nsAString::const_iterator iter;
    input.BeginReading(iter);
    UniChar* inputStr = (UniChar*) NS_CONST_CAST(PRUnichar*, iter.get());

    
    
    size_t resultLen = inputLen * 2;
    if (!EnsureStringLength(output, resultLen))
        return NS_ERROR_OUT_OF_MEMORY;

    nsACString::iterator out_iter;
    output.BeginWriting(out_iter);
    char *result = out_iter.get();

    size_t cSubs = 0;
    size_t resultLeft = resultLen;

    if (!UnicodeConverter)
      NS_StartupNativeCharsetUtils();
  
    int unirc = ::UniUconvFromUcs(UnicodeConverter, &inputStr, &inputLen,
                                  (void**)&result, &resultLeft, &cSubs);

    NS_ASSERTION(unirc != UCONV_E2BIG, "Path too big");
  
    if (unirc != ULS_SUCCESS) {
        output.Truncate();
        return NS_ERROR_FAILURE;
    }

    
    
    output.Truncate(resultLen - resultLeft);
    return NS_OK;
}

void
NS_StartupNativeCharsetUtils()
{
    ULONG ulLength;
    ULONG ulCodePage;
    DosQueryCp(sizeof(ULONG), &ulCodePage, &ulLength);

    UniChar codepage[20];
    int unirc = ::UniMapCpToUcsCp(ulCodePage, codepage, 20);
    if (unirc == ULS_SUCCESS) {
        unirc = ::UniCreateUconvObject(codepage, &UnicodeConverter);
        if (unirc == ULS_SUCCESS) {
            uconv_attribute_t attr;
            ::UniQueryUconvObject(UnicodeConverter, &attr, sizeof(uconv_attribute_t), 
                                  NULL, NULL, NULL);
            attr.options = UCONV_OPTION_SUBSTITUTE_BOTH;
            attr.subchar_len=1;
            attr.subchar[0]='_';
            ::UniSetUconvObject(UnicodeConverter, &attr);
        }
    }
}

void
NS_ShutdownNativeCharsetUtils()
{
    ::UniFreeUconvObject(UnicodeConverter);
}

#else

#include "nsReadableUtils.h"

NS_COM nsresult
NS_CopyNativeToUnicode(const nsACString &input, nsAString  &output)
{
    CopyASCIItoUTF16(input, output);
    return NS_OK;
}

NS_COM nsresult
NS_CopyUnicodeToNative(const nsAString  &input, nsACString &output)
{
    LossyCopyUTF16toASCII(input, output);
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

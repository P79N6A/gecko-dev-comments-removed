





#include "js/CharacterEncoding.h"

#include "mozilla/Range.h"

#include "jscntxt.h"
#include "jsprf.h"

using namespace js;

Latin1CharsZ
JS::LossyTwoByteCharsToNewLatin1CharsZ(js::ExclusiveContext* cx,
                                       const mozilla::Range<const char16_t> tbchars)
{
    MOZ_ASSERT(cx);
    size_t len = tbchars.length();
    unsigned char* latin1 = cx->pod_malloc<unsigned char>(len + 1);
    if (!latin1)
        return Latin1CharsZ();
    for (size_t i = 0; i < len; ++i)
        latin1[i] = static_cast<unsigned char>(tbchars[i]);
    latin1[len] = '\0';
    return Latin1CharsZ(latin1, len);
}

template <typename CharT>
static size_t
GetDeflatedUTF8StringLength(const CharT* chars, size_t nchars)
{
    size_t nbytes = nchars;
    for (const CharT* end = chars + nchars; chars < end; chars++) {
        char16_t c = *chars;
        if (c < 0x80)
            continue;
        uint32_t v;
        if (0xD800 <= c && c <= 0xDFFF) {
            
            if (c >= 0xDC00 || (chars + 1) == end) {
                nbytes += 2; 
                continue;
            }
            char16_t c2 = chars[1];
            if (c2 < 0xDC00 || c2 > 0xDFFF) {
                nbytes += 2; 
                continue;
            }
            v = ((c - 0xD800) << 10) + (c2 - 0xDC00) + 0x10000;
            nbytes--;
            chars++;
        } else {
            v = c;
        }
        v >>= 11;
        nbytes++;
        while (v) {
            v >>= 5;
            nbytes++;
        }
    }
    return nbytes;
}

JS_PUBLIC_API(size_t)
JS::GetDeflatedUTF8StringLength(JSFlatString* s)
{
    JS::AutoCheckCannotGC nogc;
    return s->hasLatin1Chars()
           ? ::GetDeflatedUTF8StringLength(s->latin1Chars(nogc), s->length())
           : ::GetDeflatedUTF8StringLength(s->twoByteChars(nogc), s->length());
}

static void
PutUTF8ReplacementCharacter(mozilla::RangedPtr<char>& dst)
{
    *dst++ = char(0xEF);
    *dst++ = char(0xBF);
    *dst++ = char(0xBD);
}

template <typename CharT>
static void
DeflateStringToUTF8Buffer(const CharT* src, size_t srclen, mozilla::RangedPtr<char> dst)
{
    while (srclen) {
        uint32_t v;
        char16_t c = *src++;
        srclen--;
        if (c >= 0xDC00 && c <= 0xDFFF) {
            PutUTF8ReplacementCharacter(dst);
            continue;
        } else if (c < 0xD800 || c > 0xDBFF) {
            v = c;
        } else {
            if (srclen < 1) {
                PutUTF8ReplacementCharacter(dst);
                continue;
            }
            char16_t c2 = *src;
            if ((c2 < 0xDC00) || (c2 > 0xDFFF)) {
                PutUTF8ReplacementCharacter(dst);
                continue;
            }
            src++;
            srclen--;
            v = ((c - 0xD800) << 10) + (c2 - 0xDC00) + 0x10000;
        }
        size_t utf8Len;
        if (v < 0x0080) {
            
            *dst++ = char(v);
            utf8Len = 1;
        } else {
            uint8_t utf8buf[4];
            utf8Len = OneUcs4ToUtf8Char(utf8buf, v);
            for (size_t i = 0; i < utf8Len; i++)
                *dst++ = char(utf8buf[i]);
        }
    }
}

JS_PUBLIC_API(void)
JS::DeflateStringToUTF8Buffer(JSFlatString* src, mozilla::RangedPtr<char> dst)
{
    JS::AutoCheckCannotGC nogc;
    return src->hasLatin1Chars()
           ? ::DeflateStringToUTF8Buffer(src->latin1Chars(nogc), src->length(), dst)
           : ::DeflateStringToUTF8Buffer(src->twoByteChars(nogc), src->length(), dst);
}

template <typename CharT>
UTF8CharsZ
JS::CharsToNewUTF8CharsZ(js::ExclusiveContext* maybeCx, const mozilla::Range<const CharT> chars)
{
    
    const CharT* str = chars.start().get();
    size_t len = ::GetDeflatedUTF8StringLength(str, chars.length());

    
    char* utf8;
    if (maybeCx)
        utf8 = maybeCx->pod_malloc<char>(len + 1);
    else
        utf8 = js_pod_malloc<char>(len + 1);
    if (!utf8)
        return UTF8CharsZ();

    
    ::DeflateStringToUTF8Buffer(str, chars.length(), mozilla::RangedPtr<char>(utf8, len));
    utf8[len] = '\0';

    return UTF8CharsZ(utf8, len);
}

template UTF8CharsZ
JS::CharsToNewUTF8CharsZ(js::ExclusiveContext* maybeCx,
                         const mozilla::Range<const Latin1Char> chars);

template UTF8CharsZ
JS::CharsToNewUTF8CharsZ(js::ExclusiveContext* maybeCx,
                         const mozilla::Range<const char16_t> chars);

static const uint32_t INVALID_UTF8 = UINT32_MAX;






uint32_t
JS::Utf8ToOneUcs4Char(const uint8_t* utf8Buffer, int utf8Length)
{
    MOZ_ASSERT(1 <= utf8Length && utf8Length <= 4);

    if (utf8Length == 1) {
        MOZ_ASSERT(!(*utf8Buffer & 0x80));
        return *utf8Buffer;
    }

    
    static const uint32_t minucs4Table[] = { 0x80, 0x800, 0x10000 };

    MOZ_ASSERT((*utf8Buffer & (0x100 - (1 << (7 - utf8Length)))) ==
               (0x100 - (1 << (8 - utf8Length))));
    uint32_t ucs4Char = *utf8Buffer++ & ((1 << (7 - utf8Length)) - 1);
    uint32_t minucs4Char = minucs4Table[utf8Length - 2];
    while (--utf8Length) {
        MOZ_ASSERT((*utf8Buffer & 0xC0) == 0x80);
        ucs4Char = (ucs4Char << 6) | (*utf8Buffer++ & 0x3F);
    }

    if (MOZ_UNLIKELY(ucs4Char < minucs4Char || (ucs4Char >= 0xD800 && ucs4Char <= 0xDFFF)))
        return INVALID_UTF8;

    return ucs4Char;
}

static void
ReportInvalidCharacter(JSContext* cx, uint32_t offset)
{
    char buffer[10];
    JS_snprintf(buffer, 10, "%d", offset);
    JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, GetErrorMessage, nullptr,
                                 JSMSG_MALFORMED_UTF8_CHAR, buffer);
}

static void
ReportBufferTooSmall(JSContext* cx, uint32_t dummy)
{
    JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_BUFFER_TOO_SMALL);
}

static void
ReportTooBigCharacter(JSContext* cx, uint32_t v)
{
    char buffer[10];
    JS_snprintf(buffer, 10, "0x%x", v + 0x10000);
    JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, GetErrorMessage, nullptr,
                                 JSMSG_UTF8_CHAR_TOO_LARGE, buffer);
}

enum InflateUTF8Action {
    CountAndReportInvalids,
    CountAndIgnoreInvalids,
    Copy
};

static const uint32_t REPLACE_UTF8 = 0xFFFD;



template <InflateUTF8Action action>
static bool
InflateUTF8StringToBuffer(JSContext* cx, const UTF8Chars src, char16_t* dst, size_t* dstlenp,
                          bool* isAsciip)
{
    *isAsciip = true;

    
    
    size_t srclen = src.length();
    uint32_t j = 0;
    for (uint32_t i = 0; i < srclen; i++, j++) {
        uint32_t v = uint32_t(src[i]);
        if (!(v & 0x80)) {
            
            if (action == Copy)
                dst[j] = char16_t(v);

        } else {
            
            *isAsciip = false;
            uint32_t n = 1;
            while (v & (0x80 >> n))
                n++;

        #define INVALID(report, arg, n2)                                \
            do {                                                        \
                if (action == CountAndReportInvalids) {                 \
                    report(cx, arg);                                    \
                    return false;                                       \
                } else {                                                \
                    if (action == Copy)                                 \
                        dst[j] = char16_t(REPLACE_UTF8);                \
                    else                                                \
                        MOZ_ASSERT(action == CountAndIgnoreInvalids);   \
                    n = n2;                                             \
                    goto invalidMultiByteCodeUnit;                      \
                }                                                       \
            } while (0)

            
            if (n < 2 || n > 4)
                INVALID(ReportInvalidCharacter, i, 1);

            
            if (i + n > srclen)
                INVALID(ReportBufferTooSmall,  0, 1);

            
            
            if ((v == 0xE0 && ((uint8_t)src[i + 1] & 0xE0) != 0xA0) ||  
                (v == 0xED && ((uint8_t)src[i + 1] & 0xE0) != 0x80) ||  
                (v == 0xF0 && ((uint8_t)src[i + 1] & 0xF0) == 0x80) ||  
                (v == 0xF4 && ((uint8_t)src[i + 1] & 0xF0) != 0x80))    
            {
                INVALID(ReportInvalidCharacter, i, 1);
            }

            
            for (uint32_t m = 1; m < n; m++)
                if ((src[i + m] & 0xC0) != 0x80)
                    INVALID(ReportInvalidCharacter, i, m);

            
            v = JS::Utf8ToOneUcs4Char((uint8_t*)&src[i], n);
            if (v < 0x10000) {
                
                if (action == Copy)
                    dst[j] = char16_t(v);

            } else {
                v -= 0x10000;
                if (v <= 0xFFFFF) {
                    
                    if (action == Copy)
                        dst[j] = char16_t((v >> 10) + 0xD800);
                    j++;
                    if (action == Copy)
                        dst[j] = char16_t((v & 0x3FF) + 0xDC00);

                } else {
                    
                    INVALID(ReportTooBigCharacter, v, 1);
                }
            }

          invalidMultiByteCodeUnit:
            
            
            
            i += n - 1;
        }
    }

    *dstlenp = j;

    return true;
}

typedef bool (*CountAction)(JSContext*, const UTF8Chars, char16_t*, size_t*, bool* isAsciip);

static TwoByteCharsZ
InflateUTF8StringHelper(JSContext* cx, const UTF8Chars src, CountAction countAction, size_t* outlen)
{
    *outlen = 0;

    bool isAscii;
    if (!countAction(cx, src,  nullptr, outlen, &isAscii))
        return TwoByteCharsZ();

    char16_t* dst = cx->pod_malloc<char16_t>(*outlen + 1);  
    if (!dst) {
        ReportOutOfMemory(cx);
        return TwoByteCharsZ();
    }

    if (isAscii) {
        size_t srclen = src.length();
        MOZ_ASSERT(*outlen == srclen);
        for (uint32_t i = 0; i < srclen; i++)
            dst[i] = char16_t(src[i]);

    } else {
        JS_ALWAYS_TRUE(InflateUTF8StringToBuffer<Copy>(cx, src, dst, outlen, &isAscii));
    }

    dst[*outlen] = 0;    

    return TwoByteCharsZ(dst, *outlen);
}

TwoByteCharsZ
JS::UTF8CharsToNewTwoByteCharsZ(JSContext* cx, const UTF8Chars utf8, size_t* outlen)
{
    return InflateUTF8StringHelper(cx, utf8, InflateUTF8StringToBuffer<CountAndReportInvalids>,
                                   outlen);
}

TwoByteCharsZ
JS::LossyUTF8CharsToNewTwoByteCharsZ(JSContext* cx, const UTF8Chars utf8, size_t* outlen)
{
    return InflateUTF8StringHelper(cx, utf8, InflateUTF8StringToBuffer<CountAndIgnoreInvalids>,
                                   outlen);
}


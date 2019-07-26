






#include "jscntxt.h"

#include "js/CharacterEncoding.h"

#include "jscntxtinlines.h"
#include "jsscriptinlines.h"

using namespace JS;

Latin1CharsZ
JS::LossyTwoByteCharsToNewLatin1CharsZ(JSContext *cx, TwoByteChars tbchars)
{
    JS_ASSERT(cx);
    size_t len = tbchars.length();
    unsigned char *latin1 = cx->pod_malloc<unsigned char>(len + 1);
    if (!latin1)
        return Latin1CharsZ();
    for (size_t i = 0; i < len; ++i)
        latin1[i] = static_cast<unsigned char>(tbchars[i]);
    latin1[len] = '\0';
    return Latin1CharsZ(latin1, len);
}

static size_t
GetDeflatedUTF8StringLength(JSContext *cx, const jschar *chars,
                            size_t nchars)
{
    size_t nbytes;
    const jschar *end;
    unsigned c, c2;

    nbytes = nchars;
    for (end = chars + nchars; chars != end; chars++) {
        c = *chars;
        if (c < 0x80)
            continue;
        if (0xD800 <= c && c <= 0xDFFF) {
            
            if (c >= 0xDC00 || (chars + 1) == end) {
                nbytes += 2; 
                continue;
            }
            c2 = chars[1];
            if (c2 < 0xDC00 || c2 > 0xDFFF) {
                nbytes += 2; 
                continue;
            }
            c = ((c - 0xD800) << 10) + (c2 - 0xDC00) + 0x10000;
            nbytes--;
            chars++;
        }
        c >>= 11;
        nbytes++;
        while (c) {
            c >>= 5;
            nbytes++;
        }
    }
    return nbytes;
}

static bool
PutUTF8ReplacementCharacter(char **dst, size_t *dstlenp) {
    if (*dstlenp < 3)
        return false;
    *(*dst)++ = (char) 0xEF;
    *(*dst)++ = (char) 0xBF;
    *(*dst)++ = (char) 0xBD;
    *dstlenp -= 3;
    return true;
}





static bool
DeflateStringToUTF8Buffer(JSContext *cx, const jschar *src, size_t srclen,
                          char *dst, size_t *dstlenp)
{
    size_t dstlen = *dstlenp;
    size_t origDstlen = dstlen;

    while (srclen) {
        uint32_t v;
        jschar c = *src++;
        srclen--;
        if (c >= 0xDC00 && c <= 0xDFFF) {
            if (!PutUTF8ReplacementCharacter(&dst, &dstlen))
                goto bufferTooSmall;
            continue;
        } else if (c < 0xD800 || c > 0xDBFF) {
            v = c;
        } else {
            if (srclen < 1) {
                if (!PutUTF8ReplacementCharacter(&dst, &dstlen))
                    goto bufferTooSmall;
                continue;
            }
            jschar c2 = *src;
            if ((c2 < 0xDC00) || (c2 > 0xDFFF)) {
                if (!PutUTF8ReplacementCharacter(&dst, &dstlen))
                    goto bufferTooSmall;
                continue;
            }
            src++;
            srclen--;
            v = ((c - 0xD800) << 10) + (c2 - 0xDC00) + 0x10000;
        }
        size_t utf8Len;
        if (v < 0x0080) {
            
            if (dstlen == 0)
                goto bufferTooSmall;
            *dst++ = (char) v;
            utf8Len = 1;
        } else {
            uint8_t utf8buf[4];
            utf8Len = js_OneUcs4ToUtf8Char(utf8buf, v);
            if (utf8Len > dstlen)
                goto bufferTooSmall;
            for (size_t i = 0; i < utf8Len; i++)
                *dst++ = (char) utf8buf[i];
        }
        dstlen -= utf8Len;
    }
    *dstlenp = (origDstlen - dstlen);
    return true;

bufferTooSmall:
    *dstlenp = (origDstlen - dstlen);
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BUFFER_TOO_SMALL);
    return false;
}


UTF8CharsZ
JS::TwoByteCharsToNewUTF8CharsZ(JSContext *cx, TwoByteChars tbchars)
{
    JS_ASSERT(cx);

    
    jschar *str = tbchars.start().get();
    size_t len = GetDeflatedUTF8StringLength(cx, str, tbchars.length());

    
    unsigned char *utf8 = cx->pod_malloc<unsigned char>(len + 1);

    
    DeflateStringToUTF8Buffer(cx, str, tbchars.length(), (char *)utf8, &len);
    utf8[len] = '\0';

    return UTF8CharsZ(utf8, len);
}

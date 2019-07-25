






































#ifndef jsstr_h___
#define jsstr_h___

#include <ctype.h>
#include "jsapi.h"
#include "jsprvtd.h"
#include "jshashtable.h"
#include "jslock.h"
#include "jsobj.h"
#include "jsvalue.h"
#include "jscell.h"

namespace js {


class StringBuffer;








class StringSegmentRange;
class MutatingRopeSegmentRange;




class RopeBuilder;

}  

extern JSString * JS_FASTCALL
js_ConcatStrings(JSContext *cx, JSString *s1, JSString *s2);

extern JSString * JS_FASTCALL
js_toLowerCase(JSContext *cx, JSString *str);

extern JSString * JS_FASTCALL
js_toUpperCase(JSContext *cx, JSString *str);

struct JSSubString {
    size_t          length;
    const jschar    *chars;
};

extern jschar      js_empty_ucstr[];
extern JSSubString js_EmptySubString;


extern const uint8 js_X[];
extern const uint8 js_Y[];
extern const uint32 js_A[];


typedef enum JSCharType {
    JSCT_UNASSIGNED             = 0,
    JSCT_UPPERCASE_LETTER       = 1,
    JSCT_LOWERCASE_LETTER       = 2,
    JSCT_TITLECASE_LETTER       = 3,
    JSCT_MODIFIER_LETTER        = 4,
    JSCT_OTHER_LETTER           = 5,
    JSCT_NON_SPACING_MARK       = 6,
    JSCT_ENCLOSING_MARK         = 7,
    JSCT_COMBINING_SPACING_MARK = 8,
    JSCT_DECIMAL_DIGIT_NUMBER   = 9,
    JSCT_LETTER_NUMBER          = 10,
    JSCT_OTHER_NUMBER           = 11,
    JSCT_SPACE_SEPARATOR        = 12,
    JSCT_LINE_SEPARATOR         = 13,
    JSCT_PARAGRAPH_SEPARATOR    = 14,
    JSCT_CONTROL                = 15,
    JSCT_FORMAT                 = 16,
    JSCT_PRIVATE_USE            = 18,
    JSCT_SURROGATE              = 19,
    JSCT_DASH_PUNCTUATION       = 20,
    JSCT_START_PUNCTUATION      = 21,
    JSCT_END_PUNCTUATION        = 22,
    JSCT_CONNECTOR_PUNCTUATION  = 23,
    JSCT_OTHER_PUNCTUATION      = 24,
    JSCT_MATH_SYMBOL            = 25,
    JSCT_CURRENCY_SYMBOL        = 26,
    JSCT_MODIFIER_SYMBOL        = 27,
    JSCT_OTHER_SYMBOL           = 28
} JSCharType;


#define JS_CCODE(c)     (js_A[js_Y[(js_X[(uint16)(c)>>6]<<6)|((c)&0x3F)]])
#define JS_CTYPE(c)     (JS_CCODE(c) & 0x1F)

#define JS_ISALPHA(c)   ((((1 << JSCT_UPPERCASE_LETTER) |                     \
                           (1 << JSCT_LOWERCASE_LETTER) |                     \
                           (1 << JSCT_TITLECASE_LETTER) |                     \
                           (1 << JSCT_MODIFIER_LETTER) |                      \
                           (1 << JSCT_OTHER_LETTER))                          \
                          >> JS_CTYPE(c)) & 1)

#define JS_ISALNUM(c)   ((((1 << JSCT_UPPERCASE_LETTER) |                     \
                           (1 << JSCT_LOWERCASE_LETTER) |                     \
                           (1 << JSCT_TITLECASE_LETTER) |                     \
                           (1 << JSCT_MODIFIER_LETTER) |                      \
                           (1 << JSCT_OTHER_LETTER) |                         \
                           (1 << JSCT_DECIMAL_DIGIT_NUMBER))                  \
                          >> JS_CTYPE(c)) & 1)


#define JS_ISLETTER(c)   ((((1 << JSCT_UPPERCASE_LETTER) |                    \
                            (1 << JSCT_LOWERCASE_LETTER) |                    \
                            (1 << JSCT_TITLECASE_LETTER) |                    \
                            (1 << JSCT_MODIFIER_LETTER) |                     \
                            (1 << JSCT_OTHER_LETTER) |                        \
                            (1 << JSCT_LETTER_NUMBER))                        \
                           >> JS_CTYPE(c)) & 1)





#define JS_ISIDPART(c)  ((((1 << JSCT_UPPERCASE_LETTER) |                     \
                           (1 << JSCT_LOWERCASE_LETTER) |                     \
                           (1 << JSCT_TITLECASE_LETTER) |                     \
                           (1 << JSCT_MODIFIER_LETTER) |                      \
                           (1 << JSCT_OTHER_LETTER) |                         \
                           (1 << JSCT_LETTER_NUMBER) |                        \
                           (1 << JSCT_NON_SPACING_MARK) |                     \
                           (1 << JSCT_COMBINING_SPACING_MARK) |               \
                           (1 << JSCT_DECIMAL_DIGIT_NUMBER) |                 \
                           (1 << JSCT_CONNECTOR_PUNCTUATION))                 \
                          >> JS_CTYPE(c)) & 1)


#define JS_ISFORMAT(c) (((1 << JSCT_FORMAT) >> JS_CTYPE(c)) & 1)

extern const bool js_isidstart[];
extern const bool js_isident[];

static inline bool
JS_ISIDSTART(int c)
{
    unsigned w = c;

    return (w < 128) ? js_isidstart[w] : JS_ISLETTER(c);
}

static inline bool
JS_ISIDENT(int c)
{
    unsigned w = c;

    return (w < 128) ? js_isident[w] : JS_ISIDPART(c);
}

#define JS_ISXMLSPACE(c)        ((c) == ' ' || (c) == '\t' || (c) == '\r' ||  \
                                 (c) == '\n')
#define JS_ISXMLNSSTART(c)      ((JS_CCODE(c) & 0x00000100) || (c) == '_')
#define JS_ISXMLNS(c)           ((JS_CCODE(c) & 0x00000080) || (c) == '.' ||  \
                                 (c) == '-' || (c) == '_')
#define JS_ISXMLNAMESTART(c)    (JS_ISXMLNSSTART(c) || (c) == ':')
#define JS_ISXMLNAME(c)         (JS_ISXMLNS(c) || (c) == ':')

#define JS_ISDIGIT(c)   (JS_CTYPE(c) == JSCT_DECIMAL_DIGIT_NUMBER)

const jschar BYTE_ORDER_MARK = 0xFEFF;
const jschar BYTE_ORDER_MARK2 = 0xFFFE;
const jschar NO_BREAK_SPACE  = 0x00A0;

extern const bool js_isspace[];

static inline bool
JS_ISSPACE(int c)
{
    unsigned w = c;

    return (w < 128)
           ? js_isspace[w]
           : w == NO_BREAK_SPACE || w == BYTE_ORDER_MARK ||
             (JS_CCODE(w) & 0x00070000) == 0x00040000;
}

static inline bool
JS_ISSPACE_OR_BOM(int c)
{
    unsigned w = c;

    
    return (w < 128)
           ? js_isspace[w]
           : w == NO_BREAK_SPACE || w == BYTE_ORDER_MARK || w == BYTE_ORDER_MARK2 ||
             (JS_CCODE(w) & 0x00070000) == 0x00040000;
}

#define JS_ISPRINT(c)   ((c) < 128 && isprint(c))

#define JS_ISUPPER(c)   (JS_CTYPE(c) == JSCT_UPPERCASE_LETTER)
#define JS_ISLOWER(c)   (JS_CTYPE(c) == JSCT_LOWERCASE_LETTER)

#define JS_TOUPPER(c)   ((jschar) ((JS_CCODE(c) & 0x00100000)                 \
                                   ? (c) - ((int32)JS_CCODE(c) >> 22)         \
                                   : (c)))
#define JS_TOLOWER(c)   ((jschar) ((JS_CCODE(c) & 0x00200000)                 \
                                   ? (c) + ((int32)JS_CCODE(c) >> 22)         \
                                   : (c)))





#define JS7_ISDEC(c)    ((((unsigned)(c)) - '0') <= 9)
#define JS7_UNDEC(c)    ((c) - '0')
#define JS7_ISHEX(c)    ((c) < 128 && isxdigit(c))
#define JS7_UNHEX(c)    (uintN)(JS7_ISDEC(c) ? (c) - '0' : 10 + tolower(c) - 'a')
#define JS7_ISLET(c)    ((c) < 128 && isalpha(c))


extern js::Class js_StringClass;

inline bool
JSObject::isString() const
{
    return getClass() == &js_StringClass;
}

extern JSObject *
js_InitStringClass(JSContext *cx, JSObject *obj);

extern const char js_escape_str[];
extern const char js_unescape_str[];
extern const char js_uneval_str[];
extern const char js_decodeURI_str[];
extern const char js_encodeURI_str[];
extern const char js_decodeURIComponent_str[];
extern const char js_encodeURIComponent_str[];


extern JSFixedString *
js_NewString(JSContext *cx, jschar *chars, size_t length);

extern JSLinearString *
js_NewDependentString(JSContext *cx, JSString *base, size_t start, size_t length);


extern JSFixedString *
js_NewStringCopyN(JSContext *cx, const jschar *s, size_t n);

extern JSFixedString *
js_NewStringCopyN(JSContext *cx, const char *s, size_t n);


extern JSFixedString *
js_NewStringCopyZ(JSContext *cx, const jschar *s);

extern JSFixedString *
js_NewStringCopyZ(JSContext *cx, const char *s);




extern const char *
js_ValueToPrintable(JSContext *cx, const js::Value &,
                    JSAutoByteString *bytes, bool asSource = false);





extern JSString *
js_ValueToString(JSContext *cx, const js::Value &v);

namespace js {






static JS_ALWAYS_INLINE JSString *
ValueToString_TestForStringInline(JSContext *cx, const Value &v)
{
    if (v.isString())
        return v.toString();
    return js_ValueToString(cx, v);
}






inline bool
ValueToStringBuffer(JSContext *cx, const Value &v, StringBuffer &sb);

} 





extern JS_FRIEND_API(JSString *)
js_ValueToSource(JSContext *cx, const js::Value &v);

namespace js {





inline uint32
HashChars(const jschar *chars, size_t length)
{
    uint32 h = 0;
    for (; length; chars++, length--)
        h = JS_ROTATE_LEFT32(h, 4) ^ *chars;
    return h;
}





extern bool
EqualStrings(JSContext *cx, JSString *str1, JSString *str2, JSBool *result);


extern bool
EqualStrings(JSLinearString *str1, JSLinearString *str2);





extern bool
CompareStrings(JSContext *cx, JSString *str1, JSString *str2, int32 *result);




extern bool
StringEqualsAscii(JSLinearString *str, const char *asciiBytes);

} 

extern size_t
js_strlen(const jschar *s);

extern jschar *
js_strchr(const jschar *s, jschar c);

extern jschar *
js_strchr_limit(const jschar *s, jschar c, const jschar *limit);

#define js_strncpy(t, s, n)     memcpy((t), (s), (n) * sizeof(jschar))




static inline const jschar *
js_SkipWhiteSpace(const jschar *s, const jschar *end)
{
    JS_ASSERT(s <= end);
    while (s != end && JS_ISSPACE(*s))
        s++;
    return s;
}

namespace js {


















enum FlationCoding
{
    NormalEncoding,
    CESU8Encoding
};






extern jschar *
InflateString(JSContext *cx, const char *bytes, size_t *length,
              FlationCoding fc = NormalEncoding);

extern char *
DeflateString(JSContext *cx, const jschar *chars, size_t length);








extern bool
InflateStringToBuffer(JSContext *cx, const char *bytes, size_t length,
                      jschar *chars, size_t *charsLength);

extern bool
InflateUTF8StringToBuffer(JSContext *cx, const char *bytes, size_t length,
                          jschar *chars, size_t *charsLength,
                          FlationCoding fc = NormalEncoding);


extern size_t
GetDeflatedStringLength(JSContext *cx, const jschar *chars, size_t charsLength);


extern size_t
GetDeflatedUTF8StringLength(JSContext *cx, const jschar *chars,
                            size_t charsLength,
                            FlationCoding fc = NormalEncoding);







extern bool
DeflateStringToBuffer(JSContext *cx, const jschar *chars,
                      size_t charsLength, char *bytes, size_t *length);




extern bool
DeflateStringToUTF8Buffer(JSContext *cx, const jschar *chars,
                          size_t charsLength, char *bytes, size_t *length,
                          FlationCoding fc = NormalEncoding);

} 


extern JSBool
js_str_escape(JSContext *cx, uintN argc, js::Value *argv, js::Value *rval);





namespace js {
extern JSBool
str_replace(JSContext *cx, uintN argc, js::Value *vp);
}

extern JSBool
js_str_toString(JSContext *cx, uintN argc, js::Value *vp);

extern JSBool
js_str_charAt(JSContext *cx, uintN argc, js::Value *vp);

extern JSBool
js_str_charCodeAt(JSContext *cx, uintN argc, js::Value *vp);





extern int
js_OneUcs4ToUtf8Char(uint8 *utf8Buffer, uint32 ucs4Char);

namespace js {

extern size_t
PutEscapedStringImpl(char *buffer, size_t size, FILE *fp, JSLinearString *str, uint32 quote);










inline size_t
PutEscapedString(char *buffer, size_t size, JSLinearString *str, uint32 quote)
{
    size_t n = PutEscapedStringImpl(buffer, size, NULL, str, quote);

    
    JS_ASSERT(n != size_t(-1));
    return n;
}






inline bool
FileEscapedString(FILE *fp, JSLinearString *str, uint32 quote)
{
    return PutEscapedStringImpl(NULL, 0, fp, str, quote) != size_t(-1);
}

} 

extern JSBool
js_String(JSContext *cx, uintN argc, js::Value *vp);

#endif 

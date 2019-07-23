






































#ifndef jsstr_h___
#define jsstr_h___









#include <ctype.h>
#include "jspubtd.h"
#include "jsprvtd.h"

JS_BEGIN_EXTERN_C




































struct JSString {
    size_t          length;
    union {
        jschar      *chars;
        JSString    *base;
    } u;
};








#define JSSTRFLAG_DEPENDENT         JSSTRING_BIT(JS_BITS_PER_WORD - 1)
#define JSSTRFLAG_PREFIX            JSSTRING_BIT(JS_BITS_PER_WORD - 2)
#define JSSTRFLAG_MUTABLE           JSSTRFLAG_PREFIX
#define JSSTRFLAG_ATOMIZED          JSSTRING_BIT(JS_BITS_PER_WORD - 3)

#define JSSTRING_LENGTH_BITS        (JS_BITS_PER_WORD - 3)
#define JSSTRING_LENGTH_MASK        JSSTRING_BITMASK(JSSTRING_LENGTH_BITS)


#define JSSTRING_BIT(n)             ((size_t)1 << (n))
#define JSSTRING_BITMASK(n)         (JSSTRING_BIT(n) - 1)
#define JSSTRING_HAS_FLAG(str,flg)  ((str)->length & (flg))
#define JSSTRING_IS_DEPENDENT(str)  JSSTRING_HAS_FLAG(str, JSSTRFLAG_DEPENDENT)
#define JSSTRING_IS_FLAT(str)       (!JSSTRING_IS_DEPENDENT(str))
#define JSSTRING_IS_MUTABLE(str)    (((str)->length & (JSSTRFLAG_DEPENDENT |  \
                                                       JSSTRFLAG_MUTABLE)) == \
                                     JSSTRFLAG_MUTABLE)
#define JSSTRING_IS_ATOMIZED(str)   (((str)->length & (JSSTRFLAG_DEPENDENT |  \
                                                       JSSTRFLAG_ATOMIZED)) ==\
                                     JSSTRFLAG_ATOMIZED)

#define JSSTRING_CHARS(str)         (JSSTRING_IS_DEPENDENT(str)               \
                                     ? JSSTRDEP_CHARS(str)                    \
                                     : JSFLATSTR_CHARS(str))
#define JSSTRING_LENGTH(str)        (JSSTRING_IS_DEPENDENT(str)               \
                                     ? JSSTRDEP_LENGTH(str)                   \
                                     : JSFLATSTR_LENGTH(str))

#define JSSTRING_CHARS_AND_LENGTH(str, chars_, length_)                       \
    ((void)(JSSTRING_IS_DEPENDENT(str)                                        \
            ? ((length_) = JSSTRDEP_LENGTH(str),                              \
               (chars_) = JSSTRDEP_CHARS(str))                                \
            : ((length_) = JSFLATSTR_LENGTH(str),                             \
               (chars_) = JSFLATSTR_CHARS(str))))

#define JSSTRING_CHARS_AND_END(str, chars_, end)                              \
    ((void)((end) = JSSTRING_IS_DEPENDENT(str)                                \
                  ? JSSTRDEP_LENGTH(str) + ((chars_) = JSSTRDEP_CHARS(str))   \
                  : JSFLATSTR_LENGTH(str) + ((chars_) = JSFLATSTR_CHARS(str))))


#define JSFLATSTR_INIT(str, chars_, length_)                                  \
    ((void)(JS_ASSERT(((length_) & ~JSSTRING_LENGTH_MASK) == 0),              \
            (str)->length = (length_), (str)->u.chars = (chars_)))

#define JSFLATSTR_LENGTH(str)                                                 \
    (JS_ASSERT(JSSTRING_IS_FLAT(str)), (str)->length & JSSTRING_LENGTH_MASK)

#define JSFLATSTR_CHARS(str)                                                  \
    (JS_ASSERT(JSSTRING_IS_FLAT(str)), (str)->u.chars)



























#define JSFLATSTR_SET_ATOMIZED(str)                                           \
    ((void)(JS_ASSERT(JSSTRING_IS_FLAT(str) && !JSSTRING_IS_MUTABLE(str)),    \
            (str)->length |= JSSTRFLAG_ATOMIZED))

#define JSFLATSTR_SET_MUTABLE(str)                                            \
    ((void)(JS_ASSERT(JSSTRING_IS_FLAT(str) && !JSSTRING_IS_ATOMIZED(str)),   \
            (str)->length |= JSSTRFLAG_MUTABLE))

#define JSFLATSTR_CLEAR_MUTABLE(str)                                          \
    ((void)(JS_ASSERT(JSSTRING_IS_FLAT(str)),                                 \
            JSSTRING_HAS_FLAG(str, JSSTRFLAG_MUTABLE) &&                      \
            ((str)->length &= ~JSSTRFLAG_MUTABLE)))


#define JSSTRDEP_START_BITS         (JSSTRING_LENGTH_BITS-JSSTRDEP_LENGTH_BITS)
#define JSSTRDEP_START_SHIFT        JSSTRDEP_LENGTH_BITS
#define JSSTRDEP_START_MASK         JSSTRING_BITMASK(JSSTRDEP_START_BITS)
#define JSSTRDEP_LENGTH_BITS        (JSSTRING_LENGTH_BITS / 2)
#define JSSTRDEP_LENGTH_MASK        JSSTRING_BITMASK(JSSTRDEP_LENGTH_BITS)

#define JSSTRDEP_IS_PREFIX(str)     JSSTRING_HAS_FLAG(str, JSSTRFLAG_PREFIX)

#define JSSTRDEP_START(str)         (JSSTRDEP_IS_PREFIX(str) ? 0              \
                                     : (((str)->length                        \
                                         >> JSSTRDEP_START_SHIFT)             \
                                        & JSSTRDEP_START_MASK))
#define JSSTRDEP_LENGTH(str)        ((str)->length                            \
                                     & (JSSTRDEP_IS_PREFIX(str)               \
                                        ? JSSTRING_LENGTH_MASK                \
                                        : JSSTRDEP_LENGTH_MASK))

#define JSSTRDEP_INIT(str,bstr,off,len)                                       \
    ((str)->length = JSSTRFLAG_DEPENDENT                                      \
                   | ((off) << JSSTRDEP_START_SHIFT)                          \
                   | (len),                                                   \
     (str)->u.base = (bstr))

#define JSPREFIX_INIT(str,bstr,len)                                           \
    ((str)->length = JSSTRFLAG_DEPENDENT | JSSTRFLAG_PREFIX | (len),          \
     (str)->u.base = (bstr))

#define JSSTRDEP_BASE(str)          ((str)->u.base)
#define JSPREFIX_BASE(str)          JSSTRDEP_BASE(str)
#define JSPREFIX_SET_BASE(str,bstr) ((str)->u.base = (bstr))

#define JSSTRDEP_CHARS(str)                                                   \
    (JSSTRING_IS_DEPENDENT(JSSTRDEP_BASE(str))                                \
     ? js_GetDependentStringChars(str)                                        \
     : JSFLATSTR_CHARS(JSSTRDEP_BASE(str)) + JSSTRDEP_START(str))

extern size_t
js_MinimizeDependentStrings(JSString *str, int level, JSString **basep);

extern jschar *
js_GetDependentStringChars(JSString *str);

extern const jschar *
js_GetStringChars(JSContext *cx, JSString *str);

extern JSString *
js_ConcatStrings(JSContext *cx, JSString *left, JSString *right);

extern const jschar *
js_UndependString(JSContext *cx, JSString *str);

extern JSBool
js_MakeStringImmutable(JSContext *cx, JSString *str);

typedef struct JSCharBuffer {
    size_t          length;
    jschar          *chars;
} JSCharBuffer;

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






#define JS_ISWORD(c)    ((c) < 128 && (isalnum(c) || (c) == '_'))

#define JS_ISIDSTART(c) (JS_ISLETTER(c) || (c) == '_' || (c) == '$')
#define JS_ISIDENT(c)   (JS_ISIDPART(c) || (c) == '_' || (c) == '$')

#define JS_ISXMLSPACE(c)        ((c) == ' ' || (c) == '\t' || (c) == '\r' ||  \
                                 (c) == '\n')
#define JS_ISXMLNSSTART(c)      ((JS_CCODE(c) & 0x00000100) || (c) == '_')
#define JS_ISXMLNS(c)           ((JS_CCODE(c) & 0x00000080) || (c) == '.' ||  \
                                 (c) == '-' || (c) == '_')
#define JS_ISXMLNAMESTART(c)    (JS_ISXMLNSSTART(c) || (c) == ':')
#define JS_ISXMLNAME(c)         (JS_ISXMLNS(c) || (c) == ':')

#define JS_ISDIGIT(c)   (JS_CTYPE(c) == JSCT_DECIMAL_DIGIT_NUMBER)



#define JS_ISSPACE(c)   ((JS_CCODE(c) & 0x00070000) == 0x00040000)
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


extern JSBool
js_InitRuntimeStringState(JSContext *cx);

extern JSBool
js_InitDeflatedStringCache(JSRuntime *rt);





#define UNIT_STRING_LIMIT 256U





extern JSString *
js_GetUnitString(JSContext *cx, JSString *str, size_t index);

extern void
js_FinishUnitStrings(JSRuntime *rt);

extern void
js_FinishRuntimeStringState(JSContext *cx);

extern void
js_FinishDeflatedStringCache(JSRuntime *rt);


extern JSClass js_StringClass;

extern JSObject *
js_InitStringClass(JSContext *cx, JSObject *obj);

extern const char js_escape_str[];
extern const char js_unescape_str[];
extern const char js_uneval_str[];
extern const char js_decodeURI_str[];
extern const char js_encodeURI_str[];
extern const char js_decodeURIComponent_str[];
extern const char js_encodeURIComponent_str[];


extern JSString *
js_NewString(JSContext *cx, jschar *chars, size_t length);

extern JSString *
js_NewDependentString(JSContext *cx, JSString *base, size_t start,
                      size_t length);


extern JSString *
js_NewStringCopyN(JSContext *cx, const jschar *s, size_t n);


extern JSString *
js_NewStringCopyZ(JSContext *cx, const jschar *s);








extern void
js_FinalizeStringRT(JSRuntime *rt, JSString *str, intN type, JSContext *cx);




typedef JSString *(*JSValueToStringFun)(JSContext *cx, jsval v);

extern JS_FRIEND_API(const char *)
js_ValueToPrintable(JSContext *cx, jsval v, JSValueToStringFun v2sfun);

#define js_ValueToPrintableString(cx,v) \
    js_ValueToPrintable(cx, v, js_ValueToString)

#define js_ValueToPrintableSource(cx,v) \
    js_ValueToPrintable(cx, v, js_ValueToSource)





extern JS_FRIEND_API(JSString *)
js_ValueToString(JSContext *cx, jsval v);





extern JS_FRIEND_API(JSString *)
js_ValueToSource(JSContext *cx, jsval v);





extern uint32
js_HashString(JSString *str);





extern JSBool
js_EqualStrings(JSString *str1, JSString *str2);





extern intN
js_CompareStrings(JSString *str1, JSString *str2);








#define BMH_CHARSET_SIZE 256    /* ISO-Latin-1 */
#define BMH_PATLEN_MAX   255    /* skip table element is uint8 */

#define BMH_BAD_PATTERN  (-2)   /* return value if pat is not ISO-Latin-1 */

extern jsint
js_BoyerMooreHorspool(const jschar *text, jsint textlen,
                      const jschar *pat, jsint patlen,
                      jsint start);

extern size_t
js_strlen(const jschar *s);

extern jschar *
js_strchr(const jschar *s, jschar c);

extern jschar *
js_strchr_limit(const jschar *s, jschar c, const jschar *limit);

#define js_strncpy(t, s, n)     memcpy((t), (s), (n) * sizeof(jschar))




extern const jschar *
js_SkipWhiteSpace(const jschar *s, const jschar *end);






extern jschar *
js_InflateString(JSContext *cx, const char *bytes, size_t *length);

extern char *
js_DeflateString(JSContext *cx, const jschar *chars, size_t length);







extern JSBool
js_InflateStringToBuffer(JSContext* cx, const char *bytes, size_t length,
                         jschar *chars, size_t* charsLength);




extern size_t
js_GetDeflatedStringLength(JSContext *cx, const jschar *chars,
                           size_t charsLength);







extern JSBool
js_DeflateStringToBuffer(JSContext* cx, const jschar *chars,
                         size_t charsLength, char *bytes, size_t* length);





extern JSBool
js_SetStringBytes(JSContext *cx, JSString *str, char *bytes, size_t length);





extern const char *
js_GetStringBytes(JSContext *cx, JSString *str);


extern void
js_PurgeDeflatedStringCache(JSRuntime *rt, JSString *str);

JSBool
js_str_escape(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
              jsval *rval);





extern int
js_OneUcs4ToUtf8Char(uint8 *utf8Buffer, uint32 ucs4Char);










#define js_PutEscapedString(buffer, bufferSize, str, quote)                   \
    js_PutEscapedStringImpl(buffer, bufferSize, NULL, str, quote)








#define js_FileEscapedString(file, str, quote)                                \
    (JS_ASSERT(file), js_PutEscapedStringImpl(NULL, 0, file, str, quote))

extern JS_FRIEND_API(size_t)
js_PutEscapedStringImpl(char *buffer, size_t bufferSize, FILE *fp,
                        JSString *str, uint32 quote);

JS_END_EXTERN_C

#endif 

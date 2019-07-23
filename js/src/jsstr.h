






































#ifndef jsstr_h___
#define jsstr_h___









#include <ctype.h>
#include "jspubtd.h"
#include "jsprvtd.h"
#include "jslock.h"

JS_BEGIN_EXTERN_C





#define UNIT_STRING_LIMIT 256U

#define JSSTRING_BIT(n)             ((size_t)1 << (n))
#define JSSTRING_BITMASK(n)         (JSSTRING_BIT(n) - 1)

#define UNIT_STRING_SPACE(sp)    ((jschar *) ((sp) + UNIT_STRING_LIMIT))
#define UNIT_STRING_SPACE_RT(rt) UNIT_STRING_SPACE((rt)->unitStrings)

#define IN_UNIT_STRING_SPACE(sp,cp)                                           \
    ((size_t)((cp) - UNIT_STRING_SPACE(sp)) < 2 * UNIT_STRING_LIMIT)
#define IN_UNIT_STRING_SPACE_RT(rt,cp)                                        \
    IN_UNIT_STRING_SPACE((rt)->unitStrings, cp)

class TraceRecorder;

extern jschar *
js_GetDependentStringChars(JSString *str);





extern JSString *
js_MakeUnitString(JSContext *cx, jschar c);








































struct JSString {
    friend class TraceRecorder;

    friend JSAtom *
    js_AtomizeString(JSContext *cx, JSString *str, uintN flags);

    friend JSString * JS_FASTCALL
    js_ConcatStrings(JSContext *cx, JSString *left, JSString *right);

private:
    size_t          mLength;
    union {
        jschar      *mChars;
        JSString    *mBase;
    };

    








    enum
#if defined(_MSC_VER) && defined(_WIN64)
    : size_t 
#endif
    {
        DEPENDENT =     JSSTRING_BIT(JS_BITS_PER_WORD - 1),
        PREFIX =        JSSTRING_BIT(JS_BITS_PER_WORD - 2),
        MUTABLE =       PREFIX,
        ATOMIZED =      JSSTRING_BIT(JS_BITS_PER_WORD - 3),
        DEFLATED =      JSSTRING_BIT(JS_BITS_PER_WORD - 4),

        LENGTH_BITS =   JS_BITS_PER_WORD - 4,
        LENGTH_MASK =   JSSTRING_BITMASK(LENGTH_BITS),

        




        DEPENDENT_LENGTH_BITS = size_t(LENGTH_BITS) / 2,
        DEPENDENT_LENGTH_MASK = JSSTRING_BITMASK(DEPENDENT_LENGTH_BITS),
        DEPENDENT_START_BITS =  LENGTH_BITS - DEPENDENT_LENGTH_BITS,
        DEPENDENT_START_SHIFT = DEPENDENT_LENGTH_BITS,
        DEPENDENT_START_MASK =  JSSTRING_BITMASK(DEPENDENT_START_BITS)
    };

    bool hasFlag(size_t flag) const {
        return (mLength & flag) != 0;
    }

public:
    enum
#if defined(_MSC_VER) && defined(_WIN64)
    : size_t 
#endif
    {
        MAX_LENGTH = LENGTH_MASK,
        MAX_DEPENDENT_START = DEPENDENT_START_MASK,
        MAX_DEPENDENT_LENGTH = DEPENDENT_LENGTH_MASK
    };

    bool isDependent() const {
        return hasFlag(DEPENDENT);
    }

    bool isFlat() const {
        return !isDependent();
    }

    bool isDeflated() const {
        return hasFlag(DEFLATED);
    }

    void setDeflated() {
        JS_ATOMIC_SET_MASK((jsword *) &mLength, DEFLATED);
    }

    bool isMutable() const {
        return !isDependent() && hasFlag(MUTABLE);
    }

    bool isAtomized() const {
        return !isDependent() && hasFlag(ATOMIZED);
    }

    JS_ALWAYS_INLINE jschar *chars() {
        return isDependent() ? dependentChars() : flatChars();
    }

    JS_ALWAYS_INLINE size_t length() const {
        return isDependent() ? dependentLength() : flatLength();
    }

    JS_ALWAYS_INLINE bool empty() const {
        return length() == 0;
    }

    JS_ALWAYS_INLINE void getCharsAndLength(const jschar *&chars, size_t &length) {
        if (isDependent()) {
            length = dependentLength();
            chars = dependentChars();
        } else {
            length = flatLength();
            chars = flatChars();
        }
    }

    JS_ALWAYS_INLINE void getCharsAndEnd(const jschar *&chars, const jschar *&end) {
        end = isDependent()
              ? dependentLength() + (chars = dependentChars())
              : flatLength() + (chars = flatChars());
    }

    
    void initFlat(jschar *chars, size_t length) {
        JS_ASSERT(length <= MAX_LENGTH);
        mLength = length;
        mChars = chars;
    }

    jschar *flatChars() const {
        JS_ASSERT(isFlat());
        return mChars;
    }

    size_t flatLength() const {
        JS_ASSERT(isFlat());
        return mLength & LENGTH_MASK;
    }

    




    void reinitFlat(jschar *chars, size_t length) {
        JS_ASSERT(length <= MAX_LENGTH);
        mLength = (mLength & DEFLATED) | (length & ~DEFLATED);
        mChars = chars;
    }

    


























    void flatSetAtomized() {
        JS_ASSERT(isFlat() && !isMutable());
        JS_STATIC_ASSERT(sizeof(mLength) == sizeof(jsword));
        JS_ATOMIC_SET_MASK((jsword *) &mLength, ATOMIZED);
    }

    void flatSetMutable() {
        JS_ASSERT(isFlat() && !isAtomized());
        mLength |= MUTABLE;
    }

    void flatClearMutable() {
        JS_ASSERT(isFlat());
        if (hasFlag(MUTABLE))
            mLength &= ~MUTABLE;
    }

    void initDependent(JSString *bstr, size_t off, size_t len) {
        JS_ASSERT(off <= MAX_DEPENDENT_START);
        JS_ASSERT(len <= MAX_DEPENDENT_LENGTH);
        mLength = DEPENDENT | (off << DEPENDENT_START_SHIFT) | len;
        mBase = bstr;
    }

    
    void reinitDependent(JSString *bstr, size_t off, size_t len) {
        JS_ASSERT(off <= MAX_DEPENDENT_START);
        JS_ASSERT(len <= MAX_DEPENDENT_LENGTH);
        mLength = DEPENDENT | (mLength & DEFLATED) | (off << DEPENDENT_START_SHIFT) | len;
        mBase = bstr;
    }

    JSString *dependentBase() const {
        JS_ASSERT(isDependent());
        return mBase;
    }

    bool dependentIsPrefix() const {
        JS_ASSERT(isDependent());
        return hasFlag(PREFIX);
    }

    JS_ALWAYS_INLINE jschar *dependentChars() {
        return dependentBase()->isDependent()
               ? js_GetDependentStringChars(this)
               : dependentBase()->flatChars() + dependentStart();
    }

    JS_ALWAYS_INLINE size_t dependentStart() const {
        return dependentIsPrefix()
               ? 0
               : ((mLength >> DEPENDENT_START_SHIFT) & DEPENDENT_START_MASK);
    }

    JS_ALWAYS_INLINE size_t dependentLength() const {
        JS_ASSERT(isDependent());
        return mLength & (dependentIsPrefix() ? LENGTH_MASK : DEPENDENT_LENGTH_MASK);
    }

    void initPrefix(JSString *bstr, size_t len) {
        JS_ASSERT(len <= MAX_LENGTH);
        mLength = DEPENDENT | PREFIX | len;
        mBase = bstr;
    }

    
    void reinitPrefix(JSString *bstr, size_t len) {
        JS_ASSERT(len <= MAX_LENGTH);
        mLength = DEPENDENT | PREFIX | (mLength & DEFLATED) | len;
        mBase = bstr;
    }

    JSString *prefixBase() const {
        JS_ASSERT(isDependent() && dependentIsPrefix());
        return dependentBase();
    }

    void prefixSetBase(JSString *bstr) {
        JS_ASSERT(isDependent() && dependentIsPrefix());
        mBase = bstr;
    }

    static JSString *getUnitString(JSContext *cx, jschar c);
    static JSString *getUnitString(JSContext *cx, JSString *str, size_t index);
};

extern const jschar *
js_GetStringChars(JSContext *cx, JSString *str);

extern JSString * JS_FASTCALL
js_ConcatStrings(JSContext *cx, JSString *left, JSString *right);

extern const jschar *
js_UndependString(JSContext *cx, JSString *str);

extern JSBool
js_MakeStringImmutable(JSContext *cx, JSString *str);

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





extern const bool js_alnum[];







#define JS_ISWORD(c)    ((c) < 128 && js_alnum[(c)])

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

static inline bool
JS_ISSPACE(jschar c)
{
    unsigned w = c;

    if (w < 256)
        return (w <= ' ' && (w == ' ' || (9 <= w && w <= 0xD))) || w == 0xA0;

    return (JS_CCODE(w) & 0x00070000) == 0x00040000;
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


extern JSBool
js_InitRuntimeStringState(JSContext *cx);

extern JSBool
js_InitDeflatedStringCache(JSRuntime *rt);

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
js_NewStringFromCharBuffer(JSContext *cx, JSCharBuffer &cb);

extern JSString *
js_NewDependentString(JSContext *cx, JSString *base, size_t start,
                      size_t length);


extern JSString *
js_NewStringCopyN(JSContext *cx, const jschar *s, size_t n);


extern JSString *
js_NewStringCopyZ(JSContext *cx, const jschar *s);




typedef JSString *(*JSValueToStringFun)(JSContext *cx, jsval v);

extern JS_FRIEND_API(const char *)
js_ValueToPrintable(JSContext *cx, jsval v, JSValueToStringFun v2sfun);

#define js_ValueToPrintableString(cx,v) \
    js_ValueToPrintable(cx, v, js_ValueToString)

#define js_ValueToPrintableSource(cx,v) \
    js_ValueToPrintable(cx, v, js_ValueToSource)





extern JS_FRIEND_API(JSString *)
js_ValueToString(JSContext *cx, jsval v);






extern JS_FRIEND_API(JSBool)
js_ValueToCharBuffer(JSContext *cx, jsval v, JSCharBuffer &cb);





extern JS_FRIEND_API(JSString *)
js_ValueToSource(JSContext *cx, jsval v);





extern uint32
js_HashString(JSString *str);





extern JSBool JS_FASTCALL
js_EqualStrings(JSString *str1, JSString *str2);





extern int32 JS_FASTCALL
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




static inline const jschar *
js_SkipWhiteSpace(const jschar *s, const jschar *end)
{
    JS_ASSERT(s <= end);
    while (s != end && JS_ISSPACE(*s))
        s++;
    return s;
}






extern jschar *
js_InflateString(JSContext *cx, const char *bytes, size_t *length);

extern char *
js_DeflateString(JSContext *cx, const jschar *chars, size_t length);







extern JSBool
js_InflateStringToBuffer(JSContext *cx, const char *bytes, size_t length,
                         jschar *chars, size_t *charsLength);




extern size_t
js_GetDeflatedStringLength(JSContext *cx, const jschar *chars,
                           size_t charsLength);







extern JSBool
js_DeflateStringToBuffer(JSContext *cx, const jschar *chars,
                         size_t charsLength, char *bytes, size_t *length);





extern JSBool
js_SetStringBytes(JSContext *cx, JSString *str, char *bytes, size_t length);





extern const char *
js_GetStringBytes(JSContext *cx, JSString *str);


extern void
js_PurgeDeflatedStringCache(JSRuntime *rt, JSString *str);


extern JSBool
js_str_escape(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
              jsval *rval);

extern JSBool
js_str_toString(JSContext *cx, uintN argc, jsval *vp);

extern JSBool
js_StringReplaceHelper(JSContext *cx, uintN argc, JSObject *lambda,
                       JSString *repstr, jsval *vp);





extern int
js_OneUcs4ToUtf8Char(uint8 *utf8Buffer, uint32 ucs4Char);










#define js_PutEscapedString(buffer, bufferSize, str, quote)                   \
    js_PutEscapedStringImpl(buffer, bufferSize, NULL, str, quote)








#define js_FileEscapedString(file, str, quote)                                \
    (JS_ASSERT(file), js_PutEscapedStringImpl(NULL, 0, file, str, quote))

extern JS_FRIEND_API(size_t)
js_PutEscapedStringImpl(char *buffer, size_t bufferSize, FILE *fp,
                        JSString *str, uint32 quote);

extern JSBool
js_String(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JS_END_EXTERN_C

#endif 

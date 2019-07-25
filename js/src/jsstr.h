






































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

enum {
    UNIT_STRING_LIMIT        = 256U,
    SMALL_CHAR_LIMIT         = 128U, 
    NUM_SMALL_CHARS          = 64U,
    INT_STRING_LIMIT         = 256U,
    NUM_HUNDRED_STRINGS      = 156U
};

extern jschar *
js_GetDependentStringChars(JSString *str);

extern JSString * JS_FASTCALL
js_ConcatStrings(JSContext *cx, JSString *left, JSString *right);

JS_STATIC_ASSERT(JS_BITS_PER_WORD >= 32);

struct JSRopeBufferInfo {
    
    size_t capacity;
};


namespace js { namespace mjit {
    class Compiler;
}}

struct JSLinearString;

































struct JSString
{
    friend class js::TraceRecorder;
    friend class js::mjit::Compiler;

    friend JSAtom *js_AtomizeString(JSContext *cx, JSString *str, uintN flags);

    



    size_t                 lengthAndFlags;      
    union {
        const jschar       *chars;              
        JSString           *left;               
    } u;
    union {
        jschar             inlineStorage[4];    
        struct {
            union {
                JSString   *right;              
                JSString   *base;               
                size_t     capacity;            
            };
            union {
                JSString   *parent;             
                size_t     reserved;            
            };
        } s;
        size_t             externalStringType;  
    };

    











    static const size_t TYPE_FLAGS_MASK = JS_BITMASK(4);
    static const size_t LENGTH_SHIFT    = 4;

    static const size_t TYPE_MASK       = JS_BITMASK(2);
    static const size_t FLAT            = 0x0;
    static const size_t DEPENDENT       = 0x1;
    static const size_t ROPE            = 0x2;

    
    static const size_t DEPENDENT_BIT   = JS_BIT(0);
    static const size_t ROPE_BIT        = JS_BIT(1);

    static const size_t ATOMIZED        = JS_BIT(2);
    static const size_t EXTENSIBLE      = JS_BIT(3);


    size_t buildLengthAndFlags(size_t length, size_t flags) {
        return (length << LENGTH_SHIFT) | flags;
    }

    inline js::gc::Cell *asCell() {
        return reinterpret_cast<js::gc::Cell *>(this);
    }

    inline js::gc::FreeCell *asFreeCell() {
        return reinterpret_cast<js::gc::FreeCell *>(this);
    }

    



    static const size_t MAX_LENGTH = (1 << 28) - 1;

    JS_ALWAYS_INLINE bool isDependent() const {
        return lengthAndFlags & DEPENDENT_BIT;
    }

    JS_ALWAYS_INLINE bool isFlat() const {
        return (lengthAndFlags & TYPE_MASK) == FLAT;
    }

    JS_ALWAYS_INLINE bool isExtensible() const {
        JS_ASSERT_IF(lengthAndFlags & EXTENSIBLE, isFlat());
        return lengthAndFlags & EXTENSIBLE;
    }

    JS_ALWAYS_INLINE bool isAtomized() const {
        JS_ASSERT_IF(lengthAndFlags & ATOMIZED, isFlat());
        return lengthAndFlags & ATOMIZED;
    }

    JS_ALWAYS_INLINE bool isRope() const {
        return lengthAndFlags & ROPE_BIT;
    }

    JS_ALWAYS_INLINE size_t length() const {
        return lengthAndFlags >> LENGTH_SHIFT;
    }

    JS_ALWAYS_INLINE bool empty() const {
        return lengthAndFlags <= TYPE_FLAGS_MASK;
    }

    
    JS_ALWAYS_INLINE const jschar *getChars(JSContext *cx) {
        if (isRope())
            return flatten(cx);
        return nonRopeChars();
    }

    
    JS_ALWAYS_INLINE const jschar *getCharsZ(JSContext *cx) {
        if (!isFlat())
            return undepend(cx);
        return flatChars();
    }

    JS_ALWAYS_INLINE void initFlatNotTerminated(jschar *chars, size_t length) {
        JS_ASSERT(length <= MAX_LENGTH);
        JS_ASSERT(!isStatic(this));
        lengthAndFlags = buildLengthAndFlags(length, FLAT);
        u.chars = chars;
    }

    
    JS_ALWAYS_INLINE void initFlat(jschar *chars, size_t length) {
        initFlatNotTerminated(chars, length);
        JS_ASSERT(chars[length] == jschar(0));
    }

    JS_ALWAYS_INLINE void initShortString(const jschar *chars, size_t length) {
        JS_ASSERT(length <= MAX_LENGTH);
        JS_ASSERT(chars >= inlineStorage && chars < (jschar *)(this + 2));
        JS_ASSERT(!isStatic(this));
        lengthAndFlags = buildLengthAndFlags(length, FLAT);
        u.chars = chars;
    }

    JS_ALWAYS_INLINE void initFlatExtensible(jschar *chars, size_t length, size_t cap) {
        JS_ASSERT(length <= MAX_LENGTH);
        JS_ASSERT(chars[length] == jschar(0));
        JS_ASSERT(!isStatic(this));
        lengthAndFlags = buildLengthAndFlags(length, FLAT | EXTENSIBLE);
        u.chars = chars;
        s.capacity = cap;
    }

    JS_ALWAYS_INLINE JSFlatString *assertIsFlat() {
        JS_ASSERT(isFlat());
        return reinterpret_cast<JSFlatString *>(this);
    }

    JS_ALWAYS_INLINE const jschar *flatChars() const {
        JS_ASSERT(isFlat());
        return u.chars;
    }

    JS_ALWAYS_INLINE size_t flatLength() const {
        JS_ASSERT(isFlat());
        return length();
    }

    inline void flatSetAtomized() {
        JS_ASSERT(isFlat());
        JS_ASSERT(!isStatic(this));
        lengthAndFlags |= ATOMIZED;
    }

    inline void flatClearExtensible() {
        



        JS_ASSERT(isFlat());
        if (lengthAndFlags & EXTENSIBLE)
            lengthAndFlags &= ~EXTENSIBLE;
    }

    



    inline void initDependent(JSString *base, const jschar *chars, size_t length) {
        JS_ASSERT(!isStatic(this));
        JS_ASSERT(base->isFlat());
        JS_ASSERT(chars >= base->flatChars() && chars < base->flatChars() + base->length());
        JS_ASSERT(length <= base->length() - (chars - base->flatChars()));
        lengthAndFlags = buildLengthAndFlags(length, DEPENDENT);
        u.chars = chars;
        s.base = base;
    }

    inline JSLinearString *dependentBase() const {
        JS_ASSERT(isDependent());
        return s.base->assertIsLinear();
    }

    JS_ALWAYS_INLINE const jschar *dependentChars() {
        JS_ASSERT(isDependent());
        return u.chars;
    }

    inline size_t dependentLength() const {
        JS_ASSERT(isDependent());
        return length();
    }

    const jschar *undepend(JSContext *cx);

    const jschar *nonRopeChars() const {
        JS_ASSERT(!isRope());
        return u.chars;
    }

    
    inline void initRopeNode(JSString *left, JSString *right, size_t length) {
        JS_ASSERT(left->length() + right->length() == length);
        lengthAndFlags = buildLengthAndFlags(length, ROPE);
        u.left = left;
        s.right = right;
    }

    inline JSString *ropeLeft() const {
        JS_ASSERT(isRope());
        return u.left;
    }

    inline JSString *ropeRight() const {
        JS_ASSERT(isRope());
        return s.right;
    }

    inline void finishTraversalConversion(JSString *base, const jschar *baseBegin, const jschar *end) {
        JS_ASSERT(baseBegin <= u.chars && u.chars <= end);
        lengthAndFlags = buildLengthAndFlags(end - u.chars, DEPENDENT);
        s.base = base;
    }

    const jschar *flatten(JSContext *maybecx);

    JSLinearString *ensureLinear(JSContext *cx) {
        if (isRope() && !flatten(cx))
            return NULL;
        return reinterpret_cast<JSLinearString *>(this);
    }

    bool isLinear() const {
        return !isRope();
    }

    JSLinearString *assertIsLinear() {
        JS_ASSERT(isLinear());
        return reinterpret_cast<JSLinearString *>(this);
    }

    typedef uint8 SmallChar;

    static inline bool fitsInSmallChar(jschar c) {
        return c < SMALL_CHAR_LIMIT && toSmallChar[c] != INVALID_SMALL_CHAR;
    }

    static inline bool isUnitString(void *ptr) {
        jsuword delta = reinterpret_cast<jsuword>(ptr) -
                        reinterpret_cast<jsuword>(unitStringTable);
        if (delta >= UNIT_STRING_LIMIT * sizeof(JSString))
            return false;

        
        JS_ASSERT(delta % sizeof(JSString) == 0);
        return true;
    }

    static inline bool isLength2String(void *ptr) {
        jsuword delta = reinterpret_cast<jsuword>(ptr) -
                        reinterpret_cast<jsuword>(length2StringTable);
        if (delta >= NUM_SMALL_CHARS * NUM_SMALL_CHARS * sizeof(JSString))
            return false;

        
        JS_ASSERT(delta % sizeof(JSString) == 0);
        return true;
    }

    static inline bool isHundredString(void *ptr) {
        jsuword delta = reinterpret_cast<jsuword>(ptr) -
                        reinterpret_cast<jsuword>(hundredStringTable);
        if (delta >= NUM_HUNDRED_STRINGS * sizeof(JSString))
            return false;

        
        JS_ASSERT(delta % sizeof(JSString) == 0);
        return true;
    }

    static inline bool isStatic(void *ptr) {
        return isUnitString(ptr) || isLength2String(ptr) || isHundredString(ptr);
    }

#ifdef __SUNPRO_CC
#pragma align 8 (__1cIJSStringPunitStringTable_, __1cIJSStringSlength2StringTable_, __1cIJSStringShundredStringTable_)
#endif

    static const SmallChar INVALID_SMALL_CHAR = -1;

    static const jschar fromSmallChar[];
    static const SmallChar toSmallChar[];
    static const JSString unitStringTable[];
    static const JSString length2StringTable[];
    static const JSString hundredStringTable[];
    



    static const JSString *const intStringTable[];

    static JSFlatString *unitString(jschar c);
    static JSLinearString *getUnitString(JSContext *cx, JSString *str, size_t index);
    static JSFlatString *length2String(jschar c1, jschar c2);
    static JSFlatString *length2String(uint32 i);
    static JSFlatString *intString(jsint i);

    static JSFlatString *lookupStaticString(const jschar *chars, size_t length);

    JS_ALWAYS_INLINE void finalize(JSContext *cx);

    static size_t offsetOfLengthAndFlags() {
        return offsetof(JSString, lengthAndFlags);
    }

    static size_t offsetOfChars() {
        return offsetof(JSString, u.chars);
    }

    static void staticAsserts() {
        JS_STATIC_ASSERT(((JSString::MAX_LENGTH << JSString::LENGTH_SHIFT) >>
                           JSString::LENGTH_SHIFT) == JSString::MAX_LENGTH);
    }
};






struct JSLinearString : JSString
{
    const jschar *chars() const { return JSString::nonRopeChars(); }
};

JS_STATIC_ASSERT(sizeof(JSLinearString) == sizeof(JSString));





struct JSFlatString : JSLinearString
{
    const jschar *charsZ() const { return chars(); }
};

JS_STATIC_ASSERT(sizeof(JSFlatString) == sizeof(JSString));





struct JSAtom : JSFlatString
{
};

struct JSExternalString : JSString
{
    static const uintN TYPE_LIMIT = 8;
    static JSStringFinalizeOp str_finalizers[TYPE_LIMIT];

    static intN changeFinalizer(JSStringFinalizeOp oldop,
                                JSStringFinalizeOp newop) {
        for (uintN i = 0; i != JS_ARRAY_LENGTH(str_finalizers); i++) {
            if (str_finalizers[i] == oldop) {
                str_finalizers[i] = newop;
                return intN(i);
            }
        }
        return -1;
    }

    void finalize(JSContext *cx);
    void finalize();
};

JS_STATIC_ASSERT(sizeof(JSString) == sizeof(JSExternalString));






class JSShortString : public js::gc::Cell
{
    JSString mHeader;
    JSString mDummy;

  public:
    




    inline jschar *init(size_t length) {
        JS_ASSERT(length <= MAX_SHORT_STRING_LENGTH);
        mHeader.initShortString(mHeader.inlineStorage, length);
        return mHeader.inlineStorage;
    }

    inline jschar *getInlineStorageBeforeInit() {
        return mHeader.inlineStorage;
    }

    inline void initAtOffsetInBuffer(jschar *p, size_t length) {
        JS_ASSERT(p >= mHeader.inlineStorage && p < mHeader.inlineStorage + MAX_SHORT_STRING_LENGTH);
        mHeader.initShortString(p, length);
    }

    inline void resetLength(size_t length) {
        mHeader.initShortString(mHeader.flatChars(), length);
    }

    inline JSString *header() {
        return &mHeader;
    }

    static const size_t FREE_STRING_WORDS = 2;

    static const size_t MAX_SHORT_STRING_LENGTH =
            ((sizeof(JSString) + FREE_STRING_WORDS * sizeof(size_t)) / sizeof(jschar)) - 1;

    static inline bool fitsIntoShortString(size_t length) {
        return length <= MAX_SHORT_STRING_LENGTH;
    }

    JS_ALWAYS_INLINE void finalize(JSContext *cx);

    static void staticAsserts() {
        JS_STATIC_ASSERT(offsetof(JSString, inlineStorage) ==
                         sizeof(JSString) - JSShortString::FREE_STRING_WORDS * sizeof(void *));
        JS_STATIC_ASSERT(offsetof(JSShortString, mDummy) == sizeof(JSString));
        JS_STATIC_ASSERT(offsetof(JSString, inlineStorage) +
                         sizeof(jschar) * (JSShortString::MAX_SHORT_STRING_LENGTH + 1) ==
                         sizeof(JSShortString));
    }
};

namespace js {

class StringBuffer;








class StringSegmentRange;
class MutatingRopeSegmentRange;




class RopeBuilder;

}  

extern const jschar *
js_GetStringChars(JSContext *cx, JSString *str);

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
           : w == NO_BREAK_SPACE || w == BYTE_ORDER_MARK ||
             (JS_CCODE(w) & 0x00070000) == 0x00040000 || w == 0xfffe || w == 0xfeff;
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
#define JS7_ISDECNZ(c)  ((((unsigned)(c)) - '1') <= 8)
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


extern JSFlatString *
js_NewString(JSContext *cx, jschar *chars, size_t length);

extern JSLinearString *
js_NewDependentString(JSContext *cx, JSString *base, size_t start,
                      size_t length);


extern JSFlatString *
js_NewStringCopyN(JSContext *cx, const jschar *s, size_t n);

extern JSFlatString *
js_NewStringCopyN(JSContext *cx, const char *s, size_t n);


extern JSFlatString *
js_NewStringCopyZ(JSContext *cx, const jschar *s);

extern JSFlatString *
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






extern bool
ValueToStringBuffer(JSContext *cx, const Value &v, StringBuffer &sb);

} 





extern JS_FRIEND_API(JSString *)
js_ValueToSource(JSContext *cx, const js::Value &v);





inline uint32
js_HashString(JSLinearString *str)
{
    const jschar *s = str->chars();
    size_t n = str->length();
    uint32 h;
    for (h = 0; n; s++, n--)
        h = JS_ROTATE_LEFT32(h, 4) ^ *s;
    return h;
}

namespace js {





extern bool
EqualStrings(JSContext *cx, JSString *str1, JSString *str2, JSBool *result);


extern bool
EqualStrings(JSLinearString *str1, JSLinearString *str2);





extern bool
CompareStrings(JSContext *cx, JSString *str1, JSString *str2, int32 *result);




extern bool
StringEqualsAscii(JSLinearString *str, const char *asciiBytes);

} 







static const jsuint sBMHCharSetSize = 256; 
static const jsuint sBMHPatLenMax   = 255; 
static const jsint  sBMHBadPattern  = -2;  

extern jsint
js_BoyerMooreHorspool(const jschar *text, jsuint textlen,
                      const jschar *pat, jsuint patlen);

extern size_t
js_strlen(const jschar *s);

extern jschar *
js_strchr(const jschar *s, jschar c);

extern jschar *
js_strchr_limit(const jschar *s, jschar c, const jschar *limit);

#define js_strncpy(t, s, n)     memcpy((t), (s), (n) * sizeof(jschar))

inline void
js_short_strncpy(jschar *dest, const jschar *src, size_t num)
{
    



    JS_ASSERT(JSShortString::fitsIntoShortString(num));
    for (size_t i = 0; i < num; i++)
        dest[i] = src[i];
}




static inline const jschar *
js_SkipWhiteSpace(const jschar *s, const jschar *end)
{
    JS_ASSERT(s <= end);
    while (s != end && JS_ISSPACE(*s))
        s++;
    return s;
}
















extern jschar *
js_InflateString(JSContext *cx, const char *bytes, size_t *length, bool useCESU8 = false);

extern char *
js_DeflateString(JSContext *cx, const jschar *chars, size_t length);







extern JSBool
js_InflateStringToBuffer(JSContext *cx, const char *bytes, size_t length,
                         jschar *chars, size_t *charsLength);




extern JSBool
js_InflateUTF8StringToBuffer(JSContext *cx, const char *bytes, size_t length,
                             jschar *chars, size_t *charsLength,
                             bool useCESU8 = false);





extern size_t
js_GetDeflatedStringLength(JSContext *cx, const jschar *chars,
                           size_t charsLength);





extern size_t
js_GetDeflatedUTF8StringLength(JSContext *cx, const jschar *chars,
                               size_t charsLength, bool useCESU8 = false);







extern JSBool
js_DeflateStringToBuffer(JSContext *cx, const jschar *chars,
                         size_t charsLength, char *bytes, size_t *length);




extern JSBool
js_DeflateStringToUTF8Buffer(JSContext *cx, const jschar *chars,
                             size_t charsLength, char *bytes, size_t *length,
                             bool useCESU8 = false);


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

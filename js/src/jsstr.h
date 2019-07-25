






































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



























































































class JSString : public js::gc::Cell
{
  protected:
    static const size_t NUM_INLINE_CHARS = 2 * sizeof(void *) / sizeof(jschar);

    
    struct Data
    {
        size_t                     lengthAndFlags;      
        union {
            const jschar           *chars;              
            JSString               *left;               
        } u1;
        union {
            jschar                 inlineStorage[NUM_INLINE_CHARS]; 
            struct {
                union {
                    JSLinearString *base;               
                    JSString       *right;              
                    size_t         capacity;            
                    size_t         externalType;        
                } u2;
                union {
                    JSString       *parent;             
                    void           *externalClosure;    
                    size_t         reserved;            
                } u3;
            } s;
        };
    } d;

  public:
    

    static const size_t LENGTH_SHIFT      = 4;
    static const size_t FLAGS_MASK        = JS_BITMASK(LENGTH_SHIFT);
    static const size_t MAX_LENGTH        = JS_BIT(32 - LENGTH_SHIFT) - 1;

    

























    static const size_t ROPE_BIT          = JS_BIT(0);

    static const size_t LINEAR_MASK       = JS_BITMASK(1);
    static const size_t LINEAR_FLAGS      = 0x0;

    static const size_t DEPENDENT_BIT     = JS_BIT(1);

    static const size_t FLAT_MASK         = JS_BITMASK(2);
    static const size_t FLAT_FLAGS        = 0x0;

    static const size_t FIXED_FLAGS       = JS_BIT(2);

    static const size_t ATOM_MASK         = JS_BITMASK(3);
    static const size_t ATOM_FLAGS        = 0x0;

    static const size_t STATIC_ATOM_MASK  = JS_BITMASK(4);
    static const size_t STATIC_ATOM_FLAGS = 0x0;

    static const size_t EXTENSIBLE_FLAGS  = JS_BIT(2) | JS_BIT(3);
    static const size_t NON_STATIC_ATOM   = JS_BIT(3);

    size_t buildLengthAndFlags(size_t length, size_t flags) {
        return (length << LENGTH_SHIFT) | flags;
    }

    static void staticAsserts() {
        JS_STATIC_ASSERT(size_t(JSString::MAX_LENGTH) <= size_t(JSVAL_INT_MAX));
        JS_STATIC_ASSERT(JSString::MAX_LENGTH <= JSVAL_INT_MAX);
        JS_STATIC_ASSERT(JS_BITS_PER_WORD >= 32);
        JS_STATIC_ASSERT(((JSString::MAX_LENGTH << JSString::LENGTH_SHIFT) >>
                           JSString::LENGTH_SHIFT) == JSString::MAX_LENGTH);
        JS_STATIC_ASSERT(sizeof(JSString) ==
                         offsetof(JSString, d.inlineStorage) +
                         NUM_INLINE_CHARS * sizeof(jschar));
    }

    
    friend class JSRope;

  public:
    

    JS_ALWAYS_INLINE
    size_t length() const {
        return d.lengthAndFlags >> LENGTH_SHIFT;
    }

    JS_ALWAYS_INLINE
    bool empty() const {
        return d.lengthAndFlags <= FLAGS_MASK;
    }

    




    inline const jschar *getChars(JSContext *cx);
    inline const jschar *getCharsZ(JSContext *cx);

    

    inline JSLinearString *ensureLinear(JSContext *cx);
    inline JSFlatString *ensureFlat(JSContext *cx);
    inline JSFixedString *ensureFixed(JSContext *cx);

    

    JS_ALWAYS_INLINE
    bool isRope() const {
        bool rope = d.lengthAndFlags & ROPE_BIT;
        JS_ASSERT_IF(rope, (d.lengthAndFlags & FLAGS_MASK) == ROPE_BIT);
        return rope;
    }

    JS_ALWAYS_INLINE
    JSRope &asRope() {
        JS_ASSERT(isRope());
        return *(JSRope *)this;
    }

    JS_ALWAYS_INLINE
    bool isLinear() const {
        return (d.lengthAndFlags & LINEAR_MASK) == LINEAR_FLAGS;
    }

    JS_ALWAYS_INLINE
    JSLinearString &asLinear() {
        JS_ASSERT(isLinear());
        return *(JSLinearString *)this;
    }

    JS_ALWAYS_INLINE
    bool isDependent() const {
        bool dependent = d.lengthAndFlags & DEPENDENT_BIT;
        JS_ASSERT_IF(dependent, (d.lengthAndFlags & FLAGS_MASK) == DEPENDENT_BIT);
        return dependent;
    }

    JS_ALWAYS_INLINE
    JSDependentString &asDependent() {
        JS_ASSERT(isDependent());
        return *(JSDependentString *)this;
    }

    JS_ALWAYS_INLINE
    bool isFlat() const {
        return (d.lengthAndFlags & FLAT_MASK) == FLAT_FLAGS;
    }

    JS_ALWAYS_INLINE
    JSFlatString &asFlat() {
        JS_ASSERT(isFlat());
        return *(JSFlatString *)this;
    }

    JS_ALWAYS_INLINE
    bool isExtensible() const {
        return (d.lengthAndFlags & FLAGS_MASK) == EXTENSIBLE_FLAGS;
    }

    JS_ALWAYS_INLINE
    JSExtensibleString &asExtensible() const {
        JS_ASSERT(isExtensible());
        return *(JSExtensibleString *)this;
    }

#ifdef DEBUG
    bool isShort() const;
    bool isFixed() const;
#endif

    JS_ALWAYS_INLINE
    JSFixedString &asFixed() {
        JS_ASSERT(isFixed());
        return *(JSFixedString *)this;
    }

    bool isExternal() const;

    JS_ALWAYS_INLINE
    JSExternalString &asExternal() {
        JS_ASSERT(isExternal());
        return *(JSExternalString *)this;
    }

    JS_ALWAYS_INLINE
    bool isAtom() const {
        bool atomized = (d.lengthAndFlags & ATOM_MASK) == ATOM_FLAGS;
        JS_ASSERT_IF(atomized, isFlat());
        return atomized;
    }

    JS_ALWAYS_INLINE
    JSAtom &asAtom() const {
        JS_ASSERT(isAtom());
        return *(JSAtom *)this;
    }

    JS_ALWAYS_INLINE
    bool isStaticAtom() const {
        return (d.lengthAndFlags & FLAGS_MASK) == STATIC_ATOM_FLAGS;
    }

    

    inline void finalize(JSContext *cx);

    

    static size_t offsetOfLengthAndFlags() {
        return offsetof(JSString, d.lengthAndFlags);
    }

    static size_t offsetOfChars() {
        return offsetof(JSString, d.u1.chars);
    }
};

class JSRope : public JSString
{
    friend class JSString;
    JSFlatString *flatten(JSContext *cx);

    void init(JSString *left, JSString *right, size_t length);

  public:
    static inline JSRope *new_(JSContext *cx, JSString *left,
                               JSString *right, size_t length);

    inline JSString *leftChild() const {
        JS_ASSERT(isRope());
        return d.u1.left;
    }

    inline JSString *rightChild() const {
        JS_ASSERT(isRope());
        return d.s.u2.right;
    }
};

JS_STATIC_ASSERT(sizeof(JSRope) == sizeof(JSString));

class JSLinearString : public JSString
{
    friend class JSString;

  public:
    void mark(JSTracer *trc);

    JS_ALWAYS_INLINE
    const jschar *chars() const {
        JS_ASSERT(isLinear());
        return d.u1.chars;
    }
};

JS_STATIC_ASSERT(sizeof(JSLinearString) == sizeof(JSString));

class JSDependentString : public JSLinearString
{
    friend class JSString;
    JSFixedString *undepend(JSContext *cx);

    void init(JSLinearString *base, const jschar *chars, size_t length);

  public:
    static inline JSDependentString *new_(JSContext *cx, JSLinearString *base,
                                          const jschar *chars, size_t length);

    JSLinearString *base() const {
        JS_ASSERT(isDependent());
        return d.s.u2.base;
    }
};

JS_STATIC_ASSERT(sizeof(JSDependentString) == sizeof(JSString));

class JSFlatString : public JSLinearString
{
    friend class JSRope;
    void morphExtensibleIntoDependent(JSLinearString *base) {
        d.lengthAndFlags = buildLengthAndFlags(length(), DEPENDENT_BIT);
        d.s.u2.base = base;
    }

  public:
    JS_ALWAYS_INLINE
    const jschar *charsZ() const {
        JS_ASSERT(isFlat());
        return chars();
    }

    

    inline void finalize(JSRuntime *rt);
};

JS_STATIC_ASSERT(sizeof(JSFlatString) == sizeof(JSString));

class JSExtensibleString : public JSFlatString
{
  public:
    JS_ALWAYS_INLINE
    size_t capacity() const {
        JS_ASSERT(isExtensible());
        return d.s.u2.capacity;
    }
};

JS_STATIC_ASSERT(sizeof(JSExtensibleString) == sizeof(JSString));

class JSFixedString : public JSFlatString
{
    void init(const jschar *chars, size_t length);

  public:
    static inline JSFixedString *new_(JSContext *cx, const jschar *chars, size_t length);

    




    inline JSAtom *morphAtomizedStringIntoAtom();
};

JS_STATIC_ASSERT(sizeof(JSFixedString) == sizeof(JSString));

class JSInlineString : public JSFixedString
{
    static const size_t MAX_INLINE_LENGTH = NUM_INLINE_CHARS - 1;

  public:
    static inline JSInlineString *new_(JSContext *cx);

    inline jschar *init(size_t length);

    inline void resetLength(size_t length);

    static bool lengthFits(size_t length) {
        return length <= MAX_INLINE_LENGTH;
    }

};

JS_STATIC_ASSERT(sizeof(JSInlineString) == sizeof(JSString));

class JSShortString : public JSInlineString
{
    
    static const size_t INLINE_EXTENSION_CHARS = sizeof(JSString::Data) / sizeof(jschar);

    static void staticAsserts() {
        JS_STATIC_ASSERT(INLINE_EXTENSION_CHARS % js::gc::Cell::CellSize == 0);
        JS_STATIC_ASSERT(MAX_SHORT_LENGTH + 1 ==
                         (sizeof(JSShortString) -
                          offsetof(JSShortString, d.inlineStorage)) / sizeof(jschar));
    }

    jschar inlineStorageExtension[INLINE_EXTENSION_CHARS];

  public:
    static inline JSShortString *new_(JSContext *cx);

    jschar *inlineStorageBeforeInit() {
        return d.inlineStorage;
    }

    inline void initAtOffsetInBuffer(const jschar *chars, size_t length);

    static const size_t MAX_SHORT_LENGTH = JSString::NUM_INLINE_CHARS +
                                           INLINE_EXTENSION_CHARS
                                           -1 ;

    static bool lengthFits(size_t length) {
        return length <= MAX_SHORT_LENGTH;
    }

    

    JS_ALWAYS_INLINE void finalize(JSContext *cx);
};

JS_STATIC_ASSERT(sizeof(JSShortString) == 2 * sizeof(JSString));





class JSExternalString : public JSFixedString
{
    static void staticAsserts() {
        JS_STATIC_ASSERT(TYPE_LIMIT == 8);
    }

    void init(const jschar *chars, size_t length, intN type, void *closure);

  public:
    static inline JSExternalString *new_(JSContext *cx, const jschar *chars,
                                         size_t length, intN type, void *closure);

    intN externalType() const {
        JS_ASSERT(isExternal());
        JS_ASSERT(d.s.u2.externalType < TYPE_LIMIT);
        return d.s.u2.externalType;
    }

    void *externalClosure() const {
        JS_ASSERT(isExternal());
        return d.s.u3.externalClosure;
    }

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

JS_STATIC_ASSERT(sizeof(JSExternalString) == sizeof(JSString));

class JSAtom : public JSFixedString
{
  public:
    

    static const size_t UNIT_STATIC_LIMIT   = 256U;
    static const size_t SMALL_CHAR_LIMIT    = 128U; 
    static const size_t NUM_SMALL_CHARS     = 64U;
    static const size_t INT_STATIC_LIMIT    = 256U;
    static const size_t NUM_HUNDRED_STATICS = 156U;

#ifdef __SUNPRO_CC
# pragma align 8 (__1cGJSAtomPunitStaticTable_, __1cGJSAtomSlength2StaticTable_, __1cGJSAtomShundredStaticTable_)
#endif
    static const JSString::Data unitStaticTable[];
    static const JSString::Data length2StaticTable[];
    static const JSString::Data hundredStaticTable[];
    static const JSString::Data *const intStaticTable[];

  private:
    
    static inline bool isUnitString(const void *ptr);
    static inline bool isLength2String(const void *ptr);
    static inline bool isHundredString(const void *ptr);

    typedef uint8 SmallChar;
    static const SmallChar INVALID_SMALL_CHAR = -1;

    static inline bool fitsInSmallChar(jschar c);

    static const jschar fromSmallChar[];
    static const SmallChar toSmallChar[];

    static void staticAsserts() {
        JS_STATIC_ASSERT(sizeof(JSString::Data) == sizeof(JSString));
    }

    static JSStaticAtom &length2Static(jschar c1, jschar c2);
    static JSStaticAtom &length2Static(uint32 i);

  public:
    



    static inline bool isStatic(const void *ptr);

    static inline bool hasIntStatic(int32 i);
    static inline JSStaticAtom &intStatic(jsint i);

    static inline bool hasUnitStatic(jschar c);
    static JSStaticAtom &unitStatic(jschar c);

    
    static inline JSLinearString *getUnitStringForElement(JSContext *cx, JSString *str, size_t index);

    
    static inline JSStaticAtom *lookupStatic(const jschar *chars, size_t length);

    inline void finalize(JSRuntime *rt);
};

JS_STATIC_ASSERT(sizeof(JSAtom) == sizeof(JSString));

class JSInlineAtom : public JSInlineString 
{
    



};

JS_STATIC_ASSERT(sizeof(JSInlineAtom) == sizeof(JSInlineString));

class JSShortAtom : public JSShortString 
{
    



};

JS_STATIC_ASSERT(sizeof(JSShortAtom) == sizeof(JSShortString));

class JSStaticAtom : public JSAtom
{};

JS_STATIC_ASSERT(sizeof(JSStaticAtom) == sizeof(JSString));



JS_ALWAYS_INLINE const jschar *
JSString::getChars(JSContext *cx)
{
    if (JSLinearString *str = ensureLinear(cx))
        return str->chars();
    return NULL;
}

JS_ALWAYS_INLINE const jschar *
JSString::getCharsZ(JSContext *cx)
{
    if (JSFlatString *str = ensureFlat(cx))
        return str->chars();
    return NULL;
}

JS_ALWAYS_INLINE JSLinearString *
JSString::ensureLinear(JSContext *cx)
{
    return isLinear()
           ? &asLinear()
           : asRope().flatten(cx);
}

JS_ALWAYS_INLINE JSFlatString *
JSString::ensureFlat(JSContext *cx)
{
    return isFlat()
           ? &asFlat()
           : isDependent()
             ? asDependent().undepend(cx)
             : asRope().flatten(cx);
}

JS_ALWAYS_INLINE JSFixedString *
JSString::ensureFixed(JSContext *cx)
{
    if (!ensureFlat(cx))
        return NULL;
    if (isExtensible()) {
        JS_ASSERT((d.lengthAndFlags & FLAT_MASK) == 0);
        JS_STATIC_ASSERT(EXTENSIBLE_FLAGS == (JS_BIT(2) | JS_BIT(3)));
        JS_STATIC_ASSERT(FIXED_FLAGS == JS_BIT(2));
        d.lengthAndFlags ^= JS_BIT(3);
    }
    return &asFixed();
}

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

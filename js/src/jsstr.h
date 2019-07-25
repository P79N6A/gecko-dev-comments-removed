






































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

#define JSSTRING_BIT(n)             ((size_t)1 << (n))
#define JSSTRING_BITMASK(n)         (JSSTRING_BIT(n) - 1)

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








































struct JSString {
    friend class js::TraceRecorder;
    friend class js::mjit::Compiler;

    friend JSAtom *
    js_AtomizeString(JSContext *cx, JSString *str, uintN flags);
 public:
    



    size_t                          mLengthAndFlags;  
    union {
        jschar                      *mChars; 
        JSString                    *mLeft;  
    };
    union {
        



        jschar                      mInlineStorage[4]; 
        struct {
            union {
                size_t              mCapacity; 
                JSString            *mParent; 
                JSRopeBufferInfo    *mBufferWithInfo; 
            };
            union {
                JSString            *mBase;  
                JSString            *mRight; 
            };
        } e;
        uintN                       externalStringType; 
    };

    














    static const size_t FLAT =          0;
    static const size_t DEPENDENT =     1;
    static const size_t INTERIOR_NODE = 2;
    static const size_t TOP_NODE =      3;

    
    static const size_t ROPE_BIT = JSSTRING_BIT(1);

    static const size_t ATOMIZED = JSSTRING_BIT(2);
    static const size_t EXTENSIBLE = JSSTRING_BIT(3);

    static const size_t FLAGS_LENGTH_SHIFT = 4;

    static const size_t TYPE_MASK = JSSTRING_BITMASK(2);
    static const size_t TYPE_FLAGS_MASK = JSSTRING_BITMASK(4);

    inline bool hasFlag(size_t flag) const {
        return (mLengthAndFlags & flag) != 0;
    }

    inline js::gc::Cell *asCell() {
        return reinterpret_cast<js::gc::Cell *>(this);
    }
    
    inline js::gc::FreeCell *asFreeCell() {
        return reinterpret_cast<js::gc::FreeCell *>(this);
    }

    



    static const size_t MAX_LENGTH = (1 << 28) - 1;

    inline size_t type() const {
        return mLengthAndFlags & TYPE_MASK;
    }

    JS_ALWAYS_INLINE bool isDependent() const {
        return type() == DEPENDENT;
    }

    JS_ALWAYS_INLINE bool isFlat() const {
        return type() == FLAT;
    }

    inline bool isExtensible() const {
        return isFlat() && hasFlag(EXTENSIBLE);
    }

    inline bool isRope() const {
        return hasFlag(ROPE_BIT);
    }

    JS_ALWAYS_INLINE bool isAtomized() const {
        return isFlat() && hasFlag(ATOMIZED);
    }

    inline bool isInteriorNode() const {
        return type() == INTERIOR_NODE;
    }

    inline bool isTopNode() const {
        return type() == TOP_NODE;
    }

    JS_ALWAYS_INLINE jschar *chars() {
        if (JS_UNLIKELY(isRope()))
            flatten();
        return mChars;
    }

    JS_ALWAYS_INLINE size_t length() const {
        return mLengthAndFlags >> FLAGS_LENGTH_SHIFT;
    }

    JS_ALWAYS_INLINE bool empty() const {
        return length() == 0;
    }

    JS_ALWAYS_INLINE void getCharsAndLength(const jschar *&chars, size_t &length) {
        chars = this->chars();
        length = this->length();
    }

    JS_ALWAYS_INLINE void getCharsAndEnd(const jschar *&chars, const jschar *&end) {
        end = length() + (chars = this->chars());
    }

    JS_ALWAYS_INLINE jschar *inlineStorage() {
        JS_ASSERT(isFlat());
        return mInlineStorage;
    }

    
    JS_ALWAYS_INLINE void initFlat(jschar *chars, size_t length) {
        JS_ASSERT(length <= MAX_LENGTH);
        JS_ASSERT(!isStatic(this));
        e.mBase = NULL;
        e.mCapacity = 0;
        mLengthAndFlags = (length << FLAGS_LENGTH_SHIFT) | FLAT;
        mChars = chars;
    }

    JS_ALWAYS_INLINE void initShortString(jschar *chars, size_t length) {
        JS_ASSERT(length <= MAX_LENGTH);
        JS_ASSERT(!isStatic(this));
        mLengthAndFlags = (length << FLAGS_LENGTH_SHIFT) | FLAT;
        mChars = chars;
    }

    JS_ALWAYS_INLINE void initFlatExtensible(jschar *chars, size_t length, size_t cap) {
        JS_ASSERT(length <= MAX_LENGTH);
        JS_ASSERT(!isStatic(this));
        e.mBase = NULL;
        e.mCapacity = cap;
        mLengthAndFlags = (length << FLAGS_LENGTH_SHIFT) | FLAT | EXTENSIBLE;
        mChars = chars;
    }

    JS_ALWAYS_INLINE jschar *flatChars() const {
        JS_ASSERT(isFlat());
        return mChars;
    }

    JS_ALWAYS_INLINE size_t flatLength() const {
        JS_ASSERT(isFlat());
        return length();
    }

    JS_ALWAYS_INLINE size_t flatCapacity() const {
        JS_ASSERT(isFlat());
        return e.mCapacity;
    }

    


























    inline void flatSetAtomized() {
        JS_ASSERT(isFlat());
        JS_ASSERT(!isStatic(this));
        JS_ATOMIC_SET_MASK((jsword *)&mLengthAndFlags, ATOMIZED);
    }

    inline void flatSetExtensible() {
        JS_ASSERT(isFlat());
        JS_ASSERT(!isAtomized());
        mLengthAndFlags |= EXTENSIBLE;
    }

    inline void flatClearExtensible() {
        JS_ASSERT(isFlat());

        



        if (mLengthAndFlags & EXTENSIBLE)
            mLengthAndFlags &= ~EXTENSIBLE;
    }

    



    inline void initDependent(JSString *bstr, jschar *chars, size_t len) {
        JS_ASSERT(len <= MAX_LENGTH);
        JS_ASSERT(!isStatic(this));
        e.mParent = NULL;
        mChars = chars;
        mLengthAndFlags = DEPENDENT | (len << FLAGS_LENGTH_SHIFT);
        e.mBase = bstr;
    }

    inline JSString *dependentBase() const {
        JS_ASSERT(isDependent());
        return e.mBase;
    }

    JS_ALWAYS_INLINE jschar *dependentChars() {
        return mChars;
    }

    inline size_t dependentLength() const {
        JS_ASSERT(isDependent());
        return length();
    }

    
    inline void initTopNode(JSString *left, JSString *right, size_t len,
                            JSRopeBufferInfo *buf) {
        JS_ASSERT(left->length() + right->length() <= MAX_LENGTH);
        JS_ASSERT(!isStatic(this));
        mLengthAndFlags = TOP_NODE | (len << FLAGS_LENGTH_SHIFT);
        mLeft = left;
        e.mRight = right;
        e.mBufferWithInfo = buf;
    }

    inline void convertToInteriorNode(JSString *parent) {
        JS_ASSERT(isTopNode());
        e.mParent = parent;
        mLengthAndFlags = INTERIOR_NODE | (length() << FLAGS_LENGTH_SHIFT);
    }

    inline JSString *interiorNodeParent() const {
        JS_ASSERT(isInteriorNode());
        return e.mParent;
    }

    inline JSString *ropeLeft() const {
        JS_ASSERT(isRope());
        return mLeft;
    }

    inline JSString *ropeRight() const {
        JS_ASSERT(isRope());
        return e.mRight;
    }

    inline size_t topNodeCapacity() const {
        JS_ASSERT(isTopNode());
        return e.mBufferWithInfo->capacity;
    }

    inline JSRopeBufferInfo *topNodeBuffer() const {
        JS_ASSERT(isTopNode());
        return e.mBufferWithInfo;
    }

    inline void nullifyTopNodeBuffer() {
        JS_ASSERT(isTopNode());
        e.mBufferWithInfo = NULL;
    }

    inline void finishTraversalConversion(JSString *base, jschar *end) {
        mLengthAndFlags = JSString::DEPENDENT |
                          ((end - mChars) << JSString::FLAGS_LENGTH_SHIFT);
        e.mBase = base;
    }

    inline bool ensureNotDependent(JSContext *cx) {
        return !isDependent() || undepend(cx);
    }

    inline void ensureNotRope() {
        if (isRope())
            flatten();
    }

    const jschar *undepend(JSContext *cx);

    
    void flatten();

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

    static JSString *unitString(jschar c);
    static JSString *getUnitString(JSContext *cx, JSString *str, size_t index);
    static JSString *length2String(jschar c1, jschar c2);
    static JSString *length2String(uint32 i);
    static JSString *intString(jsint i);

    static JSString *lookupStaticString(const jschar *chars, size_t length);
    
    JS_ALWAYS_INLINE void finalize(JSContext *cx);
};

struct JSExternalString : JSString {
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






struct JSShortString : js::gc::Cell {
    JSString mHeader;
    JSString mDummy;

    




    inline jschar *init(size_t length) {
        JS_ASSERT(length <= MAX_SHORT_STRING_LENGTH);
        mHeader.initShortString(mHeader.inlineStorage(), length);
        return mHeader.inlineStorage();
    }

    inline jschar *getInlineStorageBeforeInit() {
        return mHeader.mInlineStorage;
    }

    inline void initAtOffsetInBuffer(jschar *p, size_t length) {
        JS_ASSERT(p >= mHeader.mInlineStorage && p < mHeader.mInlineStorage + MAX_SHORT_STRING_LENGTH);
        mHeader.initShortString(p, length);
    }

    inline void resetLength(size_t length) {
        mHeader.initShortString(mHeader.flatChars(), length);
    }

    inline JSString *header() {
        return &mHeader;
    }

    static const size_t MAX_SHORT_STRING_LENGTH =
            ((sizeof(JSString) + 2 * sizeof(size_t)) / sizeof(jschar)) - 1;

    static inline bool fitsIntoShortString(size_t length) {
        return length <= MAX_SHORT_STRING_LENGTH;
    }

    JS_ALWAYS_INLINE void finalize(JSContext *cx);
};





JS_STATIC_ASSERT(offsetof(JSString, mInlineStorage) == 2 * sizeof(void *));
JS_STATIC_ASSERT(offsetof(JSShortString, mDummy) == sizeof(JSString));
JS_STATIC_ASSERT(offsetof(JSString, mInlineStorage) +
                 sizeof(jschar) * (JSShortString::MAX_SHORT_STRING_LENGTH + 1) ==
                 sizeof(JSShortString));












class JSRopeNodeIterator {
  private:
    JSString *mStr;
    size_t mUsedFlags;

    static const size_t DONE_LEFT = 0x1;
    static const size_t DONE_RIGHT = 0x2;

  public:
    JSRopeNodeIterator(JSString *str)
      : mUsedFlags(0)
    {
        mStr = str;
    }
    
    JSString *init() {
        
        if (!mStr->isRope()) {
            JSString *oldStr = mStr;
            mStr = NULL;
            return oldStr;
        }
        
        while (mStr->isInteriorNode())
            mStr = mStr->interiorNodeParent();
        while (mStr->ropeLeft()->isInteriorNode())
            mStr = mStr->ropeLeft();
        JS_ASSERT(mUsedFlags == 0);
        return mStr;
    }

    JSString *next() {
        if (!mStr)
            return NULL;
        if (!mStr->ropeLeft()->isInteriorNode() && !(mUsedFlags & DONE_LEFT)) {
            mUsedFlags |= DONE_LEFT;
            return mStr->ropeLeft();
        }
        if (!mStr->ropeRight()->isInteriorNode() && !(mUsedFlags & DONE_RIGHT)) {
            mUsedFlags |= DONE_RIGHT;
            return mStr->ropeRight();
        }
        if (mStr->ropeRight()->isInteriorNode()) {
            



            mStr = mStr->ropeRight();
            while (mStr->ropeLeft()->isInteriorNode())
                mStr = mStr->ropeLeft();
        } else {
            



            JSString *prev;
            do {
                prev = mStr;
                
                mStr = mStr->isInteriorNode() ? mStr->interiorNodeParent() : NULL;
            } while (mStr && mStr->ropeRight() == prev);
        }
        mUsedFlags = 0;
        return mStr;
    }
};





class JSRopeLeafIterator {
  private:
    JSRopeNodeIterator mNodeIterator;

  public:

    JSRopeLeafIterator(JSString *topNode) :
        mNodeIterator(topNode) {
        JS_ASSERT(topNode->isTopNode());
    }

    inline JSString *init() {
        JSString *str = mNodeIterator.init();
        while (str->isRope()) {
            str = mNodeIterator.next();
            JS_ASSERT(str);
        }
        return str;
    }

    inline JSString *next() {
        JSString *str;
        do {
            str = mNodeIterator.next();
        } while (str && str->isRope());
        return str;
    }
};

class JSRopeBuilder {
    JSContext   * const cx;
    JSString    *mStr;

  public:
    JSRopeBuilder(JSContext *cx);

    inline bool append(JSString *str) {
        mStr = js_ConcatStrings(cx, mStr, str);
        return !!mStr;
    }

    inline JSString *getStr() {
        return mStr;
    }
};
     
JS_STATIC_ASSERT(JSString::INTERIOR_NODE & JSString::ROPE_BIT);
JS_STATIC_ASSERT(JSString::TOP_NODE & JSString::ROPE_BIT);

JS_STATIC_ASSERT(((JSString::MAX_LENGTH << JSString::FLAGS_LENGTH_SHIFT) >>
                   JSString::FLAGS_LENGTH_SHIFT) == JSString::MAX_LENGTH);

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

const jschar BYTE_ORDER_MARK = 0xFEFF;
const jschar NO_BREAK_SPACE  = 0x00A0;

static inline bool
JS_ISSPACE(jschar c)
{
    unsigned w = c;

    if (w < 256)
        return (w <= ' ' && (w == ' ' || (9 <= w && w <= 0xD))) || w == NO_BREAK_SPACE;

    return w == BYTE_ORDER_MARK || (JS_CCODE(w) & 0x00070000) == 0x00040000;
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
js_NewStringCopyN(JSContext *cx, const char *s, size_t n);


extern JSString *
js_NewStringCopyZ(JSContext *cx, const jschar *s);

extern JSString *
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

}






extern JSBool
js_ValueToCharBuffer(JSContext *cx, const js::Value &v, JSCharBuffer &cb);





extern JS_FRIEND_API(JSString *)
js_ValueToSource(JSContext *cx, const js::Value &v);





extern uint32
js_HashString(JSString *str);





extern JSBool JS_FASTCALL
js_EqualStrings(JSString *str1, JSString *str2);





extern int32 JS_FASTCALL
js_CompareStrings(JSString *str1, JSString *str2);

namespace js {



extern JSBool
MatchStringAndAscii(JSString *str, const char *asciiBytes);

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
js_InflateString(JSContext *cx, const char *bytes, size_t *length);

extern char *
js_DeflateString(JSContext *cx, const jschar *chars, size_t length);







extern JSBool
js_InflateStringToBuffer(JSContext *cx, const char *bytes, size_t length,
                         jschar *chars, size_t *charsLength);




extern JSBool
js_InflateUTF8StringToBuffer(JSContext *cx, const char *bytes, size_t length,
                             jschar *chars, size_t *charsLength);





extern size_t
js_GetDeflatedStringLength(JSContext *cx, const jschar *chars,
                           size_t charsLength);




extern size_t
js_GetDeflatedUTF8StringLength(JSContext *cx, const jschar *chars,
                               size_t charsLength);







extern JSBool
js_DeflateStringToBuffer(JSContext *cx, const jschar *chars,
                         size_t charsLength, char *bytes, size_t *length);




extern JSBool
js_DeflateStringToUTF8Buffer(JSContext *cx, const jschar *chars,
                             size_t charsLength, char *bytes, size_t *length);


extern JSBool
js_str_escape(JSContext *cx, JSObject *obj, uintN argc, js::Value *argv,
              js::Value *rval);





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
PutEscapedStringImpl(char *buffer, size_t size, FILE *fp, JSString *str, uint32 quote);










inline size_t
PutEscapedString(char *buffer, size_t size, JSString *str, uint32 quote)
{
    size_t n = PutEscapedStringImpl(buffer, size, NULL, str, quote);

    
    JS_ASSERT(n != size_t(-1));
    return n;
}






inline bool
FileEscapedString(FILE *fp, JSString *str, uint32 quote)
{
    return PutEscapedStringImpl(NULL, 0, fp, str, quote) != size_t(-1);
}

} 

extern JSBool
js_String(JSContext *cx, uintN argc, js::Value *vp);

#endif 

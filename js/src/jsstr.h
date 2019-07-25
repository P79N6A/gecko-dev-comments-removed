






































#ifndef jsstr_h___
#define jsstr_h___









#include <ctype.h>
#include "jsapi.h"
#include "jsprvtd.h"
#include "jshashtable.h"
#include "jslock.h"
#include "jsobj.h"

#define JSSTRING_BIT(n)             ((size_t)1 << (n))
#define JSSTRING_BITMASK(n)         (JSSTRING_BIT(n) - 1)

enum {
    UNIT_STRING_LIMIT        = 256U,
    INT_STRING_LIMIT         = 256U
};

extern jschar *
js_GetDependentStringChars(JSString *str);

JS_STATIC_ASSERT(JS_BITS_PER_WORD >= 32);


























struct JSString {
    friend class js::TraceRecorder;

    friend JSAtom *
    js_AtomizeString(JSContext *cx, JSString *str, uintN flags);

    friend JSString * JS_FASTCALL
    js_ConcatStrings(JSContext *cx, JSString *left, JSString *right);

    
    
    size_t          mLength;
    size_t          mOffset;
    jsword          mFlags;
    union {
        jschar      *mChars;
        JSString    *mBase;
    };

    




    static const size_t DEPENDENT =     JSSTRING_BIT(1);
    static const size_t MUTABLE =       JSSTRING_BIT(2);
    static const size_t ATOMIZED =      JSSTRING_BIT(3);

    inline bool hasFlag(size_t flag) const {
        return (mFlags & flag) != 0;
    }

  public:
    



    static const size_t MAX_LENGTH = (1 << 28) - 1;

    inline bool isDependent() const {
        return hasFlag(DEPENDENT);
    }

    inline bool isFlat() const {
        return !isDependent();
    }

    inline bool isMutable() const {
        return !isDependent() && hasFlag(MUTABLE);
    }

    inline bool isAtomized() const {
        return !isDependent() && hasFlag(ATOMIZED);
    }

    inline jschar *chars() {
        return isDependent() ? dependentChars() : flatChars();
    }

    inline size_t length() const {
        return mLength;
    }

    inline bool empty() const {
        return length() == 0;
    }

    inline void getCharsAndLength(const jschar *&chars, size_t &length) {
        chars = this->chars();
        length = this->length();
    }

    inline void getCharsAndEnd(const jschar *&chars, const jschar *&end) {
        end = length() + (chars = this->chars());
    }

    
    inline void initFlat(jschar *chars, size_t length) {
        JS_ASSERT(length <= MAX_LENGTH);
        mLength = length;
        mOffset = 0;
        mFlags = 0;
        mChars = chars;
    }

    inline jschar *flatChars() const {
        JS_ASSERT(isFlat());
        return mChars;
    }

    inline size_t flatLength() const {
        JS_ASSERT(isFlat());
        return length();
    }

    


























    inline void flatSetAtomized() {
        JS_ASSERT(isFlat() && !isMutable());
        JS_ATOMIC_SET_MASK(&mFlags, ATOMIZED);
    }

    inline void flatSetMutable() {
        JS_ASSERT(isFlat() && !isAtomized());
        mFlags |= MUTABLE;
    }

    inline void flatClearMutable() {
        JS_ASSERT(isFlat());
        if (hasFlag(MUTABLE))
            mFlags &= ~MUTABLE;
    }

    inline void initDependent(JSString *bstr, size_t off, size_t len) {
        JS_ASSERT(len <= MAX_LENGTH);
        mLength = len;
        mOffset = off;
        mFlags = DEPENDENT;
        mBase = bstr;
    }

    inline JSString *dependentBase() const {
        JS_ASSERT(isDependent());
        return mBase;
    }

    inline jschar *dependentChars() {
        return dependentBase()->isDependent()
               ? js_GetDependentStringChars(this)
               : dependentBase()->flatChars() + dependentStart();
    }

    inline size_t dependentStart() const {
        return mOffset;
    }

    inline size_t dependentLength() const {
        JS_ASSERT(isDependent());
        return length();
    }

    static inline bool isUnitString(void *ptr) {
        jsuword delta = reinterpret_cast<jsuword>(ptr) -
                        reinterpret_cast<jsuword>(unitStringTable);
        if (delta >= UNIT_STRING_LIMIT * sizeof(JSString))
            return false;

        
        JS_ASSERT(delta % sizeof(JSString) == 0);
        return true;
    }

    static inline bool isIntString(void *ptr) {
        jsuword delta = reinterpret_cast<jsuword>(ptr) -
                        reinterpret_cast<jsuword>(intStringTable);
        if (delta >= INT_STRING_LIMIT * sizeof(JSString))
            return false;

        
        JS_ASSERT(delta % sizeof(JSString) == 0);
        return true;
    }

    static inline bool isStatic(void *ptr) {
        return isUnitString(ptr) || isIntString(ptr);
    }

#ifdef __SUNPRO_CC
#pragma align 8 (__1cIJSStringPunitStringTable_, __1cIJSStringOintStringTable_)
#endif

#ifndef JS_64BIT
    static JSString unitStringTable[];
    static JSString intStringTable[];
#else
    static JSString staticUnitStringTable[];
    static JSString staticIntStringTable[];
    static JSString *unitStringTable;
    static JSString *intStringTable;

    static size_t unitStringTableSize;
    static size_t intStringTableSize;
    static bool initStringTables();
    static void freeStringTables();
#endif
    static const char *deflatedIntStringTable[];
    static const char deflatedUnitStringTable[];

    static JSString *unitString(jschar c);
    static JSString *getUnitString(JSContext *cx, JSString *str, size_t index);
    static JSString *intString(jsint i);
};

JS_STATIC_ASSERT(sizeof(JSString) % JS_GCTHING_ALIGN == 0);

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
js_NewStringCopyZ(JSContext *cx, const jschar *s);




typedef JSString *(*JSValueToStringFun)(JSContext *cx, const js::Value &v);

extern JS_FRIEND_API(const char *)
js_ValueToPrintable(JSContext *cx, const js::Value &, JSValueToStringFun v2sfun);

#define js_ValueToPrintableString(cx,v) \
    js_ValueToPrintable(cx, v, js_ValueToString)

#define js_ValueToPrintableSource(cx,v) \
    js_ValueToPrintable(cx, v, js_ValueToSource)





extern JSString *
js_ValueToString(JSContext *cx, const js::Value &v);






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





extern const char *
js_GetStringBytes(JSContext *cx, JSString *str);


extern JSBool
js_str_escape(JSContext *cx, JSObject *obj, uintN argc, js::Value *argv,
              js::Value *rval);

extern JSBool
js_str_toString(JSContext *cx, uintN argc, js::Value *vp);





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
js_String(JSContext *cx, JSObject *obj, uintN argc, js::Value *argv, js::Value *rval);

namespace js {

class DeflatedStringCache {
  public:
    DeflatedStringCache();
    bool init();
    ~DeflatedStringCache();

    void sweep(JSContext *cx);
    void remove(JSString *str);
    bool setBytes(JSContext *cx, JSString *str, char *bytes);

  private:
    struct StringPtrHasher
    {
        typedef JSString *Lookup;

        static HashNumber hash(JSString *str) {
            




            const jsuword ALIGN_LOG = tl::FloorLog2<sizeof(JSString)>::result;
            JS_STATIC_ASSERT(sizeof(JSString) == (size_t(1) << ALIGN_LOG));

            jsuword ptr = reinterpret_cast<jsuword>(str);
            jsuword key = ptr >> ALIGN_LOG;
            JS_ASSERT((key << ALIGN_LOG) == ptr);
            return HashNumber(key);
        }

        static bool match(JSString *s1, JSString *s2) {
            return s1 == s2;
        }
    };

    typedef HashMap<JSString *, char *, StringPtrHasher, SystemAllocPolicy> Map;

    
    char *getBytes(JSContext *cx, JSString *str);

    friend const char *
    ::js_GetStringBytes(JSContext *cx, JSString *str);

    Map                 map;
#ifdef JS_THREADSAFE
    JSLock              *lock;
#endif
};

} 

#endif 

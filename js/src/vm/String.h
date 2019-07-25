







































#ifndef String_h_
#define String_h_

#include "jscell.h"

class JSDependentString;
class JSExtensibleString;
class JSExternalString;
class JSLinearString;
class JSFixedString;
class JSStaticAtom;
class JSRope;
class JSAtom;



























































































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

    
    bool isShort() const;
    bool isFixed() const;
    bool isInline() const;

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

    

    JS_FRIEND_API(size_t) charsHeapSize();

    

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

#endif








#ifndef String_h_
#define String_h_

#include "mozilla/Attributes.h"

#include "jsapi.h"
#include "jsatom.h"
#include "jsfriendapi.h"
#include "jsstr.h"

#include "gc/Barrier.h"
#include "gc/Heap.h"

class JSString;
class JSDependentString;
class JSUndependedString;
class JSExtensibleString;
class JSExternalString;
class JSLinearString;
class JSFixedString;
class JSRope;
class JSAtom;

namespace js {

class StaticStrings;
class PropertyName;


static const size_t UINT32_CHAR_BUFFER_LENGTH = sizeof("4294967295") - 1;

} 



























































































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
                    const JSStringFinalizer *externalFinalizer;
                } u2;
                union {
                    JSString       *parent;             
                    size_t         reserved;            
                } u3;
            } s;
        };
    } d;

  public:
    

    









































    static const size_t LENGTH_SHIFT          = 4;
    static const size_t FLAGS_MASK            = JS_BITMASK(LENGTH_SHIFT);

    static const size_t ROPE_FLAGS            = 0;
    static const size_t DEPENDENT_FLAGS       = JS_BIT(0);
    static const size_t UNDEPENDED_FLAGS      = JS_BIT(0) | JS_BIT(1);
    static const size_t EXTENSIBLE_FLAGS      = JS_BIT(1);
    static const size_t FIXED_FLAGS           = JS_BIT(2);

    static const size_t INT32_MASK            = JS_BITMASK(3);
    static const size_t INT32_FLAGS           = JS_BIT(1) | JS_BIT(2);

    static const size_t HAS_BASE_BIT          = JS_BIT(0);
    static const size_t ATOM_BIT              = JS_BIT(3);

    static const size_t MAX_LENGTH            = JS_BIT(32 - LENGTH_SHIFT) - 1;

    size_t buildLengthAndFlags(size_t length, size_t flags) {
        JS_ASSERT(length <= MAX_LENGTH);
        JS_ASSERT(flags <= FLAGS_MASK);
        return (length << LENGTH_SHIFT) | flags;
    }

    




    static inline bool validateLength(JSContext *cx, size_t length);

    static void staticAsserts() {
        JS_STATIC_ASSERT(JS_BITS_PER_WORD >= 32);
        JS_STATIC_ASSERT(((JSString::MAX_LENGTH << JSString::LENGTH_SHIFT) >>
                           JSString::LENGTH_SHIFT) == JSString::MAX_LENGTH);
        JS_STATIC_ASSERT(sizeof(JSString) ==
                         offsetof(JSString, d.inlineStorage) + NUM_INLINE_CHARS * sizeof(jschar));
        JS_STATIC_ASSERT(offsetof(JSString, d.u1.chars) ==
                         offsetof(js::shadow::Atom, chars));
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

    static bool ensureLinear(JSContext *cx, JSString *str) {
        return str->ensureLinear(cx) != NULL;
    }

    

    JS_ALWAYS_INLINE
    bool isRope() const {
        return (d.lengthAndFlags & FLAGS_MASK) == ROPE_FLAGS;
    }

    JS_ALWAYS_INLINE
    JSRope &asRope() const {
        JS_ASSERT(isRope());
        return *(JSRope *)this;
    }

    JS_ALWAYS_INLINE
    bool isLinear() const {
        return !isRope();
    }

    JS_ALWAYS_INLINE
    JSLinearString &asLinear() const {
        JS_ASSERT(JSString::isLinear());
        return *(JSLinearString *)this;
    }

    JS_ALWAYS_INLINE
    bool isDependent() const {
        return (d.lengthAndFlags & FLAGS_MASK) == DEPENDENT_FLAGS;
    }

    JS_ALWAYS_INLINE
    JSDependentString &asDependent() const {
        JS_ASSERT(isDependent());
        return *(JSDependentString *)this;
    }

    JS_ALWAYS_INLINE
    bool isFlat() const {
        return isLinear() && !isDependent();
    }

    JS_ALWAYS_INLINE
    JSFlatString &asFlat() const {
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
    JSFixedString &asFixed() const {
        JS_ASSERT(isFixed());
        return *(JSFixedString *)this;
    }

    bool isExternal() const;

    JS_ALWAYS_INLINE
    JSExternalString &asExternal() const {
        JS_ASSERT(isExternal());
        return *(JSExternalString *)this;
    }

    JS_ALWAYS_INLINE
    bool isUndepended() const {
        return (d.lengthAndFlags & FLAGS_MASK) == UNDEPENDED_FLAGS;
    }

    JS_ALWAYS_INLINE
    bool isAtom() const {
        return (d.lengthAndFlags & ATOM_BIT);
    }

    JS_ALWAYS_INLINE
    JSAtom &asAtom() const {
        JS_ASSERT(isAtom());
        return *(JSAtom *)this;
    }

    

    inline bool hasBase() const {
        JS_STATIC_ASSERT((DEPENDENT_FLAGS | JS_BIT(1)) == UNDEPENDED_FLAGS);
        return (d.lengthAndFlags & HAS_BASE_BIT);
    }

    inline JSLinearString *base() const;

    inline void markBase(JSTracer *trc);

    

    inline void finalize(js::FreeOp *fop);

    

    size_t sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf);

    

    static size_t offsetOfLengthAndFlags() {
        return offsetof(JSString, d.lengthAndFlags);
    }

    static size_t offsetOfChars() {
        return offsetof(JSString, d.u1.chars);
    }

    static inline void writeBarrierPre(JSString *str);
    static inline void writeBarrierPost(JSString *str, void *addr);
    static inline bool needWriteBarrierPre(JSCompartment *comp);
    static inline void readBarrier(JSString *str);

    static inline js::ThingRootKind rootKind() { return js::THING_ROOT_STRING; }

#ifdef DEBUG
    void dump();
    bool equals(const char *s);
#endif

  private:
    JSString() MOZ_DELETE;
    JSString(const JSString &other) MOZ_DELETE;
    void operator=(const JSString &other) MOZ_DELETE;
};

class JSRope : public JSString
{
    enum UsingBarrier { WithIncrementalBarrier, NoBarrier };
    template<UsingBarrier b>
    JSFlatString *flattenInternal(JSContext *cx);

    friend class JSString;
    JSFlatString *flatten(JSContext *cx);

    void init(JSString *left, JSString *right, size_t length);

  public:
    static inline JSRope *new_(JSContext *cx, js::HandleString left,
                               js::HandleString right, size_t length);

    inline JSString *leftChild() const {
        JS_ASSERT(isRope());
        return d.u1.left;
    }

    inline JSString *rightChild() const {
        JS_ASSERT(isRope());
        return d.s.u2.right;
    }

    inline void markChildren(JSTracer *trc);
};

JS_STATIC_ASSERT(sizeof(JSRope) == sizeof(JSString));

class JSLinearString : public JSString
{
    friend class JSString;

    
    JSLinearString *ensureLinear(JSContext *cx) MOZ_DELETE;
    bool isLinear() const MOZ_DELETE;
    JSLinearString &asLinear() const MOZ_DELETE;

  public:
    JS_ALWAYS_INLINE
    const jschar *chars() const {
        JS_ASSERT(JSString::isLinear());
        return d.u1.chars;
    }
};

JS_STATIC_ASSERT(sizeof(JSLinearString) == sizeof(JSString));

class JSDependentString : public JSLinearString
{
    friend class JSString;
    JSFixedString *undepend(JSContext *cx);

    void init(JSLinearString *base, const jschar *chars, size_t length);

    
    bool isDependent() const MOZ_DELETE;
    JSDependentString &asDependent() const MOZ_DELETE;

  public:
    static inline JSLinearString *new_(JSContext *cx, JSLinearString *base,
                                       const jschar *chars, size_t length);
};

JS_STATIC_ASSERT(sizeof(JSDependentString) == sizeof(JSString));

class JSFlatString : public JSLinearString
{
    
    JSFlatString *ensureFlat(JSContext *cx) MOZ_DELETE;
    bool isFlat() const MOZ_DELETE;
    JSFlatString &asFlat() const MOZ_DELETE;

    bool isIndexSlow(uint32_t *indexp) const;

  public:
    JS_ALWAYS_INLINE
    const jschar *charsZ() const {
        JS_ASSERT(JSString::isFlat());
        return chars();
    }

    





    inline bool isIndex(uint32_t *indexp) const {
        const jschar *s = chars();
        return JS7_ISDEC(*s) && isIndexSlow(indexp);
    }

    




    inline js::PropertyName *toPropertyName(JSContext *cx);

    inline void finalize(js::FreeOp *fop);
};

JS_STATIC_ASSERT(sizeof(JSFlatString) == sizeof(JSString));

class JSExtensibleString : public JSFlatString
{
    
    bool isExtensible() const MOZ_DELETE;
    JSExtensibleString &asExtensible() const MOZ_DELETE;

  public:
    JS_ALWAYS_INLINE
    size_t capacity() const {
        JS_ASSERT(JSString::isExtensible());
        return d.s.u2.capacity;
    }
};

JS_STATIC_ASSERT(sizeof(JSExtensibleString) == sizeof(JSString));

class JSFixedString : public JSFlatString
{
    void init(const jschar *chars, size_t length);

    
    JSFlatString *ensureFixed(JSContext *cx) MOZ_DELETE;
    bool isFixed() const MOZ_DELETE;
    JSFixedString &asFixed() const MOZ_DELETE;

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

  protected: 
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

    

    JS_ALWAYS_INLINE void finalize(js::FreeOp *fop);
};

JS_STATIC_ASSERT(sizeof(JSShortString) == 2 * sizeof(JSString));

class JSExternalString : public JSFixedString
{
    void init(const jschar *chars, size_t length, const JSStringFinalizer *fin);

    
    bool isExternal() const MOZ_DELETE;
    JSExternalString &asExternal() const MOZ_DELETE;

  public:
    static inline JSExternalString *new_(JSContext *cx, const jschar *chars, size_t length,
                                         const JSStringFinalizer *fin);

    const JSStringFinalizer *externalFinalizer() const {
        JS_ASSERT(JSString::isExternal());
        return d.s.u2.externalFinalizer;
    }

    

    inline void finalize(js::FreeOp *fop);
};

JS_STATIC_ASSERT(sizeof(JSExternalString) == sizeof(JSString));

class JSUndependedString : public JSFixedString
{
    




};

JS_STATIC_ASSERT(sizeof(JSUndependedString) == sizeof(JSString));

class JSAtom : public JSFixedString
{
    
    bool isAtom() const MOZ_DELETE;
    JSAtom &asAtom() const MOZ_DELETE;

  public:
    
    inline js::PropertyName *asPropertyName();

    inline void finalize(js::FreeOp *fop);

#ifdef DEBUG
    void dump();
#endif
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

namespace js {

class StaticStrings
{
  private:
    
    static const size_t SMALL_CHAR_LIMIT    = 128U;
    static const size_t NUM_SMALL_CHARS     = 64U;

    static const size_t INT_STATIC_LIMIT    = 256U;

    JSAtom *length2StaticTable[NUM_SMALL_CHARS * NUM_SMALL_CHARS];
    JSAtom *intStaticTable[INT_STATIC_LIMIT];

  public:
    
    static const size_t UNIT_STATIC_LIMIT   = 256U;
    JSAtom *unitStaticTable[UNIT_STATIC_LIMIT];

    StaticStrings() {
        PodArrayZero(unitStaticTable);
        PodArrayZero(length2StaticTable);
        PodArrayZero(intStaticTable);
    }

    bool init(JSContext *cx);
    void trace(JSTracer *trc);

    static inline bool hasUint(uint32_t u);
    inline JSAtom *getUint(uint32_t u);

    static inline bool hasInt(int32_t i);
    inline JSAtom *getInt(int32_t i);

    static inline bool hasUnit(jschar c);
    JSAtom *getUnit(jschar c);

    
    inline JSLinearString *getUnitStringForElement(JSContext *cx, JSString *str, size_t index);

    static bool isStatic(JSAtom *atom);

    
    inline JSAtom *lookup(const jschar *chars, size_t length);

  private:
    typedef uint8_t SmallChar;
    static const SmallChar INVALID_SMALL_CHAR = -1;

    static inline bool fitsInSmallChar(jschar c);

    static const SmallChar toSmallChar[];

    JSAtom *getLength2(jschar c1, jschar c2);
    JSAtom *getLength2(uint32_t u);
};
















class PropertyName : public JSAtom
{};

JS_STATIC_ASSERT(sizeof(PropertyName) == sizeof(JSString));

static JS_ALWAYS_INLINE jsid
NameToId(PropertyName *name)
{
    return NON_INTEGER_ATOM_TO_JSID(name);
}

typedef HeapPtr<JSAtom> HeapPtrAtom;

class AutoNameVector : public AutoVectorRooter<PropertyName *>
{
    typedef AutoVectorRooter<PropertyName *> BaseType;
  public:
    explicit AutoNameVector(JSContext *cx
                            JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<PropertyName *>(cx, NAMEVECTOR)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    HandlePropertyName operator[](size_t i) const {
        return HandlePropertyName::fromMarkedLocation(&BaseType::operator[](i));
    }

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

} 



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
    if (isExtensible())
        d.lengthAndFlags = buildLengthAndFlags(length(), FIXED_FLAGS);
    return &asFixed();
}

inline JSLinearString *
JSString::base() const
{
    JS_ASSERT(hasBase());
    JS_ASSERT(!d.s.u2.base->isInline());
    return d.s.u2.base;
}

inline js::PropertyName *
JSAtom::asPropertyName()
{
#ifdef DEBUG
    uint32_t dummy;
    JS_ASSERT(!isIndex(&dummy));
#endif
    return static_cast<js::PropertyName *>(this);
}

#endif

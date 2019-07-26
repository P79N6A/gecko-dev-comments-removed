





#ifndef vm_String_h
#define vm_String_h

#include "mozilla/MemoryReporting.h"
#include "mozilla/PodOperations.h"

#include "jsapi.h"
#include "jsfriendapi.h"
#include "jsstr.h"

#include "gc/Barrier.h"
#include "gc/Heap.h"
#include "gc/Marking.h"
#include "gc/Rooting.h"
#include "js/CharacterEncoding.h"
#include "js/RootingAPI.h"

class JSDependentString;
class JSExtensibleString;
class JSExternalString;
class JSInlineString;
class JSStableString;
class JSRope;

namespace js {

class StaticStrings;
class PropertyName;


static const size_t UINT32_CHAR_BUFFER_LENGTH = sizeof("4294967295") - 1;

} 





























































































class JSString : public js::gc::BarrieredCell<JSString>
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

    




    static inline bool validateLength(js::ThreadSafeContext *maybecx, size_t length);

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

    




    inline const jschar *getChars(js::ExclusiveContext *cx);
    inline const jschar *getCharsZ(js::ExclusiveContext *cx);
    inline bool getChar(js::ExclusiveContext *cx, size_t index, jschar *code);

    







    bool hasPureChars() const { return isLinear(); }
    bool hasPureCharsZ() const { return isFlat(); }
    inline const jschar *pureChars() const;
    inline const jschar *pureCharsZ() const;
    inline bool copyNonPureChars(js::ThreadSafeContext *cx,
                                 js::ScopedJSFreePtr<jschar> &out) const;
    inline bool copyNonPureCharsZ(js::ThreadSafeContext *cx,
                                  js::ScopedJSFreePtr<jschar> &out) const;

    

    inline JSLinearString *ensureLinear(js::ExclusiveContext *cx);
    inline JSFlatString *ensureFlat(js::ExclusiveContext *cx);
    inline JSStableString *ensureStable(js::ExclusiveContext *cx);

    static bool ensureLinear(js::ExclusiveContext *cx, JSString *str) {
        return str->ensureLinear(cx) != nullptr;
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

    JS_ALWAYS_INLINE
    bool isInline() const {
        return isFlat() && !isExtensible() && (d.u1.chars == d.inlineStorage);
    }

    JS_ALWAYS_INLINE
    JSInlineString &asInline() const {
        JS_ASSERT(isInline());
        return *(JSInlineString *)this;
    }

    bool isShort() const;

    JS_ALWAYS_INLINE
    JSStableString &asStable() const {
        JS_ASSERT(!isInline());
        return *(JSStableString *)this;
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

    

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf);

    

    static size_t offsetOfLengthAndFlags() {
        return offsetof(JSString, d.lengthAndFlags);
    }

    static size_t offsetOfChars() {
        return offsetof(JSString, d.u1.chars);
    }

    js::gc::AllocKind getAllocKind() const { return tenuredGetAllocKind(); }

    static inline js::ThingRootKind rootKind() { return js::THING_ROOT_STRING; }

#ifdef DEBUG
    void dump();
    static void dumpChars(const jschar *s, size_t len);
    bool equals(const char *s);
#endif

  private:
    JSString() MOZ_DELETE;
    JSString(const JSString &other) MOZ_DELETE;
    void operator=(const JSString &other) MOZ_DELETE;
};

class JSRope : public JSString
{
    bool copyNonPureCharsInternal(js::ThreadSafeContext *cx,
                                  js::ScopedJSFreePtr<jschar> &out,
                                  bool nullTerminate) const;
    bool copyNonPureChars(js::ThreadSafeContext *cx, js::ScopedJSFreePtr<jschar> &out) const;
    bool copyNonPureCharsZ(js::ThreadSafeContext *cx, js::ScopedJSFreePtr<jschar> &out) const;

    enum UsingBarrier { WithIncrementalBarrier, NoBarrier };
    template<UsingBarrier b>
    JSFlatString *flattenInternal(js::ExclusiveContext *cx);

    friend class JSString;
    JSFlatString *flatten(js::ExclusiveContext *cx);

    void init(js::ThreadSafeContext *cx, JSString *left, JSString *right, size_t length);

  public:
    template <js::AllowGC allowGC>
    static inline JSRope *new_(js::ThreadSafeContext *cx,
                               typename js::MaybeRooted<JSString*, allowGC>::HandleType left,
                               typename js::MaybeRooted<JSString*, allowGC>::HandleType right,
                               size_t length);

    inline JSString *leftChild() const {
        JS_ASSERT(isRope());
        return d.u1.left;
    }

    inline JSString *rightChild() const {
        JS_ASSERT(isRope());
        return d.s.u2.right;
    }

    inline void markChildren(JSTracer *trc);

    inline static size_t offsetOfLeft() {
        return offsetof(JSRope, d.u1.left);
    }
    inline static size_t offsetOfRight() {
        return offsetof(JSRope, d.s.u2.right);
    }
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

    JS::TwoByteChars range() const {
        JS_ASSERT(JSString::isLinear());
        return JS::TwoByteChars(d.u1.chars, length());
    }
};

JS_STATIC_ASSERT(sizeof(JSLinearString) == sizeof(JSString));

class JSDependentString : public JSLinearString
{
    bool copyNonPureCharsZ(js::ThreadSafeContext *cx, js::ScopedJSFreePtr<jschar> &out) const;

    friend class JSString;
    JSFlatString *undepend(js::ExclusiveContext *cx);

    void init(js::ThreadSafeContext *cx, JSLinearString *base, const jschar *chars,
              size_t length);

    
    bool isDependent() const MOZ_DELETE;
    JSDependentString &asDependent() const MOZ_DELETE;

  public:
    static inline JSLinearString *new_(js::ExclusiveContext *cx, JSLinearString *base,
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

    



    JS_ALWAYS_INLINE JSAtom *morphAtomizedStringIntoAtom() {
        d.lengthAndFlags = buildLengthAndFlags(length(), ATOM_BIT);
        return &asAtom();
    }

    inline void finalize(js::FreeOp *fop);
};

JS_STATIC_ASSERT(sizeof(JSFlatString) == sizeof(JSString));

class JSStableString : public JSFlatString
{
    void init(const jschar *chars, size_t length);

  public:
    template <js::AllowGC allowGC>
    static inline JSStableString *new_(js::ThreadSafeContext *cx,
                                       const jschar *chars, size_t length);

    JS_ALWAYS_INLINE
    JS::StableCharPtr chars() const {
        JS_ASSERT(!JSString::isInline());
        return JS::StableCharPtr(d.u1.chars, length());
    }

    JS_ALWAYS_INLINE
    JS::StableTwoByteChars range() const {
        JS_ASSERT(!JSString::isInline());
        return JS::StableTwoByteChars(d.u1.chars, length());
    }
};

JS_STATIC_ASSERT(sizeof(JSStableString) == sizeof(JSString));

#if !(defined(JSGC_ROOT_ANALYSIS) || defined(JSGC_USE_EXACT_ROOTING))
namespace JS {
















template <>
class Rooted<JSStableString *>
{
  public:
    Rooted(JSContext *cx, JSStableString *initial = nullptr
           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : rooter(cx, initial)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    operator JSStableString *() const { return get(); }
    JSStableString * operator ->() const { return get(); }
    JSStableString ** address() { return reinterpret_cast<JSStableString **>(rooter.addr()); }
    JSStableString * const * address() const {
        return reinterpret_cast<JSStableString * const *>(rooter.addr());
    }
    JSStableString * get() const { return static_cast<JSStableString *>(rooter.string()); }

    Rooted & operator =(JSStableString *value)
    {
        JS_ASSERT(!js::GCMethods<JSStableString *>::poisoned(value));
        rooter.setString(value);
        return *this;
    }

    Rooted & operator =(const Rooted &value)
    {
        rooter.setString(value.get());
        return *this;
    }

  private:
    JS::AutoStringRooter rooter;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
    Rooted(const Rooted &) MOZ_DELETE;
};
}
#endif

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

class JSInlineString : public JSFlatString
{
    static const size_t MAX_INLINE_LENGTH = NUM_INLINE_CHARS - 1;

  public:
    template <js::AllowGC allowGC>
    static inline JSInlineString *new_(js::ThreadSafeContext *cx);

    inline jschar *init(size_t length);

    JSStableString *uninline(js::ExclusiveContext *cx);

    inline void resetLength(size_t length);

    static bool lengthFits(size_t length) {
        return length <= MAX_INLINE_LENGTH;
    }

    static size_t offsetOfInlineStorage() {
        return offsetof(JSInlineString, d.inlineStorage);
    }
};

JS_STATIC_ASSERT(sizeof(JSInlineString) == sizeof(JSString));

class JSShortString : public JSInlineString
{
    
    static const size_t INLINE_EXTENSION_CHARS = sizeof(JSString::Data) / sizeof(jschar);

    static void staticAsserts() {
        JS_STATIC_ASSERT(INLINE_EXTENSION_CHARS % js::gc::CellSize == 0);
        JS_STATIC_ASSERT(MAX_SHORT_LENGTH + 1 ==
                         (sizeof(JSShortString) -
                          offsetof(JSShortString, d.inlineStorage)) / sizeof(jschar));
    }

  protected: 
    jschar inlineStorageExtension[INLINE_EXTENSION_CHARS];

  public:
    template <js::AllowGC allowGC>
    static inline JSShortString *new_(js::ThreadSafeContext *cx);

    static const size_t MAX_SHORT_LENGTH = JSString::NUM_INLINE_CHARS +
                                           INLINE_EXTENSION_CHARS
                                           -1 ;

    static bool lengthFits(size_t length) {
        return length <= MAX_SHORT_LENGTH;
    }

    

    JS_ALWAYS_INLINE void finalize(js::FreeOp *fop);
};

JS_STATIC_ASSERT(sizeof(JSShortString) == 2 * sizeof(JSString));

class JSExternalString : public JSFlatString
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

class JSUndependedString : public JSFlatString
{
    




};

JS_STATIC_ASSERT(sizeof(JSUndependedString) == sizeof(JSString));

class JSAtom : public JSFlatString
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

namespace js {












class ScopedThreadSafeStringInspector
{
  private:
    JSString *str_;
    ScopedJSFreePtr<jschar> scopedChars_;
    const jschar *chars_;

  public:
    ScopedThreadSafeStringInspector(JSString *str)
      : str_(str),
        chars_(nullptr)
    { }

    bool ensureChars(ThreadSafeContext *cx);

    const jschar *chars() {
        JS_ASSERT(chars_);
        return chars_;
    }

    JS::TwoByteChars range() {
        JS_ASSERT(chars_);
        return JS::TwoByteChars(chars_, str_->length());
    }
};

class StaticStrings
{
  private:
    
    static const size_t SMALL_CHAR_LIMIT    = 128U;
    static const size_t NUM_SMALL_CHARS     = 64U;

    JSAtom *length2StaticTable[NUM_SMALL_CHARS * NUM_SMALL_CHARS];

    void clear() {
        mozilla::PodArrayZero(unitStaticTable);
        mozilla::PodArrayZero(length2StaticTable);
        mozilla::PodArrayZero(intStaticTable);
    }

  public:
    
    static const size_t UNIT_STATIC_LIMIT   = 256U;
    JSAtom *unitStaticTable[UNIT_STATIC_LIMIT];

    static const size_t INT_STATIC_LIMIT    = 256U;
    JSAtom *intStaticTable[INT_STATIC_LIMIT];

    StaticStrings() {
        clear();
    }

    bool init(JSContext *cx);
    void trace(JSTracer *trc);
    void finish() {
        clear();
    }

    static bool hasUint(uint32_t u) { return u < INT_STATIC_LIMIT; }

    JSAtom *getUint(uint32_t u) {
        JS_ASSERT(hasUint(u));
        return intStaticTable[u];
    }

    static bool hasInt(int32_t i) {
        return uint32_t(i) < INT_STATIC_LIMIT;
    }

    JSAtom *getInt(int32_t i) {
        JS_ASSERT(hasInt(i));
        return getUint(uint32_t(i));
    }

    static bool hasUnit(jschar c) { return c < UNIT_STATIC_LIMIT; }

    JSAtom *getUnit(jschar c) {
        JS_ASSERT(hasUnit(c));
        return unitStaticTable[c];
    }

    
    inline JSLinearString *getUnitStringForElement(JSContext *cx, JSString *str, size_t index);

    static bool isStatic(JSAtom *atom);

    
    JSAtom *lookup(const jschar *chars, size_t length) {
        switch (length) {
          case 1:
            if (chars[0] < UNIT_STATIC_LIMIT)
                return getUnit(chars[0]);
            return nullptr;
          case 2:
            if (fitsInSmallChar(chars[0]) && fitsInSmallChar(chars[1]))
                return getLength2(chars[0], chars[1]);
            return nullptr;
          case 3:
            





            JS_STATIC_ASSERT(INT_STATIC_LIMIT <= 999);
            if ('1' <= chars[0] && chars[0] <= '9' &&
                '0' <= chars[1] && chars[1] <= '9' &&
                '0' <= chars[2] && chars[2] <= '9') {
                int i = (chars[0] - '0') * 100 +
                          (chars[1] - '0') * 10 +
                          (chars[2] - '0');

                if (unsigned(i) < INT_STATIC_LIMIT)
                    return getInt(i);
            }
            return nullptr;
        }

        return nullptr;
    }

  private:
    typedef uint8_t SmallChar;
    static const SmallChar INVALID_SMALL_CHAR = -1;

    static bool fitsInSmallChar(jschar c) {
        return c < SMALL_CHAR_LIMIT && toSmallChar[c] != INVALID_SMALL_CHAR;
    }

    static const SmallChar toSmallChar[];

    JSAtom *getLength2(jschar c1, jschar c2);
    JSAtom *getLength2(uint32_t u) {
        JS_ASSERT(u < 100);
        return getLength2('0' + u / 10, '0' + u % 10);
    }
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
                            MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<PropertyName *>(cx, NAMEVECTOR)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    HandlePropertyName operator[](size_t i) const {
        return HandlePropertyName::fromMarkedLocation(&BaseType::operator[](i));
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

} 



JS_ALWAYS_INLINE const jschar *
JSString::getChars(js::ExclusiveContext *cx)
{
    if (JSLinearString *str = ensureLinear(cx))
        return str->chars();
    return nullptr;
}

JS_ALWAYS_INLINE bool
JSString::getChar(js::ExclusiveContext *cx, size_t index, jschar *code)
{
    JS_ASSERT(index < length());

    








    const jschar *chars;
    if (isRope()) {
        JSRope *rope = &asRope();
        if (uint32_t(index) < rope->leftChild()->length()) {
            chars = rope->leftChild()->getChars(cx);
        } else {
            chars = rope->rightChild()->getChars(cx);
            index -= rope->leftChild()->length();
        }
    } else {
        chars = getChars(cx);
    }

    if (!chars)
        return false;

    *code = chars[index];
    return true;
}

JS_ALWAYS_INLINE const jschar *
JSString::getCharsZ(js::ExclusiveContext *cx)
{
    if (JSFlatString *str = ensureFlat(cx))
        return str->chars();
    return nullptr;
}

JS_ALWAYS_INLINE const jschar *
JSString::pureChars() const
{
    JS_ASSERT(hasPureChars());
    return asLinear().chars();
}

JS_ALWAYS_INLINE const jschar *
JSString::pureCharsZ() const
{
    JS_ASSERT(hasPureCharsZ());
    return asFlat().charsZ();
}

JS_ALWAYS_INLINE bool
JSString::copyNonPureChars(js::ThreadSafeContext *cx, js::ScopedJSFreePtr<jschar> &out) const
{
    JS_ASSERT(!hasPureChars());
    return asRope().copyNonPureChars(cx, out);
}

JS_ALWAYS_INLINE bool
JSString::copyNonPureCharsZ(js::ThreadSafeContext *cx, js::ScopedJSFreePtr<jschar> &out) const
{
    JS_ASSERT(!hasPureChars());
    if (isDependent())
        return asDependent().copyNonPureCharsZ(cx, out);
    return asRope().copyNonPureCharsZ(cx, out);
}

JS_ALWAYS_INLINE JSLinearString *
JSString::ensureLinear(js::ExclusiveContext *cx)
{
    return isLinear()
           ? &asLinear()
           : asRope().flatten(cx);
}

JS_ALWAYS_INLINE JSFlatString *
JSString::ensureFlat(js::ExclusiveContext *cx)
{
    return isFlat()
           ? &asFlat()
           : isDependent()
             ? asDependent().undepend(cx)
             : asRope().flatten(cx);
}

JS_ALWAYS_INLINE JSStableString *
JSString::ensureStable(js::ExclusiveContext *maybecx)
{
    if (isRope()) {
        JSFlatString *flat = asRope().flatten(maybecx);
        if (!flat)
            return nullptr;
        JS_ASSERT(!flat->isInline());
        return &flat->asStable();
    }

    if (isDependent()) {
        JSFlatString *flat = asDependent().undepend(maybecx);
        if (!flat)
            return nullptr;
        return &flat->asStable();
    }

    if (!isInline())
        return &asStable();

    JS_ASSERT(isInline());
    return asInline().uninline(maybecx);
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

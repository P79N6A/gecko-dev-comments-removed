





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
class JSRope;

namespace js {

class StaticStrings;
class PropertyName;


static const size_t UINT32_CHAR_BUFFER_LENGTH = sizeof("4294967295") - 1;

} 

































































































class JSString : public js::gc::BarrieredCell<JSString>
{
  protected:
    static const size_t NUM_INLINE_CHARS_LATIN1 = 2 * sizeof(void *) / sizeof(char);
    static const size_t NUM_INLINE_CHARS_TWO_BYTE = 2 * sizeof(void *) / sizeof(jschar);

    
    struct Data
    {
        union {
            struct {
                uint32_t           flags;               
                uint32_t           length;              
            };
            uintptr_t              flattenData;         
        } u1;
        union {
            union {
                
                char               inlineStorageLatin1[NUM_INLINE_CHARS_LATIN1];
                jschar             inlineStorageTwoByte[NUM_INLINE_CHARS_TWO_BYTE];
            };
            struct {
                union {
                    const char     *nonInlineCharsLatin1; 
                    const jschar   *nonInlineCharsTwoByte;
                    JSString       *left;               
                } u2;
                union {
                    JSLinearString *base;               
                    JSString       *right;              
                    size_t         capacity;            
                    const JSStringFinalizer *externalFinalizer;
                } u3;
            } s;
        };
    } d;

  public:
    

    
























































    static const uint32_t FLAT_BIT              = JS_BIT(0);
    static const uint32_t HAS_BASE_BIT          = JS_BIT(1);
    static const uint32_t INLINE_CHARS_BIT      = JS_BIT(2);
    static const uint32_t ATOM_BIT              = JS_BIT(3);

    static const uint32_t ROPE_FLAGS            = 0;
    static const uint32_t DEPENDENT_FLAGS       = HAS_BASE_BIT;
    static const uint32_t UNDEPENDED_FLAGS      = FLAT_BIT | HAS_BASE_BIT;
    static const uint32_t EXTENSIBLE_FLAGS      = FLAT_BIT | JS_BIT(4);
    static const uint32_t EXTERNAL_FLAGS        = FLAT_BIT | JS_BIT(5);

    static const uint32_t FAT_INLINE_MASK       = INLINE_CHARS_BIT | JS_BIT(4);
    static const uint32_t PERMANENT_ATOM_MASK   = ATOM_BIT | JS_BIT(5);

    
    static const uint32_t INIT_INLINE_FLAGS     = FLAT_BIT | INLINE_CHARS_BIT;
    static const uint32_t INIT_FAT_INLINE_FLAGS = FLAT_BIT | FAT_INLINE_MASK;

    static const uint32_t TYPE_FLAGS_MASK       = JS_BIT(6) - 1;

    static const uint32_t LATIN1_CHARS_BIT      = JS_BIT(6);

    static const uint32_t MAX_LENGTH            = JS_BIT(28) - 1;

    




    static inline bool validateLength(js::ThreadSafeContext *maybecx, size_t length);

    static void staticAsserts() {
        static_assert(JSString::MAX_LENGTH < UINT32_MAX, "Length must fit in 32 bits");
        static_assert(sizeof(JSString) ==
                      (offsetof(JSString, d.inlineStorageLatin1) +
                       NUM_INLINE_CHARS_LATIN1 * sizeof(char)),
                      "Inline chars must fit in a JSString");
        static_assert(sizeof(JSString) ==
                      (offsetof(JSString, d.inlineStorageTwoByte) +
                       NUM_INLINE_CHARS_TWO_BYTE * sizeof(jschar)),
                      "Inline chars must fit in a JSString");

        
        using js::shadow::Atom;
        static_assert(offsetof(JSString, d.u1.length) == offsetof(Atom, length),
                      "shadow::Atom length offset must match JSString");
        static_assert(offsetof(JSString, d.u1.flags) == offsetof(Atom, flags),
                      "shadow::Atom flags offset must match JSString");
        static_assert(offsetof(JSString, d.s.u2.nonInlineCharsLatin1) == offsetof(Atom, nonInlineCharsLatin1),
                      "shadow::Atom nonInlineChars offset must match JSString");
        static_assert(offsetof(JSString, d.s.u2.nonInlineCharsTwoByte) == offsetof(Atom, nonInlineCharsTwoByte),
                      "shadow::Atom nonInlineChars offset must match JSString");
        static_assert(offsetof(JSString, d.inlineStorageLatin1) == offsetof(Atom, inlineStorageLatin1),
                      "shadow::Atom inlineStorage offset must match JSString");
        static_assert(offsetof(JSString, d.inlineStorageTwoByte) == offsetof(Atom, inlineStorageTwoByte),
                      "shadow::Atom inlineStorage offset must match JSString");
        static_assert(INLINE_CHARS_BIT == Atom::INLINE_CHARS_BIT,
                      "shadow::Atom::INLINE_CHARS_BIT must match JSString::INLINE_CHARS_BIT");
        static_assert(LATIN1_CHARS_BIT == Atom::LATIN1_CHARS_BIT,
                      "shadow::Atom::LATIN1_CHARS_BIT must match JSString::LATIN1_CHARS_BIT");
    }

    
    friend class JSRope;

  protected:
    template <typename CharT>
    MOZ_ALWAYS_INLINE
    void setNonInlineChars(const CharT *chars);

  public:
    

    MOZ_ALWAYS_INLINE
    size_t length() const {
        return d.u1.length;
    }

    MOZ_ALWAYS_INLINE
    bool empty() const {
        return d.u1.length == 0;
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

    
    bool hasLatin1Chars() const {
        return d.u1.flags & LATIN1_CHARS_BIT;
    }
    bool hasTwoByteChars() const {
        return !(d.u1.flags & LATIN1_CHARS_BIT);
    }

    

    inline JSLinearString *ensureLinear(js::ExclusiveContext *cx);
    inline JSFlatString *ensureFlat(js::ExclusiveContext *cx);

    static bool ensureLinear(js::ExclusiveContext *cx, JSString *str) {
        return str->ensureLinear(cx) != nullptr;
    }

    

    MOZ_ALWAYS_INLINE
    bool isRope() const {
        return (d.u1.flags & TYPE_FLAGS_MASK) == ROPE_FLAGS;
    }

    MOZ_ALWAYS_INLINE
    JSRope &asRope() const {
        JS_ASSERT(isRope());
        return *(JSRope *)this;
    }

    MOZ_ALWAYS_INLINE
    bool isLinear() const {
        return !isRope();
    }

    MOZ_ALWAYS_INLINE
    JSLinearString &asLinear() const {
        JS_ASSERT(JSString::isLinear());
        return *(JSLinearString *)this;
    }

    MOZ_ALWAYS_INLINE
    bool isDependent() const {
        return (d.u1.flags & TYPE_FLAGS_MASK) == DEPENDENT_FLAGS;
    }

    MOZ_ALWAYS_INLINE
    JSDependentString &asDependent() const {
        JS_ASSERT(isDependent());
        return *(JSDependentString *)this;
    }

    MOZ_ALWAYS_INLINE
    bool isFlat() const {
        return d.u1.flags & FLAT_BIT;
    }

    MOZ_ALWAYS_INLINE
    JSFlatString &asFlat() const {
        JS_ASSERT(isFlat());
        return *(JSFlatString *)this;
    }

    MOZ_ALWAYS_INLINE
    bool isExtensible() const {
        return (d.u1.flags & TYPE_FLAGS_MASK) == EXTENSIBLE_FLAGS;
    }

    MOZ_ALWAYS_INLINE
    JSExtensibleString &asExtensible() const {
        JS_ASSERT(isExtensible());
        return *(JSExtensibleString *)this;
    }

    MOZ_ALWAYS_INLINE
    bool isInline() const {
        return d.u1.flags & INLINE_CHARS_BIT;
    }

    MOZ_ALWAYS_INLINE
    JSInlineString &asInline() const {
        JS_ASSERT(isInline());
        return *(JSInlineString *)this;
    }

    MOZ_ALWAYS_INLINE
    bool isFatInline() const {
        return (d.u1.flags & FAT_INLINE_MASK) == FAT_INLINE_MASK;
    }

    
    bool isExternal() const {
        return (d.u1.flags & TYPE_FLAGS_MASK) == EXTERNAL_FLAGS;
    }

    MOZ_ALWAYS_INLINE
    JSExternalString &asExternal() const {
        JS_ASSERT(isExternal());
        return *(JSExternalString *)this;
    }

    MOZ_ALWAYS_INLINE
    bool isUndepended() const {
        return (d.u1.flags & TYPE_FLAGS_MASK) == UNDEPENDED_FLAGS;
    }

    MOZ_ALWAYS_INLINE
    bool isAtom() const {
        return d.u1.flags & ATOM_BIT;
    }

    MOZ_ALWAYS_INLINE
    bool isPermanentAtom() const {
        return (d.u1.flags & PERMANENT_ATOM_MASK) == PERMANENT_ATOM_MASK;
    }

    MOZ_ALWAYS_INLINE
    JSAtom &asAtom() const {
        JS_ASSERT(isAtom());
        return *(JSAtom *)this;
    }

    

    inline bool hasBase() const {
        return d.u1.flags & HAS_BASE_BIT;
    }

    inline JSLinearString *base() const;

    inline void markBase(JSTracer *trc);

    

    inline void finalize(js::FreeOp *fop);

    

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf);

    

    static size_t offsetOfLength() {
        return offsetof(JSString, d.u1.length);
    }
    static size_t offsetOfFlags() {
        return offsetof(JSString, d.u1.flags);
    }

    static size_t offsetOfNonInlineChars() {
        return offsetof(JSString, d.s.u2.nonInlineCharsTwoByte);
    }

    js::gc::AllocKind getAllocKind() const { return tenuredGetAllocKind(); }

    static inline js::ThingRootKind rootKind() { return js::THING_ROOT_STRING; }

#ifdef DEBUG
    void dump();
    static void dumpChars(const jschar *s, size_t len);
    bool equals(const char *s);
#endif

    static MOZ_ALWAYS_INLINE void readBarrier(JSString *thing) {
#ifdef JSGC_INCREMENTAL
        if (thing->isPermanentAtom())
            return;

        js::gc::BarrieredCell<JSString>::readBarrier(thing);
#endif
    }

    static MOZ_ALWAYS_INLINE void writeBarrierPre(JSString *thing) {
#ifdef JSGC_INCREMENTAL
        if (isNullLike(thing) || thing->isPermanentAtom())
            return;

        js::gc::BarrieredCell<JSString>::writeBarrierPre(thing);
#endif
    }

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

    template<UsingBarrier b, typename CharT>
    JSFlatString *flattenInternal(js::ExclusiveContext *cx);

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
        return d.s.u2.left;
    }

    inline JSString *rightChild() const {
        JS_ASSERT(isRope());
        return d.s.u3.right;
    }

    inline void markChildren(JSTracer *trc);

    inline static size_t offsetOfLeft() {
        return offsetof(JSRope, d.s.u2.left);
    }
    inline static size_t offsetOfRight() {
        return offsetof(JSRope, d.s.u3.right);
    }
};

JS_STATIC_ASSERT(sizeof(JSRope) == sizeof(JSString));

class JSLinearString : public JSString
{
    friend class JSString;

    
    JSLinearString *ensureLinear(JSContext *cx) MOZ_DELETE;
    bool isLinear() const MOZ_DELETE;
    JSLinearString &asLinear() const MOZ_DELETE;

  protected:
    
    MOZ_ALWAYS_INLINE
    void *nonInlineCharsRaw() const {
        JS_ASSERT(!isInline());
        static_assert(offsetof(JSLinearString, d.s.u2.nonInlineCharsTwoByte) ==
                      offsetof(JSLinearString, d.s.u2.nonInlineCharsLatin1),
                      "nonInlineCharsTwoByte and nonInlineCharsLatin1 must have same offset");
        return (void *)d.s.u2.nonInlineCharsTwoByte;
    }

  public:
    MOZ_ALWAYS_INLINE
    const jschar *nonInlineChars() const {
        JS_ASSERT(!isInline());
        JS_ASSERT(hasTwoByteChars());
        return d.s.u2.nonInlineCharsTwoByte;
    }

    template<typename CharT>
    MOZ_ALWAYS_INLINE
    const CharT *nonInlineChars(const JS::AutoCheckCannotGC &nogc) const;

    MOZ_ALWAYS_INLINE
    const char *nonInlineLatin1Chars(const JS::AutoCheckCannotGC &nogc) const {
        JS_ASSERT(!isInline());
        JS_ASSERT(hasLatin1Chars());
        return d.s.u2.nonInlineCharsLatin1;
    }

    MOZ_ALWAYS_INLINE
    const jschar *nonInlineTwoByteChars(const JS::AutoCheckCannotGC &nogc) const {
        JS_ASSERT(!isInline());
        JS_ASSERT(hasTwoByteChars());
        return d.s.u2.nonInlineCharsTwoByte;
    }

    MOZ_ALWAYS_INLINE
    const jschar *chars() const;

    MOZ_ALWAYS_INLINE
    const char *latin1Chars(const JS::AutoCheckCannotGC &nogc) const;

    MOZ_ALWAYS_INLINE
    const jschar *twoByteChars(const JS::AutoCheckCannotGC &nogc) const;

    JS::TwoByteChars range() const {
        JS_ASSERT(JSString::isLinear());
        return JS::TwoByteChars(chars(), length());
    }

    
    void debugUnsafeConvertToLatin1();
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

    
    const jschar *chars() const MOZ_DELETE;

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

    void init(const jschar *chars, size_t length);

  public:
    template <js::AllowGC allowGC>
    static inline JSFlatString *new_(js::ThreadSafeContext *cx,
                                     const jschar *chars, size_t length);

    MOZ_ALWAYS_INLINE
    const jschar *charsZ() const {
        JS_ASSERT(JSString::isFlat());
        return chars();
    }

    





    inline bool isIndex(uint32_t *indexp) const {
        const jschar *s = chars();
        return JS7_ISDEC(*s) && isIndexSlow(indexp);
    }

    




    inline js::PropertyName *toPropertyName(JSContext *cx);

    



    MOZ_ALWAYS_INLINE JSAtom *morphAtomizedStringIntoAtom() {
        d.u1.flags |= ATOM_BIT;
        return &asAtom();
    }
    MOZ_ALWAYS_INLINE JSAtom *morphAtomizedStringIntoPermanentAtom() {
        d.u1.flags |= PERMANENT_ATOM_MASK;
        return &asAtom();
    }

    inline void finalize(js::FreeOp *fop);
};

JS_STATIC_ASSERT(sizeof(JSFlatString) == sizeof(JSString));

class JSExtensibleString : public JSFlatString
{
    
    bool isExtensible() const MOZ_DELETE;
    JSExtensibleString &asExtensible() const MOZ_DELETE;

    
    const jschar *chars() const MOZ_DELETE;

  public:
    MOZ_ALWAYS_INLINE
    size_t capacity() const {
        JS_ASSERT(JSString::isExtensible());
        return d.s.u3.capacity;
    }
};

JS_STATIC_ASSERT(sizeof(JSExtensibleString) == sizeof(JSString));






class JSInlineString : public JSFlatString
{
    static const size_t MAX_LENGTH_LATIN1 = NUM_INLINE_CHARS_LATIN1 - 1;
    static const size_t MAX_LENGTH_TWO_BYTE = NUM_INLINE_CHARS_TWO_BYTE - 1;

  public:
    template <js::AllowGC allowGC>
    static inline JSInlineString *new_(js::ThreadSafeContext *cx);

    inline jschar *initTwoByte(size_t length);
    inline char *initLatin1(size_t length);

    inline void resetLength(size_t length);

    MOZ_ALWAYS_INLINE
    const jschar *chars() const {
        JS_ASSERT(JSString::isInline());
        JS_ASSERT(hasTwoByteChars());
        return d.inlineStorageTwoByte;
    }

    MOZ_ALWAYS_INLINE
    const char *latin1Chars(const JS::AutoCheckCannotGC &nogc) const {
        JS_ASSERT(JSString::isInline());
        JS_ASSERT(hasLatin1Chars());
        return d.inlineStorageLatin1;
    }

    MOZ_ALWAYS_INLINE
    const jschar *twoByteChars(const JS::AutoCheckCannotGC &nogc) const {
        JS_ASSERT(JSString::isInline());
        JS_ASSERT(hasTwoByteChars());
        return d.inlineStorageTwoByte;
    }

    static bool latin1LengthFits(size_t length) {
        return length <= MAX_LENGTH_LATIN1;
    }
    static bool twoByteLengthFits(size_t length) {
        return length <= MAX_LENGTH_TWO_BYTE;
    }

    static size_t offsetOfInlineStorage() {
        return offsetof(JSInlineString, d.inlineStorageTwoByte);
    }
};

JS_STATIC_ASSERT(sizeof(JSInlineString) == sizeof(JSString));













class JSFatInlineString : public JSInlineString
{
    static const size_t INLINE_EXTENSION_CHARS_LATIN1 = 24 - NUM_INLINE_CHARS_LATIN1;
    static const size_t INLINE_EXTENSION_CHARS_TWO_BYTE = 12 - NUM_INLINE_CHARS_TWO_BYTE;

    static void staticAsserts() {
        JS_STATIC_ASSERT((INLINE_EXTENSION_CHARS_LATIN1 * sizeof(char)) % js::gc::CellSize == 0);
        JS_STATIC_ASSERT((INLINE_EXTENSION_CHARS_TWO_BYTE * sizeof(jschar)) % js::gc::CellSize == 0);
        JS_STATIC_ASSERT(MAX_LENGTH_TWO_BYTE + 1 ==
                         (sizeof(JSFatInlineString) -
                          offsetof(JSFatInlineString, d.inlineStorageTwoByte)) / sizeof(jschar));
        JS_STATIC_ASSERT(MAX_LENGTH_LATIN1 + 1 ==
                         (sizeof(JSFatInlineString) -
                          offsetof(JSFatInlineString, d.inlineStorageLatin1)) / sizeof(char));
    }

    
    const jschar *chars() const MOZ_DELETE;

  protected: 
    union {
        char   inlineStorageExtensionLatin1[INLINE_EXTENSION_CHARS_LATIN1];
        jschar inlineStorageExtensionTwoByte[INLINE_EXTENSION_CHARS_TWO_BYTE];
    };

  public:
    template <js::AllowGC allowGC>
    static inline JSFatInlineString *new_(js::ThreadSafeContext *cx);

    static const size_t MAX_LENGTH_LATIN1 = JSString::NUM_INLINE_CHARS_LATIN1 +
                                            INLINE_EXTENSION_CHARS_LATIN1
                                            -1 ;

    static const size_t MAX_LENGTH_TWO_BYTE = JSString::NUM_INLINE_CHARS_TWO_BYTE +
                                              INLINE_EXTENSION_CHARS_TWO_BYTE
                                              -1 ;

    inline jschar *initTwoByte(size_t length);
    inline char *initLatin1(size_t length);

    static bool latin1LengthFits(size_t length) {
        return length <= MAX_LENGTH_LATIN1;
    }
    static bool twoByteLengthFits(size_t length) {
        return length <= MAX_LENGTH_TWO_BYTE;
    }

    

    MOZ_ALWAYS_INLINE void finalize(js::FreeOp *fop);
};

JS_STATIC_ASSERT(sizeof(JSFatInlineString) % js::gc::CellSize == 0);

class JSExternalString : public JSFlatString
{
    void init(const jschar *chars, size_t length, const JSStringFinalizer *fin);

    
    bool isExternal() const MOZ_DELETE;
    JSExternalString &asExternal() const MOZ_DELETE;

    
    const jschar *chars() const MOZ_DELETE;

  public:
    static inline JSExternalString *new_(JSContext *cx, const jschar *chars, size_t length,
                                         const JSStringFinalizer *fin);

    const JSStringFinalizer *externalFinalizer() const {
        JS_ASSERT(JSString::isExternal());
        return d.s.u3.externalFinalizer;
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

    MOZ_ALWAYS_INLINE
    bool isPermanent() const {
        return JSString::isPermanentAtom();
    }

    
    
    MOZ_ALWAYS_INLINE void morphIntoPermanentAtom() {
        d.u1.flags |= PERMANENT_ATOM_MASK;
    }

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
    union {
        const jschar *twoByteChars_;
        const char *latin1Chars_;
    };
    enum State { Uninitialized, Latin1, TwoByte };
    State state_;

  public:
    explicit ScopedThreadSafeStringInspector(JSString *str)
      : str_(str),
        state_(Uninitialized)
    { }

    bool ensureChars(ThreadSafeContext *cx, const JS::AutoCheckCannotGC &nogc);

    bool hasTwoByteChars() const { return state_ == TwoByte; }
    bool hasLatin1Chars() const { return state_ == Latin1; }

    const jschar *twoByteChars() const {
        MOZ_ASSERT(state_ == TwoByte);
        return twoByteChars_;
    }
    const char *latin1Chars() const {
        MOZ_ASSERT(state_ == Latin1);
        return latin1Chars_;
    }

    JS::TwoByteChars twoByteRange() const {
        MOZ_ASSERT(state_ == TwoByte);
        return JS::TwoByteChars(twoByteChars_, str_->length());
    }
};

class StaticStrings
{
  private:
    
    static const size_t SMALL_CHAR_LIMIT    = 128U;
    static const size_t NUM_SMALL_CHARS     = 64U;

    JSAtom *length2StaticTable[NUM_SMALL_CHARS * NUM_SMALL_CHARS];

  public:
    
    static const size_t UNIT_STATIC_LIMIT   = 256U;
    JSAtom *unitStaticTable[UNIT_STATIC_LIMIT];

    static const size_t INT_STATIC_LIMIT    = 256U;
    JSAtom *intStaticTable[INT_STATIC_LIMIT];

    StaticStrings() {
        mozilla::PodZero(this);
    }

    bool init(JSContext *cx);
    void trace(JSTracer *trc);

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

static MOZ_ALWAYS_INLINE jsid
NameToId(PropertyName *name)
{
    return NON_INTEGER_ATOM_TO_JSID(name);
}

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
        return HandlePropertyName::fromMarkedLocation(&begin()[i]);
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

} 



MOZ_ALWAYS_INLINE const jschar *
JSString::getChars(js::ExclusiveContext *cx)
{
    if (JSLinearString *str = ensureLinear(cx))
        return str->chars();
    return nullptr;
}

MOZ_ALWAYS_INLINE bool
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

MOZ_ALWAYS_INLINE const jschar *
JSString::getCharsZ(js::ExclusiveContext *cx)
{
    if (JSFlatString *str = ensureFlat(cx))
        return str->chars();
    return nullptr;
}

MOZ_ALWAYS_INLINE const jschar *
JSString::pureChars() const
{
    JS_ASSERT(hasPureChars());
    return asLinear().chars();
}

MOZ_ALWAYS_INLINE const jschar *
JSString::pureCharsZ() const
{
    JS_ASSERT(hasPureCharsZ());
    return asFlat().charsZ();
}

MOZ_ALWAYS_INLINE bool
JSString::copyNonPureChars(js::ThreadSafeContext *cx, js::ScopedJSFreePtr<jschar> &out) const
{
    JS_ASSERT(!hasPureChars());
    return asRope().copyNonPureChars(cx, out);
}

MOZ_ALWAYS_INLINE bool
JSString::copyNonPureCharsZ(js::ThreadSafeContext *cx, js::ScopedJSFreePtr<jschar> &out) const
{
    JS_ASSERT(!hasPureChars());
    if (isDependent())
        return asDependent().copyNonPureCharsZ(cx, out);
    return asRope().copyNonPureCharsZ(cx, out);
}

MOZ_ALWAYS_INLINE JSLinearString *
JSString::ensureLinear(js::ExclusiveContext *cx)
{
    return isLinear()
           ? &asLinear()
           : asRope().flatten(cx);
}

MOZ_ALWAYS_INLINE JSFlatString *
JSString::ensureFlat(js::ExclusiveContext *cx)
{
    return isFlat()
           ? &asFlat()
           : isDependent()
             ? asDependent().undepend(cx)
             : asRope().flatten(cx);
}

inline JSLinearString *
JSString::base() const
{
    JS_ASSERT(hasBase());
    JS_ASSERT(!d.s.u3.base->isInline());
    return d.s.u3.base;
}

template<>
MOZ_ALWAYS_INLINE const jschar *
JSLinearString::nonInlineChars(const JS::AutoCheckCannotGC &nogc) const
{
    return nonInlineTwoByteChars(nogc);
}

template<>
MOZ_ALWAYS_INLINE const char *
JSLinearString::nonInlineChars(const JS::AutoCheckCannotGC &nogc) const
{
    return nonInlineLatin1Chars(nogc);
}

template<>
MOZ_ALWAYS_INLINE void
JSString::setNonInlineChars(const jschar *chars)
{
    d.s.u2.nonInlineCharsTwoByte = chars;
}

template<>
MOZ_ALWAYS_INLINE void
JSString::setNonInlineChars(const char *chars)
{
    d.s.u2.nonInlineCharsLatin1 = chars;
}

MOZ_ALWAYS_INLINE const jschar *
JSLinearString::chars() const
{
    JS_ASSERT(JSString::isLinear());
    JS_ASSERT(hasTwoByteChars());
    return isInline() ? asInline().chars() : nonInlineChars();
}

MOZ_ALWAYS_INLINE const char *
JSLinearString::latin1Chars(const JS::AutoCheckCannotGC &nogc) const
{
    JS_ASSERT(JSString::isLinear());
    JS_ASSERT(hasLatin1Chars());
    return isInline() ? asInline().latin1Chars(nogc) : nonInlineLatin1Chars(nogc);
}

MOZ_ALWAYS_INLINE const jschar *
JSLinearString::twoByteChars(const JS::AutoCheckCannotGC &nogc) const
{
    JS_ASSERT(JSString::isLinear());
    JS_ASSERT(hasTwoByteChars());
    return isInline() ? asInline().twoByteChars(nogc) : nonInlineTwoByteChars(nogc);
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

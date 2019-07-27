





#ifndef vm_String_h
#define vm_String_h

#include "mozilla/MemoryReporting.h"
#include "mozilla/PodOperations.h"
#include "mozilla/Range.h"

#include "jsapi.h"
#include "jsfriendapi.h"
#include "jsstr.h"

#include "gc/Barrier.h"
#include "gc/Heap.h"
#include "gc/Marking.h"
#include "gc/Rooting.h"
#include "js/CharacterEncoding.h"
#include "js/GCAPI.h"
#include "js/RootingAPI.h"

class JSDependentString;
class JSExtensibleString;
class JSExternalString;
class JSInlineString;
class JSRope;

namespace js {

class AutoStableStringChars;
class StaticStrings;
class PropertyName;


static const size_t UINT32_CHAR_BUFFER_LENGTH = sizeof("4294967295") - 1;

} 












































































































class JSString : public js::gc::TenuredCell
{
  protected:
    static const size_t NUM_INLINE_CHARS_LATIN1   = 2 * sizeof(void*) / sizeof(JS::Latin1Char);
    static const size_t NUM_INLINE_CHARS_TWO_BYTE = 2 * sizeof(void*) / sizeof(char16_t);

    
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
                
                JS::Latin1Char     inlineStorageLatin1[NUM_INLINE_CHARS_LATIN1];
                char16_t           inlineStorageTwoByte[NUM_INLINE_CHARS_TWO_BYTE];
            };
            struct {
                union {
                    const JS::Latin1Char* nonInlineCharsLatin1; 
                    const char16_t* nonInlineCharsTwoByte;
                    JSString*      left;               
                } u2;
                union {
                    JSLinearString* base;               
                    JSString*      right;              
                    size_t         capacity;            
                    const JSStringFinalizer* externalFinalizer;
                } u3;
            } s;
        };
    } d;

  public:
    

    
























































    static const uint32_t FLAT_BIT               = JS_BIT(0);
    static const uint32_t HAS_BASE_BIT           = JS_BIT(1);
    static const uint32_t INLINE_CHARS_BIT       = JS_BIT(2);
    static const uint32_t ATOM_BIT               = JS_BIT(3);

    static const uint32_t ROPE_FLAGS             = 0;
    static const uint32_t DEPENDENT_FLAGS        = HAS_BASE_BIT;
    static const uint32_t UNDEPENDED_FLAGS       = FLAT_BIT | HAS_BASE_BIT;
    static const uint32_t EXTENSIBLE_FLAGS       = FLAT_BIT | JS_BIT(4);
    static const uint32_t EXTERNAL_FLAGS         = FLAT_BIT | JS_BIT(5);

    static const uint32_t FAT_INLINE_MASK        = INLINE_CHARS_BIT | JS_BIT(4);
    static const uint32_t PERMANENT_ATOM_MASK    = ATOM_BIT | JS_BIT(5);

    
    static const uint32_t INIT_THIN_INLINE_FLAGS = FLAT_BIT | INLINE_CHARS_BIT;
    static const uint32_t INIT_FAT_INLINE_FLAGS  = FLAT_BIT | FAT_INLINE_MASK;

    static const uint32_t TYPE_FLAGS_MASK        = JS_BIT(6) - 1;

    static const uint32_t LATIN1_CHARS_BIT       = JS_BIT(6);

    static const uint32_t MAX_LENGTH             = js::MaxStringLength;

    static const JS::Latin1Char MAX_LATIN1_CHAR = 0xff;

    




    static inline bool validateLength(js::ExclusiveContext* maybecx, size_t length);

    static void staticAsserts() {
        static_assert(JSString::MAX_LENGTH < UINT32_MAX, "Length must fit in 32 bits");
        static_assert(sizeof(JSString) ==
                      (offsetof(JSString, d.inlineStorageLatin1) +
                       NUM_INLINE_CHARS_LATIN1 * sizeof(char)),
                      "Inline Latin1 chars must fit in a JSString");
        static_assert(sizeof(JSString) ==
                      (offsetof(JSString, d.inlineStorageTwoByte) +
                       NUM_INLINE_CHARS_TWO_BYTE * sizeof(char16_t)),
                      "Inline char16_t chars must fit in a JSString");

        
        using js::shadow::String;
        static_assert(offsetof(JSString, d.u1.length) == offsetof(String, length),
                      "shadow::String length offset must match JSString");
        static_assert(offsetof(JSString, d.u1.flags) == offsetof(String, flags),
                      "shadow::String flags offset must match JSString");
        static_assert(offsetof(JSString, d.s.u2.nonInlineCharsLatin1) == offsetof(String, nonInlineCharsLatin1),
                      "shadow::String nonInlineChars offset must match JSString");
        static_assert(offsetof(JSString, d.s.u2.nonInlineCharsTwoByte) == offsetof(String, nonInlineCharsTwoByte),
                      "shadow::String nonInlineChars offset must match JSString");
        static_assert(offsetof(JSString, d.inlineStorageLatin1) == offsetof(String, inlineStorageLatin1),
                      "shadow::String inlineStorage offset must match JSString");
        static_assert(offsetof(JSString, d.inlineStorageTwoByte) == offsetof(String, inlineStorageTwoByte),
                      "shadow::String inlineStorage offset must match JSString");
        static_assert(INLINE_CHARS_BIT == String::INLINE_CHARS_BIT,
                      "shadow::String::INLINE_CHARS_BIT must match JSString::INLINE_CHARS_BIT");
        static_assert(LATIN1_CHARS_BIT == String::LATIN1_CHARS_BIT,
                      "shadow::String::LATIN1_CHARS_BIT must match JSString::LATIN1_CHARS_BIT");
        static_assert(TYPE_FLAGS_MASK == String::TYPE_FLAGS_MASK,
                      "shadow::String::TYPE_FLAGS_MASK must match JSString::TYPE_FLAGS_MASK");
        static_assert(ROPE_FLAGS == String::ROPE_FLAGS,
                      "shadow::String::ROPE_FLAGS must match JSString::ROPE_FLAGS");
    }

    
    friend class JSRope;

  protected:
    template <typename CharT>
    MOZ_ALWAYS_INLINE
    void setNonInlineChars(const CharT* chars);

  public:
    

    MOZ_ALWAYS_INLINE
    size_t length() const {
        return d.u1.length;
    }

    MOZ_ALWAYS_INLINE
    bool empty() const {
        return d.u1.length == 0;
    }

    inline bool getChar(js::ExclusiveContext* cx, size_t index, char16_t* code);

    
    bool hasLatin1Chars() const {
        return d.u1.flags & LATIN1_CHARS_BIT;
    }
    bool hasTwoByteChars() const {
        return !(d.u1.flags & LATIN1_CHARS_BIT);
    }

    

    inline JSLinearString* ensureLinear(js::ExclusiveContext* cx);
    inline JSFlatString* ensureFlat(js::ExclusiveContext* cx);

    static bool ensureLinear(js::ExclusiveContext* cx, JSString* str) {
        return str->ensureLinear(cx) != nullptr;
    }

    

    MOZ_ALWAYS_INLINE
    bool isRope() const {
        return (d.u1.flags & TYPE_FLAGS_MASK) == ROPE_FLAGS;
    }

    MOZ_ALWAYS_INLINE
    JSRope& asRope() const {
        MOZ_ASSERT(isRope());
        return *(JSRope*)this;
    }

    MOZ_ALWAYS_INLINE
    bool isLinear() const {
        return !isRope();
    }

    MOZ_ALWAYS_INLINE
    JSLinearString& asLinear() const {
        MOZ_ASSERT(JSString::isLinear());
        return *(JSLinearString*)this;
    }

    MOZ_ALWAYS_INLINE
    bool isDependent() const {
        return (d.u1.flags & TYPE_FLAGS_MASK) == DEPENDENT_FLAGS;
    }

    MOZ_ALWAYS_INLINE
    JSDependentString& asDependent() const {
        MOZ_ASSERT(isDependent());
        return *(JSDependentString*)this;
    }

    MOZ_ALWAYS_INLINE
    bool isFlat() const {
        return d.u1.flags & FLAT_BIT;
    }

    MOZ_ALWAYS_INLINE
    JSFlatString& asFlat() const {
        MOZ_ASSERT(isFlat());
        return *(JSFlatString*)this;
    }

    MOZ_ALWAYS_INLINE
    bool isExtensible() const {
        return (d.u1.flags & TYPE_FLAGS_MASK) == EXTENSIBLE_FLAGS;
    }

    MOZ_ALWAYS_INLINE
    JSExtensibleString& asExtensible() const {
        MOZ_ASSERT(isExtensible());
        return *(JSExtensibleString*)this;
    }

    MOZ_ALWAYS_INLINE
    bool isInline() const {
        return d.u1.flags & INLINE_CHARS_BIT;
    }

    MOZ_ALWAYS_INLINE
    JSInlineString& asInline() const {
        MOZ_ASSERT(isInline());
        return *(JSInlineString*)this;
    }

    MOZ_ALWAYS_INLINE
    bool isFatInline() const {
        return (d.u1.flags & FAT_INLINE_MASK) == FAT_INLINE_MASK;
    }

    
    bool isExternal() const {
        return (d.u1.flags & TYPE_FLAGS_MASK) == EXTERNAL_FLAGS;
    }

    MOZ_ALWAYS_INLINE
    JSExternalString& asExternal() const {
        MOZ_ASSERT(isExternal());
        return *(JSExternalString*)this;
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
    JSAtom& asAtom() const {
        MOZ_ASSERT(isAtom());
        return *(JSAtom*)this;
    }

    

    inline bool hasBase() const {
        return d.u1.flags & HAS_BASE_BIT;
    }

    inline JSLinearString* base() const;

    void traceBase(JSTracer* trc) {
        MOZ_ASSERT(hasBase());
        js::TraceManuallyBarrieredEdge(trc, &d.s.u3.base, "base");
    }

    

    inline void finalize(js::FreeOp* fop);

    

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf);

    

    static size_t offsetOfLength() {
        return offsetof(JSString, d.u1.length);
    }
    static size_t offsetOfFlags() {
        return offsetof(JSString, d.u1.flags);
    }

    static size_t offsetOfNonInlineChars() {
        static_assert(offsetof(JSString, d.s.u2.nonInlineCharsTwoByte) ==
                      offsetof(JSString, d.s.u2.nonInlineCharsLatin1),
                      "nonInlineCharsTwoByte and nonInlineCharsLatin1 must have same offset");
        return offsetof(JSString, d.s.u2.nonInlineCharsTwoByte);
    }

    static inline js::ThingRootKind rootKind() { return js::THING_ROOT_STRING; }

#ifdef DEBUG
    void dump();
    void dumpCharsNoNewline(FILE* fp=stderr);
    void dumpRepresentation(FILE* fp, int indent) const;
    void dumpRepresentationHeader(FILE* fp, int indent, const char* subclass) const;

    template <typename CharT>
    static void dumpChars(const CharT* s, size_t len, FILE* fp=stderr);

    bool equals(const char* s);
#endif

    inline void traceChildren(JSTracer* trc);

    static MOZ_ALWAYS_INLINE void readBarrier(JSString* thing) {
        if (thing->isPermanentAtom())
            return;

        TenuredCell::readBarrier(thing);
    }

    static MOZ_ALWAYS_INLINE void writeBarrierPre(JSString* thing) {
        if (isNullLike(thing) || thing->isPermanentAtom())
            return;

        TenuredCell::writeBarrierPre(thing);
    }

  private:
    JSString() = delete;
    JSString(const JSString& other) = delete;
    void operator=(const JSString& other) = delete;
};

class JSRope : public JSString
{
    template <typename CharT>
    bool copyCharsInternal(js::ExclusiveContext* cx, js::ScopedJSFreePtr<CharT>& out,
                           bool nullTerminate) const;

    enum UsingBarrier { WithIncrementalBarrier, NoBarrier };

    template<UsingBarrier b, typename CharT>
    JSFlatString* flattenInternal(js::ExclusiveContext* cx);

    template<UsingBarrier b>
    JSFlatString* flattenInternal(js::ExclusiveContext* cx);

    friend class JSString;
    JSFlatString* flatten(js::ExclusiveContext* cx);

    void init(js::ExclusiveContext* cx, JSString* left, JSString* right, size_t length);

  public:
    template <js::AllowGC allowGC>
    static inline JSRope* new_(js::ExclusiveContext* cx,
                               typename js::MaybeRooted<JSString*, allowGC>::HandleType left,
                               typename js::MaybeRooted<JSString*, allowGC>::HandleType right,
                               size_t length);

    bool copyLatin1Chars(js::ExclusiveContext* cx,
                         js::ScopedJSFreePtr<JS::Latin1Char>& out) const;
    bool copyTwoByteChars(js::ExclusiveContext* cx, js::ScopedJSFreePtr<char16_t>& out) const;

    bool copyLatin1CharsZ(js::ExclusiveContext* cx,
                          js::ScopedJSFreePtr<JS::Latin1Char>& out) const;
    bool copyTwoByteCharsZ(js::ExclusiveContext* cx, js::ScopedJSFreePtr<char16_t>& out) const;

    template <typename CharT>
    bool copyChars(js::ExclusiveContext* cx, js::ScopedJSFreePtr<CharT>& out) const;

    JSString* leftChild() const {
        MOZ_ASSERT(isRope());
        return d.s.u2.left;
    }

    JSString* rightChild() const {
        MOZ_ASSERT(isRope());
        return d.s.u3.right;
    }

    void traceChildren(JSTracer* trc) {
        js::TraceManuallyBarrieredEdge(trc, &d.s.u2.left, "left child");
        js::TraceManuallyBarrieredEdge(trc, &d.s.u3.right, "right child");
    }

    static size_t offsetOfLeft() {
        return offsetof(JSRope, d.s.u2.left);
    }
    static size_t offsetOfRight() {
        return offsetof(JSRope, d.s.u3.right);
    }

#ifdef DEBUG
    void dumpRepresentation(FILE* fp, int indent) const;
#endif
};

static_assert(sizeof(JSRope) == sizeof(JSString),
              "string subclasses must be binary-compatible with JSString");

class JSLinearString : public JSString
{
    friend class JSString;
    friend class js::AutoStableStringChars;

    
    JSLinearString* ensureLinear(JSContext* cx) = delete;
    bool isLinear() const = delete;
    JSLinearString& asLinear() const = delete;

  protected:
    
    MOZ_ALWAYS_INLINE
    void* nonInlineCharsRaw() const {
        MOZ_ASSERT(!isInline());
        static_assert(offsetof(JSLinearString, d.s.u2.nonInlineCharsTwoByte) ==
                      offsetof(JSLinearString, d.s.u2.nonInlineCharsLatin1),
                      "nonInlineCharsTwoByte and nonInlineCharsLatin1 must have same offset");
        return (void*)d.s.u2.nonInlineCharsTwoByte;
    }

    MOZ_ALWAYS_INLINE const JS::Latin1Char* rawLatin1Chars() const;
    MOZ_ALWAYS_INLINE const char16_t* rawTwoByteChars() const;

  public:
    template<typename CharT>
    MOZ_ALWAYS_INLINE
    const CharT* nonInlineChars(const JS::AutoCheckCannotGC& nogc) const;

    MOZ_ALWAYS_INLINE
    const JS::Latin1Char* nonInlineLatin1Chars(const JS::AutoCheckCannotGC& nogc) const {
        MOZ_ASSERT(!isInline());
        MOZ_ASSERT(hasLatin1Chars());
        return d.s.u2.nonInlineCharsLatin1;
    }

    MOZ_ALWAYS_INLINE
    const char16_t* nonInlineTwoByteChars(const JS::AutoCheckCannotGC& nogc) const {
        MOZ_ASSERT(!isInline());
        MOZ_ASSERT(hasTwoByteChars());
        return d.s.u2.nonInlineCharsTwoByte;
    }

    template<typename CharT>
    MOZ_ALWAYS_INLINE
    const CharT* chars(const JS::AutoCheckCannotGC& nogc) const;

    MOZ_ALWAYS_INLINE
    const JS::Latin1Char* latin1Chars(const JS::AutoCheckCannotGC& nogc) const {
        return rawLatin1Chars();
    }

    MOZ_ALWAYS_INLINE
    const char16_t* twoByteChars(const JS::AutoCheckCannotGC& nogc) const {
        return rawTwoByteChars();
    }

    mozilla::Range<const JS::Latin1Char> latin1Range(const JS::AutoCheckCannotGC& nogc) const {
        MOZ_ASSERT(JSString::isLinear());
        return mozilla::Range<const JS::Latin1Char>(latin1Chars(nogc), length());
    }

    mozilla::Range<const char16_t> twoByteRange(const JS::AutoCheckCannotGC& nogc) const {
        MOZ_ASSERT(JSString::isLinear());
        return mozilla::Range<const char16_t>(twoByteChars(nogc), length());
    }

    MOZ_ALWAYS_INLINE
    char16_t latin1OrTwoByteChar(size_t index) const {
        MOZ_ASSERT(JSString::isLinear());
        MOZ_ASSERT(index < length());
        JS::AutoCheckCannotGC nogc;
        return hasLatin1Chars() ? latin1Chars(nogc)[index] : twoByteChars(nogc)[index];
    }

#ifdef DEBUG
    void dumpRepresentationChars(FILE* fp, int indent) const;
#endif
};

static_assert(sizeof(JSLinearString) == sizeof(JSString),
              "string subclasses must be binary-compatible with JSString");

class JSDependentString : public JSLinearString
{
    friend class JSString;
    JSFlatString* undepend(js::ExclusiveContext* cx);

    template <typename CharT>
    JSFlatString* undependInternal(js::ExclusiveContext* cx);

    void init(js::ExclusiveContext* cx, JSLinearString* base, size_t start,
              size_t length);

    
    bool isDependent() const = delete;
    JSDependentString& asDependent() const = delete;

    
    size_t baseOffset() const {
        MOZ_ASSERT(JSString::isDependent());
        JS::AutoCheckCannotGC nogc;
        size_t offset;
        if (hasTwoByteChars())
            offset = twoByteChars(nogc) - base()->twoByteChars(nogc);
        else
            offset = latin1Chars(nogc) - base()->latin1Chars(nogc);
        MOZ_ASSERT(offset < base()->length());
        return offset;
    }

  public:
    static inline JSLinearString* new_(js::ExclusiveContext* cx, JSLinearString* base,
                                       size_t start, size_t length);

    inline static size_t offsetOfBase() {
        return offsetof(JSDependentString, d.s.u3.base);
    }

#ifdef DEBUG
    void dumpRepresentation(FILE* fp, int indent) const;
#endif
};

static_assert(sizeof(JSDependentString) == sizeof(JSString),
              "string subclasses must be binary-compatible with JSString");

class JSFlatString : public JSLinearString
{
    
    JSFlatString* ensureFlat(JSContext* cx) = delete;
    bool isFlat() const = delete;
    JSFlatString& asFlat() const = delete;

    template <typename CharT>
    static bool isIndexSlow(const CharT* s, size_t length, uint32_t* indexp);

    void init(const char16_t* chars, size_t length);
    void init(const JS::Latin1Char* chars, size_t length);

  public:
    template <js::AllowGC allowGC, typename CharT>
    static inline JSFlatString* new_(js::ExclusiveContext* cx,
                                     const CharT* chars, size_t length);

    





    inline bool isIndex(uint32_t* indexp) const {
        MOZ_ASSERT(JSString::isFlat());
        JS::AutoCheckCannotGC nogc;
        if (hasLatin1Chars()) {
            const JS::Latin1Char* s = latin1Chars(nogc);
            return JS7_ISDEC(*s) && isIndexSlow(s, length(), indexp);
        }
        const char16_t* s = twoByteChars(nogc);
        return JS7_ISDEC(*s) && isIndexSlow(s, length(), indexp);
    }

    




    inline js::PropertyName* toPropertyName(JSContext* cx);

    



    MOZ_ALWAYS_INLINE JSAtom* morphAtomizedStringIntoAtom() {
        d.u1.flags |= ATOM_BIT;
        return &asAtom();
    }
    MOZ_ALWAYS_INLINE JSAtom* morphAtomizedStringIntoPermanentAtom() {
        d.u1.flags |= PERMANENT_ATOM_MASK;
        return &asAtom();
    }

    inline void finalize(js::FreeOp* fop);

#ifdef DEBUG
    void dumpRepresentation(FILE* fp, int indent) const;
#endif
};

static_assert(sizeof(JSFlatString) == sizeof(JSString),
              "string subclasses must be binary-compatible with JSString");

class JSExtensibleString : public JSFlatString
{
    
    bool isExtensible() const = delete;
    JSExtensibleString& asExtensible() const = delete;

  public:
    MOZ_ALWAYS_INLINE
    size_t capacity() const {
        MOZ_ASSERT(JSString::isExtensible());
        return d.s.u3.capacity;
    }

#ifdef DEBUG
    void dumpRepresentation(FILE* fp, int indent) const;
#endif
};

static_assert(sizeof(JSExtensibleString) == sizeof(JSString),
              "string subclasses must be binary-compatible with JSString");

class JSInlineString : public JSFlatString
{
  public:
    MOZ_ALWAYS_INLINE
    const JS::Latin1Char* latin1Chars(const JS::AutoCheckCannotGC& nogc) const {
        MOZ_ASSERT(JSString::isInline());
        MOZ_ASSERT(hasLatin1Chars());
        return d.inlineStorageLatin1;
    }

    MOZ_ALWAYS_INLINE
    const char16_t* twoByteChars(const JS::AutoCheckCannotGC& nogc) const {
        MOZ_ASSERT(JSString::isInline());
        MOZ_ASSERT(hasTwoByteChars());
        return d.inlineStorageTwoByte;
    }

    template<typename CharT>
    static bool lengthFits(size_t length);

    static size_t offsetOfInlineStorage() {
        return offsetof(JSInlineString, d.inlineStorageTwoByte);
    }

#ifdef DEBUG
    void dumpRepresentation(FILE* fp, int indent) const;
#endif
};

static_assert(sizeof(JSInlineString) == sizeof(JSString),
              "string subclasses must be binary-compatible with JSString");






class JSThinInlineString : public JSInlineString
{
  public:
    static const size_t MAX_LENGTH_LATIN1 = NUM_INLINE_CHARS_LATIN1 - 1;
    static const size_t MAX_LENGTH_TWO_BYTE = NUM_INLINE_CHARS_TWO_BYTE - 1;

    template <js::AllowGC allowGC>
    static inline JSThinInlineString* new_(js::ExclusiveContext* cx);

    template <typename CharT>
    inline CharT* init(size_t length);

    template<typename CharT>
    static bool lengthFits(size_t length);
};

static_assert(sizeof(JSThinInlineString) == sizeof(JSString),
              "string subclasses must be binary-compatible with JSString");














class JSFatInlineString : public JSInlineString
{
    static const size_t INLINE_EXTENSION_CHARS_LATIN1 = 24 - NUM_INLINE_CHARS_LATIN1;
    static const size_t INLINE_EXTENSION_CHARS_TWO_BYTE = 12 - NUM_INLINE_CHARS_TWO_BYTE;

  protected: 
    union {
        char   inlineStorageExtensionLatin1[INLINE_EXTENSION_CHARS_LATIN1];
        char16_t inlineStorageExtensionTwoByte[INLINE_EXTENSION_CHARS_TWO_BYTE];
    };

  public:
    template <js::AllowGC allowGC>
    static inline JSFatInlineString* new_(js::ExclusiveContext* cx);

    static const size_t MAX_LENGTH_LATIN1 = JSString::NUM_INLINE_CHARS_LATIN1 +
                                            INLINE_EXTENSION_CHARS_LATIN1
                                            -1 ;

    static const size_t MAX_LENGTH_TWO_BYTE = JSString::NUM_INLINE_CHARS_TWO_BYTE +
                                              INLINE_EXTENSION_CHARS_TWO_BYTE
                                              -1 ;

    template <typename CharT>
    inline CharT* init(size_t length);

    template<typename CharT>
    static bool lengthFits(size_t length);

    

    MOZ_ALWAYS_INLINE void finalize(js::FreeOp* fop);
};

static_assert(sizeof(JSFatInlineString) % js::gc::CellSize == 0,
              "fat inline strings shouldn't waste space up to the next cell "
              "boundary");

class JSExternalString : public JSFlatString
{
    void init(const char16_t* chars, size_t length, const JSStringFinalizer* fin);

    
    bool isExternal() const = delete;
    JSExternalString& asExternal() const = delete;

  public:
    static inline JSExternalString* new_(JSContext* cx, const char16_t* chars, size_t length,
                                         const JSStringFinalizer* fin);

    const JSStringFinalizer* externalFinalizer() const {
        MOZ_ASSERT(JSString::isExternal());
        return d.s.u3.externalFinalizer;
    }

    



    const char16_t* twoByteChars() const {
        return rawTwoByteChars();
    }

    

    inline void finalize(js::FreeOp* fop);

#ifdef DEBUG
    void dumpRepresentation(FILE* fp, int indent) const;
#endif
};

static_assert(sizeof(JSExternalString) == sizeof(JSString),
              "string subclasses must be binary-compatible with JSString");

class JSUndependedString : public JSFlatString
{
    




};

static_assert(sizeof(JSUndependedString) == sizeof(JSString),
              "string subclasses must be binary-compatible with JSString");

class JSAtom : public JSFlatString
{
    
    bool isAtom() const = delete;
    JSAtom& asAtom() const = delete;

  public:
    
    inline js::PropertyName* asPropertyName();

    inline void finalize(js::FreeOp* fop);

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

static_assert(sizeof(JSAtom) == sizeof(JSString),
              "string subclasses must be binary-compatible with JSString");

namespace js {

class StaticStrings
{
  private:
    
    static const size_t SMALL_CHAR_LIMIT    = 128U;
    static const size_t NUM_SMALL_CHARS     = 64U;

    JSAtom* length2StaticTable[NUM_SMALL_CHARS * NUM_SMALL_CHARS];

  public:
    
    static const size_t UNIT_STATIC_LIMIT   = 256U;
    JSAtom* unitStaticTable[UNIT_STATIC_LIMIT];

    static const size_t INT_STATIC_LIMIT    = 256U;
    JSAtom* intStaticTable[INT_STATIC_LIMIT];

    StaticStrings() {
        mozilla::PodZero(this);
    }

    bool init(JSContext* cx);
    void trace(JSTracer* trc);

    static bool hasUint(uint32_t u) { return u < INT_STATIC_LIMIT; }

    JSAtom* getUint(uint32_t u) {
        MOZ_ASSERT(hasUint(u));
        return intStaticTable[u];
    }

    static bool hasInt(int32_t i) {
        return uint32_t(i) < INT_STATIC_LIMIT;
    }

    JSAtom* getInt(int32_t i) {
        MOZ_ASSERT(hasInt(i));
        return getUint(uint32_t(i));
    }

    static bool hasUnit(char16_t c) { return c < UNIT_STATIC_LIMIT; }

    JSAtom* getUnit(char16_t c) {
        MOZ_ASSERT(hasUnit(c));
        return unitStaticTable[c];
    }

    
    inline JSLinearString* getUnitStringForElement(JSContext* cx, JSString* str, size_t index);

    template <typename CharT>
    static bool isStatic(const CharT* chars, size_t len);
    static bool isStatic(JSAtom* atom);

    
    template <typename CharT>
    JSAtom* lookup(const CharT* chars, size_t length) {
        switch (length) {
          case 1: {
            char16_t c = chars[0];
            if (c < UNIT_STATIC_LIMIT)
                return getUnit(c);
            return nullptr;
          }
          case 2:
            if (fitsInSmallChar(chars[0]) && fitsInSmallChar(chars[1]))
                return getLength2(chars[0], chars[1]);
            return nullptr;
          case 3:
            





            static_assert(INT_STATIC_LIMIT <= 999,
                          "static int strings assumed below to be at most "
                          "three digits");
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

    static bool fitsInSmallChar(char16_t c) {
        return c < SMALL_CHAR_LIMIT && toSmallChar[c] != INVALID_SMALL_CHAR;
    }

    static const SmallChar toSmallChar[];

    JSAtom* getLength2(char16_t c1, char16_t c2);
    JSAtom* getLength2(uint32_t u) {
        MOZ_ASSERT(u < 100);
        return getLength2('0' + u / 10, '0' + u % 10);
    }
};
















class PropertyName : public JSAtom
{};

static_assert(sizeof(PropertyName) == sizeof(JSString),
              "string subclasses must be binary-compatible with JSString");

static MOZ_ALWAYS_INLINE jsid
NameToId(PropertyName* name)
{
    return NON_INTEGER_ATOM_TO_JSID(name);
}

class AutoNameVector : public AutoVectorRooter<PropertyName*>
{
    typedef AutoVectorRooter<PropertyName*> BaseType;
  public:
    explicit AutoNameVector(JSContext* cx
                            MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<PropertyName*>(cx, NAMEVECTOR)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    HandlePropertyName operator[](size_t i) const {
        return HandlePropertyName::fromMarkedLocation(&begin()[i]);
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

template <typename CharT>
void
CopyChars(CharT* dest, const JSLinearString& str);


template <js::AllowGC allowGC, typename CharT>
extern JSFlatString*
NewString(js::ExclusiveContext* cx, CharT* chars, size_t length);


template <js::AllowGC allowGC, typename CharT>
extern JSFlatString*
NewStringDontDeflate(js::ExclusiveContext* cx, CharT* chars, size_t length);

extern JSLinearString*
NewDependentString(JSContext* cx, JSString* base, size_t start, size_t length);


template <js::AllowGC allowGC, typename CharT>
extern JSFlatString*
NewStringCopyN(js::ExclusiveContext* cx, const CharT* s, size_t n);

template <js::AllowGC allowGC>
inline JSFlatString*
NewStringCopyN(ExclusiveContext* cx, const char* s, size_t n)
{
    return NewStringCopyN<allowGC>(cx, reinterpret_cast<const Latin1Char*>(s), n);
}


template <js::AllowGC allowGC, typename CharT>
extern JSFlatString*
NewStringCopyNDontDeflate(js::ExclusiveContext* cx, const CharT* s, size_t n);


template <js::AllowGC allowGC>
inline JSFlatString*
NewStringCopyZ(js::ExclusiveContext* cx, const char16_t* s)
{
    return NewStringCopyN<allowGC>(cx, s, js_strlen(s));
}

template <js::AllowGC allowGC>
inline JSFlatString*
NewStringCopyZ(js::ExclusiveContext* cx, const char* s)
{
    return NewStringCopyN<allowGC>(cx, s, strlen(s));
}

} 



class JSAddonId : public JSAtom
{};

MOZ_ALWAYS_INLINE bool
JSString::getChar(js::ExclusiveContext* cx, size_t index, char16_t* code)
{
    MOZ_ASSERT(index < length());

    








    JSString* str;
    if (isRope()) {
        JSRope* rope = &asRope();
        if (uint32_t(index) < rope->leftChild()->length()) {
            str = rope->leftChild();
        } else {
            str = rope->rightChild();
            index -= rope->leftChild()->length();
        }
    } else {
        str = this;
    }

    if (!str->ensureLinear(cx))
        return false;

    *code = str->asLinear().latin1OrTwoByteChar(index);
    return true;
}

MOZ_ALWAYS_INLINE JSLinearString*
JSString::ensureLinear(js::ExclusiveContext* cx)
{
    return isLinear()
           ? &asLinear()
           : asRope().flatten(cx);
}

MOZ_ALWAYS_INLINE JSFlatString*
JSString::ensureFlat(js::ExclusiveContext* cx)
{
    return isFlat()
           ? &asFlat()
           : isDependent()
             ? asDependent().undepend(cx)
             : asRope().flatten(cx);
}

inline JSLinearString*
JSString::base() const
{
    MOZ_ASSERT(hasBase());
    MOZ_ASSERT(!d.s.u3.base->isInline());
    return d.s.u3.base;
}

inline void
JSString::traceChildren(JSTracer* trc)
{
    if (hasBase())
        traceBase(trc);
    else if (isRope())
        asRope().traceChildren(trc);
}

template<>
MOZ_ALWAYS_INLINE const char16_t*
JSLinearString::nonInlineChars(const JS::AutoCheckCannotGC& nogc) const
{
    return nonInlineTwoByteChars(nogc);
}

template<>
MOZ_ALWAYS_INLINE const JS::Latin1Char*
JSLinearString::nonInlineChars(const JS::AutoCheckCannotGC& nogc) const
{
    return nonInlineLatin1Chars(nogc);
}

template<>
MOZ_ALWAYS_INLINE const char16_t*
JSLinearString::chars(const JS::AutoCheckCannotGC& nogc) const
{
    return rawTwoByteChars();
}

template<>
MOZ_ALWAYS_INLINE const JS::Latin1Char*
JSLinearString::chars(const JS::AutoCheckCannotGC& nogc) const
{
    return rawLatin1Chars();
}

template <>
MOZ_ALWAYS_INLINE bool
JSRope::copyChars<JS::Latin1Char>(js::ExclusiveContext* cx,
                                  js::ScopedJSFreePtr<JS::Latin1Char>& out) const
{
    return copyLatin1Chars(cx, out);
}

template <>
MOZ_ALWAYS_INLINE bool
JSRope::copyChars<char16_t>(js::ExclusiveContext* cx, js::ScopedJSFreePtr<char16_t>& out) const
{
    return copyTwoByteChars(cx, out);
}

template<>
MOZ_ALWAYS_INLINE bool
JSThinInlineString::lengthFits<JS::Latin1Char>(size_t length)
{
    return length <= MAX_LENGTH_LATIN1;
}

template<>
MOZ_ALWAYS_INLINE bool
JSThinInlineString::lengthFits<char16_t>(size_t length)
{
    return length <= MAX_LENGTH_TWO_BYTE;
}

template<>
MOZ_ALWAYS_INLINE bool
JSFatInlineString::lengthFits<JS::Latin1Char>(size_t length)
{
    static_assert((INLINE_EXTENSION_CHARS_LATIN1 * sizeof(char)) % js::gc::CellSize == 0,
                  "fat inline strings' Latin1 characters don't exactly "
                  "fill subsequent cells and thus are wasteful");
    static_assert(MAX_LENGTH_LATIN1 + 1 ==
                  (sizeof(JSFatInlineString) -
                   offsetof(JSFatInlineString, d.inlineStorageLatin1)) / sizeof(char),
                  "MAX_LENGTH_LATIN1 must be one less than inline Latin1 "
                  "storage count");

    return length <= MAX_LENGTH_LATIN1;
}

template<>
MOZ_ALWAYS_INLINE bool
JSFatInlineString::lengthFits<char16_t>(size_t length)
{
    static_assert((INLINE_EXTENSION_CHARS_TWO_BYTE * sizeof(char16_t)) % js::gc::CellSize == 0,
                  "fat inline strings' char16_t characters don't exactly "
                  "fill subsequent cells and thus are wasteful");
    static_assert(MAX_LENGTH_TWO_BYTE + 1 ==
                  (sizeof(JSFatInlineString) -
                   offsetof(JSFatInlineString, d.inlineStorageTwoByte)) / sizeof(char16_t),
                  "MAX_LENGTH_TWO_BYTE must be one less than inline "
                  "char16_t storage count");

    return length <= MAX_LENGTH_TWO_BYTE;
}

template<>
MOZ_ALWAYS_INLINE bool
JSInlineString::lengthFits<JS::Latin1Char>(size_t length)
{
    
    return JSFatInlineString::lengthFits<JS::Latin1Char>(length);
}

template<>
MOZ_ALWAYS_INLINE bool
JSInlineString::lengthFits<char16_t>(size_t length)
{
    
    return JSFatInlineString::lengthFits<char16_t>(length);
}

template<>
MOZ_ALWAYS_INLINE void
JSString::setNonInlineChars(const char16_t* chars)
{
    d.s.u2.nonInlineCharsTwoByte = chars;
}

template<>
MOZ_ALWAYS_INLINE void
JSString::setNonInlineChars(const JS::Latin1Char* chars)
{
    d.s.u2.nonInlineCharsLatin1 = chars;
}

MOZ_ALWAYS_INLINE const JS::Latin1Char*
JSLinearString::rawLatin1Chars() const
{
    MOZ_ASSERT(JSString::isLinear());
    MOZ_ASSERT(hasLatin1Chars());
    return isInline() ? d.inlineStorageLatin1 : d.s.u2.nonInlineCharsLatin1;
}

MOZ_ALWAYS_INLINE const char16_t*
JSLinearString::rawTwoByteChars() const
{
    MOZ_ASSERT(JSString::isLinear());
    MOZ_ASSERT(hasTwoByteChars());
    return isInline() ? d.inlineStorageTwoByte : d.s.u2.nonInlineCharsTwoByte;
}

inline js::PropertyName*
JSAtom::asPropertyName()
{
#ifdef DEBUG
    uint32_t dummy;
    MOZ_ASSERT(!isIndex(&dummy));
#endif
    return static_cast<js::PropertyName*>(this);
}

#endif 

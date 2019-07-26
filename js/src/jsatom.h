





#ifndef jsatom_h___
#define jsatom_h___

#include "mozilla/HashFunctions.h"

#include <stddef.h>
#include "jsalloc.h"
#include "jsapi.h"
#include "jsfriendapi.h"
#include "jsprototypes.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jslock.h"
#include "jsversion.h"

#include "gc/Barrier.h"
#include "js/HashTable.h"
#include "vm/CommonPropertyNames.h"

ForwardDeclareJS(Atom);

struct JSIdArray {
    int length;
    js::HeapId vector[1];    
};

namespace js {

JS_STATIC_ASSERT(sizeof(HashNumber) == 4);

static JS_ALWAYS_INLINE js::HashNumber
HashId(jsid id)
{
    return mozilla::HashGeneric(JSID_BITS(id));
}

struct JsidHasher
{
    typedef jsid Lookup;
    static HashNumber hash(const Lookup &l) {
        return HashNumber(JSID_BITS(l));
    }
    static bool match(const jsid &id, const Lookup &l) {
        return id == l;
    }
};

} 





extern const char *
js_AtomToPrintableString(JSContext *cx, JSAtom *atom, JSAutoByteString *bytes);

namespace js {


inline uint32_t
HashChars(const jschar *chars, size_t length)
{
    uint32_t h = 0;
    for (; length; chars++, length--)
        h = JS_ROTATE_LEFT32(h, 4) ^ *chars;
    return h;
}

class AtomStateEntry
{
    uintptr_t bits;

    static const uintptr_t NO_TAG_MASK = uintptr_t(-1) - 1;

  public:
    AtomStateEntry() : bits(0) {}
    AtomStateEntry(const AtomStateEntry &other) : bits(other.bits) {}
    AtomStateEntry(RawAtom ptr, bool tagged)
      : bits(uintptr_t(ptr) | uintptr_t(tagged))
    {
        JS_ASSERT((uintptr_t(ptr) & 0x1) == 0);
    }

    bool isTagged() const {
        return bits & 0x1;
    }

    



    void setTagged(bool enabled) const {
        const_cast<AtomStateEntry *>(this)->bits |= uintptr_t(enabled);
    }

    JSAtom *asPtr() const;
};

struct AtomHasher
{
    struct Lookup
    {
        const jschar    *chars;
        size_t          length;
        const JSAtom    *atom; 

        Lookup(const jschar *chars, size_t length) : chars(chars), length(length), atom(NULL) {}
        inline Lookup(const JSAtom *atom);
    };

    static HashNumber hash(const Lookup &l) { return HashChars(l.chars, l.length); }
    static inline bool match(const AtomStateEntry &entry, const Lookup &lookup);
};

typedef HashSet<AtomStateEntry, AtomHasher, SystemAllocPolicy> AtomSet;

class PropertyName;

}  

extern bool
AtomIsInterned(JSContext *cx, JSAtom *atom);


#define DECLARE_PROTO_STR(name,code,init) extern const char js_##name##_str[];
JS_FOR_EACH_PROTOTYPE(DECLARE_PROTO_STR)
#undef DECLARE_PROTO_STR

#define DECLARE_CONST_CHAR_STR(idpart, id, text)  extern const char js_##idpart##_str[];
FOR_EACH_COMMON_PROPERTYNAME(DECLARE_CONST_CHAR_STR)
#undef DECLARE_CONST_CHAR_STR


extern const char js_break_str[];
extern const char js_case_str[];
extern const char js_catch_str[];
extern const char js_class_str[];
extern const char js_const_str[];
extern const char js_continue_str[];
extern const char js_debugger_str[];
extern const char js_default_str[];
extern const char js_do_str[];
extern const char js_else_str[];
extern const char js_enum_str[];
extern const char js_export_str[];
extern const char js_extends_str[];
extern const char js_finally_str[];
extern const char js_for_str[];
extern const char js_getter_str[];
extern const char js_if_str[];
extern const char js_implements_str[];
extern const char js_import_str[];
extern const char js_in_str[];
extern const char js_instanceof_str[];
extern const char js_interface_str[];
extern const char js_let_str[];
extern const char js_new_str[];
extern const char js_package_str[];
extern const char js_private_str[];
extern const char js_protected_str[];
extern const char js_public_str[];
extern const char js_setter_str[];
extern const char js_static_str[];
extern const char js_super_str[];
extern const char js_switch_str[];
extern const char js_this_str[];
extern const char js_try_str[];
extern const char js_typeof_str[];
extern const char js_void_str[];
extern const char js_while_str[];
extern const char js_with_str[];
extern const char js_yield_str[];
#if JS_HAS_GENERATORS
extern const char   js_close_str[];
extern const char   js_send_str[];
#endif

namespace js {

extern const char * TypeStrings[];






extern JSBool
InitAtoms(JSRuntime *rt);





extern void
FinishAtoms(JSRuntime *rt);




extern void
MarkAtoms(JSTracer *trc);

extern void
SweepAtoms(JSRuntime *rt);

extern bool
InitCommonNames(JSContext *cx);

extern void
FinishCommonNames(JSRuntime *rt);


enum InternBehavior
{
    DoNotInternAtom = false,
    InternAtom = true
};

extern RawAtom
Atomize(JSContext *cx, const char *bytes, size_t length,
        js::InternBehavior ib = js::DoNotInternAtom);

template <AllowGC allowGC>
extern RawAtom
AtomizeChars(JSContext *cx, const jschar *chars, size_t length,
             js::InternBehavior ib = js::DoNotInternAtom);

template <AllowGC allowGC>
extern RawAtom
AtomizeString(JSContext *cx, JSString *str, js::InternBehavior ib = js::DoNotInternAtom);

template <AllowGC allowGC>
inline JSAtom *
ToAtom(JSContext *cx, const js::Value &v);

template <AllowGC allowGC>
bool
InternNonIntElementId(JSContext *cx, JSObject *obj, const Value &idval,
                      typename MaybeRooted<jsid, allowGC>::MutableHandleType idp,
                      typename MaybeRooted<Value, allowGC>::MutableHandleType vp);

template <AllowGC allowGC>
inline bool
InternNonIntElementId(JSContext *cx, JSObject *obj, const Value &idval,
                      typename MaybeRooted<jsid, allowGC>::MutableHandleType idp)
{
    typename MaybeRooted<Value, allowGC>::RootType dummy(cx);
    return InternNonIntElementId<allowGC>(cx, obj, idval, idp, &dummy);
}

template<XDRMode mode>
bool
XDRAtom(XDRState<mode> *xdr, js::MutableHandleAtom atomp);

} 

#endif 

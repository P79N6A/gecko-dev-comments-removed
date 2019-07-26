





#ifndef js_Id_h
#define js_Id_h














#include "mozilla/NullPtr.h"

#include "jstypes.h"

#include "js/HeapAPI.h"
#include "js/RootingAPI.h"
#include "js/TypeDecls.h"
#include "js/Utility.h"

struct jsid
{
    size_t asBits;
    bool operator==(jsid rhs) const { return asBits == rhs.asBits; }
    bool operator!=(jsid rhs) const { return asBits != rhs.asBits; }
};
#define JSID_BITS(id) (id.asBits)

#define JSID_TYPE_STRING                 0x0
#define JSID_TYPE_INT                    0x1
#define JSID_TYPE_VOID                   0x2
#define JSID_TYPE_OBJECT                 0x4
#define JSID_TYPE_MASK                   0x7



#define id iden

static MOZ_ALWAYS_INLINE bool
JSID_IS_STRING(jsid id)
{
    return (JSID_BITS(id) & JSID_TYPE_MASK) == 0;
}

static MOZ_ALWAYS_INLINE JSString *
JSID_TO_STRING(jsid id)
{
    MOZ_ASSERT(JSID_IS_STRING(id));
    return (JSString *)JSID_BITS(id);
}

static MOZ_ALWAYS_INLINE bool
JSID_IS_ZERO(jsid id)
{
    return JSID_BITS(id) == 0;
}

static MOZ_ALWAYS_INLINE bool
JSID_IS_INT(jsid id)
{
    return !!(JSID_BITS(id) & JSID_TYPE_INT);
}

static MOZ_ALWAYS_INLINE int32_t
JSID_TO_INT(jsid id)
{
    MOZ_ASSERT(JSID_IS_INT(id));
    return ((uint32_t)JSID_BITS(id)) >> 1;
}

#define JSID_INT_MIN  0
#define JSID_INT_MAX  INT32_MAX

static MOZ_ALWAYS_INLINE bool
INT_FITS_IN_JSID(int32_t i)
{
    return i >= 0;
}

static MOZ_ALWAYS_INLINE jsid
INT_TO_JSID(int32_t i)
{
    jsid id;
    MOZ_ASSERT(INT_FITS_IN_JSID(i));
    JSID_BITS(id) = ((i << 1) | JSID_TYPE_INT);
    return id;
}

static MOZ_ALWAYS_INLINE bool
JSID_IS_OBJECT(jsid id)
{
    return (JSID_BITS(id) & JSID_TYPE_MASK) == JSID_TYPE_OBJECT &&
           (size_t)JSID_BITS(id) != JSID_TYPE_OBJECT;
}

static MOZ_ALWAYS_INLINE JSObject *
JSID_TO_OBJECT(jsid id)
{
    MOZ_ASSERT(JSID_IS_OBJECT(id));
    return (JSObject *)(JSID_BITS(id) & ~(size_t)JSID_TYPE_MASK);
}

static MOZ_ALWAYS_INLINE jsid
OBJECT_TO_JSID(JSObject *obj)
{
    jsid id;
    MOZ_ASSERT(obj != nullptr);
    MOZ_ASSERT(((size_t)obj & JSID_TYPE_MASK) == 0);
    JS_ASSERT(!js::gc::IsInsideNursery(js::gc::GetGCThingRuntime(obj), obj));
    JSID_BITS(id) = ((size_t)obj | JSID_TYPE_OBJECT);
    return id;
}

static MOZ_ALWAYS_INLINE bool
JSID_IS_GCTHING(jsid id)
{
    return JSID_IS_STRING(id) || JSID_IS_OBJECT(id);
}

static MOZ_ALWAYS_INLINE void *
JSID_TO_GCTHING(jsid id)
{
    return (void *)(JSID_BITS(id) & ~(size_t)JSID_TYPE_MASK);
}

static MOZ_ALWAYS_INLINE bool
JSID_IS_VOID(const jsid id)
{
    MOZ_ASSERT_IF(((size_t)JSID_BITS(id) & JSID_TYPE_MASK) == JSID_TYPE_VOID,
                 JSID_BITS(id) == JSID_TYPE_VOID);
    return ((size_t)JSID_BITS(id) == JSID_TYPE_VOID);
}

static MOZ_ALWAYS_INLINE bool
JSID_IS_EMPTY(const jsid id)
{
    return ((size_t)JSID_BITS(id) == JSID_TYPE_OBJECT);
}

#undef id

extern JS_PUBLIC_DATA(const jsid) JSID_VOID;
extern JS_PUBLIC_DATA(const jsid) JSID_EMPTY;

extern JS_PUBLIC_DATA(const JS::HandleId) JSID_VOIDHANDLE;
extern JS_PUBLIC_DATA(const JS::HandleId) JSID_EMPTYHANDLE;

namespace js {

inline bool
IsPoisonedId(jsid iden)
{
    if (JSID_IS_STRING(iden))
        return JS::IsPoisonedPtr(JSID_TO_STRING(iden));
    if (JSID_IS_OBJECT(iden))
        return JS::IsPoisonedPtr(JSID_TO_OBJECT(iden));
    return false;
}

template <> struct GCMethods<jsid>
{
    static jsid initial() { return JSID_VOID; }
    static ThingRootKind kind() { return THING_ROOT_ID; }
    static bool poisoned(jsid id) { return IsPoisonedId(id); }
    static bool needsPostBarrier(jsid id) { return false; }
#ifdef JSGC_GENERATIONAL
    static void postBarrier(jsid *idp) {}
    static void relocate(jsid *idp) {}
#endif
};

}

#endif 

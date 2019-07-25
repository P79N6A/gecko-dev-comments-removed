







































#ifndef RegExpObject_inl_h___
#define RegExpObject_inl_h___

#include "mozilla/Util.h"

#include "RegExpObject.h"
#include "RegExpStatics.h"

#include "jsobjinlines.h"
#include "jsstrinlines.h"
#include "RegExpStatics-inl.h"

inline js::RegExpObject &
JSObject::asRegExp()
{
    JS_ASSERT(isRegExp());
    return *static_cast<js::RegExpObject *>(this);
}

namespace js {

inline RegExpShared &
RegExpObject::shared() const
{
    JS_ASSERT(JSObject::getPrivate() != NULL);
    return *static_cast<RegExpShared *>(JSObject::getPrivate());
}

inline RegExpShared *
RegExpObject::maybeShared()
{
    return static_cast<RegExpShared *>(JSObject::getPrivate());
}

inline RegExpShared *
RegExpObject::getShared(JSContext *cx)
{
    if (RegExpShared *shared = maybeShared())
        return shared;
    return createShared(cx);
}

inline void
RegExpObject::setLastIndex(const Value &v)
{
    setSlot(LAST_INDEX_SLOT, v);
}

inline void
RegExpObject::setLastIndex(double d)
{
    setSlot(LAST_INDEX_SLOT, NumberValue(d));
}

inline void
RegExpObject::zeroLastIndex()
{
    setSlot(LAST_INDEX_SLOT, Int32Value(0));
}

inline void
RegExpObject::setSource(JSAtom *source)
{
    setSlot(SOURCE_SLOT, StringValue(source));
}

inline void
RegExpObject::setIgnoreCase(bool enabled)
{
    setSlot(IGNORE_CASE_FLAG_SLOT, BooleanValue(enabled));
}

inline void
RegExpObject::setGlobal(bool enabled)
{
    setSlot(GLOBAL_FLAG_SLOT, BooleanValue(enabled));
}

inline void
RegExpObject::setMultiline(bool enabled)
{
    setSlot(MULTILINE_FLAG_SLOT, BooleanValue(enabled));
}

inline void
RegExpObject::setSticky(bool enabled)
{
    setSlot(STICKY_FLAG_SLOT, BooleanValue(enabled));
}


inline bool
detail::RegExpCode::isJITRuntimeEnabled(JSContext *cx)
{
#if defined(ANDROID) && defined(JS_METHODJIT)
    return cx->methodJitEnabled;
#else
    return true;
#endif
}

inline RegExpShared *
RegExpToShared(JSContext *cx, JSObject &obj)
{
    JS_ASSERT(ObjectClassIs(obj, ESClass_RegExp, cx));
    if (obj.isRegExp())
        return obj.asRegExp().getShared(cx);
    return Proxy::regexp_toShared(cx, &obj);
}

} 

#endif

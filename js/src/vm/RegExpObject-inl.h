






#ifndef RegExpObject_inl_h___
#define RegExpObject_inl_h___

#include "mozilla/Util.h"

#include "RegExpObject.h"
#include "RegExpStatics.h"

#include "jsobjinlines.h"
#include "jsstrinlines.h"

#include "RegExpStatics-inl.h"
#include "String-inl.h"

inline js::RegExpObject &
JSObject::asRegExp()
{
    JS_ASSERT(isRegExp());
    return *static_cast<js::RegExpObject *>(this);
}

namespace js {

inline RegExpShared *
RegExpObject::maybeShared() const
{
    return static_cast<RegExpShared *>(JSObject::getPrivate());
}

inline void
RegExpObject::shared(RegExpGuard *g) const
{
    JS_ASSERT(maybeShared() != NULL);
    g->init(*maybeShared());
}

inline bool
RegExpObject::getShared(JSContext *cx, RegExpGuard *g)
{
    if (RegExpShared *shared = maybeShared()) {
        g->init(*shared);
        return true;
    }
    return createShared(cx, g);
}

inline void
RegExpObject::setShared(JSContext *cx, RegExpShared &shared)
{
    shared.prepareForUse(cx);
    JSObject::setPrivate(&shared);
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

inline void
RegExpShared::writeBarrierPre()
{
    JSString::writeBarrierPre(source);
}


inline bool
RegExpShared::isJITRuntimeEnabled(JSContext *cx)
{
#if ENABLE_YARR_JIT
# if defined(ANDROID) && defined(JS_METHODJIT)
    return cx->methodJitEnabled;
# else
    return true;
# endif
#else
    return false;
#endif
}

inline bool
RegExpToShared(JSContext *cx, JSObject &obj, RegExpGuard *g)
{
    if (obj.isRegExp())
        return obj.asRegExp().getShared(cx, g);
    return Proxy::regexp_toShared(cx, &obj, g);
}

inline void
RegExpShared::prepareForUse(JSContext *cx)
{
    gcNumberWhenUsed = cx->runtime->gcNumber;
}

RegExpGuard::RegExpGuard(JSContext *cx)
  : re_(NULL), source_(cx)
{
}

RegExpGuard::RegExpGuard(JSContext *cx, RegExpShared &re)
  : re_(&re), source_(cx, re.source)
{
    re_->incRef();
}

RegExpGuard::~RegExpGuard()
{
    release();
}

inline void
RegExpGuard::init(RegExpShared &re)
{
    JS_ASSERT(!initialized());
    re_ = &re;
    re_->incRef();
    source_ = re_->source;
}

inline void
RegExpGuard::release()
{
    if (re_) {
        re_->decRef();
        re_ = NULL;
        source_ = NULL;
    }
}

RegExpHeapGuard::RegExpHeapGuard(RegExpShared &re)
{
    init(re);
}

RegExpHeapGuard::~RegExpHeapGuard()
{
    release();
}

inline void
RegExpHeapGuard::init(RegExpShared &re)
{
    JS_ASSERT(!initialized());
    re_ = &re;
    re_->incRef();
}

inline void
RegExpHeapGuard::release()
{
    if (re_) {
        re_->writeBarrierPre();
        re_->decRef();
        re_ = NULL;
    }
}

} 

#endif






#ifndef Iterator_inl_h_
#define Iterator_inl_h_

#include "jsiter.h"
#include "jsobjinlines.h"

inline bool
JSObject::isPropertyIterator() const
{
    return hasClass(&js::PropertyIteratorObject::class_);
}

inline js::PropertyIteratorObject &
JSObject::asPropertyIterator()
{
    JS_ASSERT(isPropertyIterator());
    return *static_cast<js::PropertyIteratorObject *>(this);
}

inline const js::PropertyIteratorObject &
JSObject::asPropertyIterator() const
{
    JS_ASSERT(isPropertyIterator());
    return *static_cast<const js::PropertyIteratorObject *>(this);
}

js::NativeIterator *
js::PropertyIteratorObject::getNativeIterator() const
{
    JS_ASSERT(isPropertyIterator());
    return static_cast<js::NativeIterator *>(getPrivate());
}

inline void
js::PropertyIteratorObject::setNativeIterator(js::NativeIterator *ni)
{
    setPrivate(ni);
}

#endif  

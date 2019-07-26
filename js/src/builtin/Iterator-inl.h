





#ifndef Iterator_inl_h_
#define Iterator_inl_h_

#include "jsiter.h"
#include "vm/ObjectImpl-inl.h"

js::NativeIterator *
js::PropertyIteratorObject::getNativeIterator() const
{
    JS_ASSERT(is<PropertyIteratorObject>());
    return static_cast<js::NativeIterator *>(getPrivate());
}

inline void
js::PropertyIteratorObject::setNativeIterator(js::NativeIterator *ni)
{
    setPrivate(ni);
}

#endif  

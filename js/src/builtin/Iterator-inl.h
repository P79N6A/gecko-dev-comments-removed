





#ifndef builtin_Iterator_inl_h
#define builtin_Iterator_inl_h

#include "jsiter.h"
#include "vm/ObjectImpl-inl.h"

inline void
js::PropertyIteratorObject::setNativeIterator(js::NativeIterator *ni)
{
    setPrivate(ni);
}

#endif 








#ifndef ObjectImpl_inl_h___
#define ObjectImpl_inl_h___

#include "ObjectImpl.h"

inline bool
js::ObjectImpl::isNative() const
{
    return lastProperty()->isNative();
}

inline js::Class *
js::ObjectImpl::getClass() const
{
    return lastProperty()->getObjectClass();
}

inline JSClass *
js::ObjectImpl::getJSClass() const
{
    return Jsvalify(getClass());
}

inline bool
js::ObjectImpl::hasClass(const Class *c) const
{
    return getClass() == c;
}

inline const js::ObjectOps *
js::ObjectImpl::getOps() const
{
    return &getClass()->ops;
}

inline bool
js::ObjectImpl::isDelegate() const
{
    return lastProperty()->hasObjectFlag(BaseShape::DELEGATE);
}

inline bool
js::ObjectImpl::inDictionaryMode() const
{
    return lastProperty()->inDictionary();
}

#endif 

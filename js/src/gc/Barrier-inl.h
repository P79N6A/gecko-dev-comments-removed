





#ifndef gc_Barrier_inl_h
#define gc_Barrier_inl_h

#include "gc/Barrier.h"

#include "jscompartment.h"

#include "gc/Marking.h"
#include "gc/StoreBuffer.h"

#include "vm/String-inl.h"

namespace js {

inline const Value &
ReadBarrieredValue::get() const
{
    if (value.isObject())
        JSObject::readBarrier(&value.toObject());
    else if (value.isString())
        JSString::readBarrier(value.toString());
    else
        JS_ASSERT(!value.isMarkable());

    return value;
}

inline
ReadBarrieredValue::operator const Value &() const
{
    return get();
}

inline JSObject &
ReadBarrieredValue::toObject() const
{
    return get().toObject();
}

} 

#endif 

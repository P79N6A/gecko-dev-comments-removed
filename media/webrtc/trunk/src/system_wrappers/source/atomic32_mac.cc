









#include "atomic32.h"

#include <assert.h>
#include <libkern/OSAtomic.h>
#include <stdlib.h>

#include "common_types.h"

namespace webrtc {

Atomic32::Atomic32(WebRtc_Word32 initialValue) : _value(initialValue)
{
    assert(Is32bitAligned());
}

Atomic32::~Atomic32()
{
}

WebRtc_Word32 Atomic32::operator++()
{
    return OSAtomicIncrement32Barrier(&_value);
}

WebRtc_Word32 Atomic32::operator--()
{
    return OSAtomicDecrement32Barrier(&_value);
}

WebRtc_Word32 Atomic32::operator+=(WebRtc_Word32 value)
{
    return OSAtomicAdd32Barrier(value, &_value);
}

WebRtc_Word32 Atomic32::operator-=(WebRtc_Word32 value)
{
    return OSAtomicAdd32Barrier(-value, &_value);
}

bool Atomic32::CompareExchange(WebRtc_Word32 newValue,
                               WebRtc_Word32 compareValue)
{
    return OSAtomicCompareAndSwap32Barrier(compareValue, newValue, &_value);
}

WebRtc_Word32 Atomic32::Value() const
{
    return _value;
}
}  

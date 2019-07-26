









#include "atomic32.h"

#include <assert.h>
#include <libkern/OSAtomic.h>
#include <stdlib.h>

#include "common_types.h"

namespace webrtc {

Atomic32::Atomic32(WebRtc_Word32 initial_value)
    : value_(initial_value) {
  assert(Is32bitAligned());
}

Atomic32::~Atomic32() {
}

WebRtc_Word32 Atomic32::operator++() {
  return OSAtomicIncrement32Barrier(&value_);
}

WebRtc_Word32 Atomic32::operator--() {
  return OSAtomicDecrement32Barrier(&value_);
}

WebRtc_Word32 Atomic32::operator+=(WebRtc_Word32 value) {
  return OSAtomicAdd32Barrier(value, &value_);
}

WebRtc_Word32 Atomic32::operator-=(WebRtc_Word32 value) {
  return OSAtomicAdd32Barrier(-value, &value_);
}

bool Atomic32::CompareExchange(WebRtc_Word32 new_value,
                               WebRtc_Word32 compare_value) {
  return OSAtomicCompareAndSwap32Barrier(compare_value, new_value, &value_);
}

WebRtc_Word32 Atomic32::Value() const {
  return value_;
}

}  











#include "atomic32.h"

#include <assert.h>
#include <windows.h>

#include "common_types.h"
#include "compile_assert.h"

namespace webrtc {

Atomic32::Atomic32(WebRtc_Word32 initial_value)
    : value_(initial_value) {
  
  
  COMPILE_ASSERT(sizeof(value_) == sizeof(LONG));
  assert(Is32bitAligned());
}

Atomic32::~Atomic32() {
}

WebRtc_Word32 Atomic32::operator++() {
  return static_cast<WebRtc_Word32>(InterlockedIncrement(
      reinterpret_cast<volatile LONG*>(&value_)));
}

WebRtc_Word32 Atomic32::operator--() {
  return static_cast<WebRtc_Word32>(InterlockedDecrement(
      reinterpret_cast<volatile LONG*>(&value_)));
}

WebRtc_Word32 Atomic32::operator+=(WebRtc_Word32 value) {
  return InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(&value_),
                                value);
}

WebRtc_Word32 Atomic32::operator-=(WebRtc_Word32 value) {
  return InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(&value_),
                                -value);
}

bool Atomic32::CompareExchange(WebRtc_Word32 new_value,
                               WebRtc_Word32 compare_value) {
  const LONG old_value = InterlockedCompareExchange(
      reinterpret_cast<volatile LONG*>(&value_),
      new_value,
      compare_value);

  
  return (old_value == compare_value);
}

WebRtc_Word32 Atomic32::Value() const {
  return value_;
}

}  

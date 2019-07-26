














#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_ATOMIC32_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_ATOMIC32_H_

#include <cstddef>

#include "common_types.h"
#include "constructor_magic.h"

namespace webrtc {





class Atomic32 {
 public:
  Atomic32(WebRtc_Word32 initial_value = 0);
  ~Atomic32();

  
  WebRtc_Word32 operator++();
  WebRtc_Word32 operator--();

  WebRtc_Word32 operator+=(WebRtc_Word32 value);
  WebRtc_Word32 operator-=(WebRtc_Word32 value);

  
  
  bool CompareExchange(WebRtc_Word32 new_value, WebRtc_Word32 compare_value);
  WebRtc_Word32 Value() const;

 private:
  
  
  Atomic32 operator+(const Atomic32& other);
  Atomic32 operator-(const Atomic32& other);

  
  inline bool Is32bitAligned() const {
    return (reinterpret_cast<ptrdiff_t>(&value_) & 3) == 0;
  }

  DISALLOW_COPY_AND_ASSIGN(Atomic32);

  WebRtc_Word32 value_;
};

}  

#endif  

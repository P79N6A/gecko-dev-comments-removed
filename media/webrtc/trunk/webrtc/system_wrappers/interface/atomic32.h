













#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_ATOMIC32_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_ATOMIC32_H_

#include <stddef.h>

#include "webrtc/common_types.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"

namespace webrtc {





class Atomic32 {
 public:
  Atomic32(int32_t initial_value = 0);
  ~Atomic32();

  
  int32_t operator++();
  int32_t operator--();

  int32_t operator+=(int32_t value);
  int32_t operator-=(int32_t value);

  
  
  bool CompareExchange(int32_t new_value, int32_t compare_value);
  int32_t Value() {
    return *this += 0;
  }

 private:
  
  
  Atomic32 operator+(const Atomic32& other);
  Atomic32 operator-(const Atomic32& other);

  
  inline bool Is32bitAligned() const {
    return (reinterpret_cast<ptrdiff_t>(&value_) & 3) == 0;
  }

  DISALLOW_COPY_AND_ASSIGN(Atomic32);

  int32_t value_;
};

}  

#endif  

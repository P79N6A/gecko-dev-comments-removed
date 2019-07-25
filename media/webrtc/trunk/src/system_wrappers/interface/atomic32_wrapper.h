












#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_ATOMIC32_WRAPPER_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_ATOMIC32_WRAPPER_H_

#include "common_types.h"

namespace webrtc {
class Atomic32Impl;
class Atomic32Wrapper
{
public:
    Atomic32Wrapper(WebRtc_Word32 initialValue = 0);
    ~Atomic32Wrapper();

    
    WebRtc_Word32 operator++();
    WebRtc_Word32 operator--();

    Atomic32Wrapper& operator=(const Atomic32Wrapper& rhs);
    Atomic32Wrapper& operator=(WebRtc_Word32 rhs);

    WebRtc_Word32 operator+=(WebRtc_Word32 rhs);
    WebRtc_Word32 operator-=(WebRtc_Word32 rhs);

    
    
    bool CompareExchange(WebRtc_Word32 newValue, WebRtc_Word32 compareValue);
    WebRtc_Word32 Value() const;
private:
    
    
    Atomic32Wrapper operator+(const Atomic32Wrapper& rhs);
    Atomic32Wrapper operator-(const Atomic32Wrapper& rhs);

    WebRtc_Word32& operator++(int);
    WebRtc_Word32& operator--(int);

    
    
    Atomic32Impl& _impl;
};
} 
#endif 

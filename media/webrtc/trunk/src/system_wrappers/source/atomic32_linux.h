












#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_ATOMIC32_LINUX_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_ATOMIC32_LINUX_H_

#include <inttypes.h>
#include <malloc.h>

#include "common_types.h"

namespace webrtc {
class Atomic32Impl
{
public:
    inline Atomic32Impl(WebRtc_Word32 initialValue);
    inline ~Atomic32Impl();

    inline WebRtc_Word32 operator++();
    inline WebRtc_Word32 operator--();

    inline Atomic32Impl& operator=(const Atomic32Impl& rhs);
    inline Atomic32Impl& operator=(WebRtc_Word32 rhs);
    inline WebRtc_Word32 operator+=(WebRtc_Word32 rhs);
    inline WebRtc_Word32 operator-=(WebRtc_Word32 rhs);

    inline bool CompareExchange(WebRtc_Word32 newValue,
                                WebRtc_Word32 compareValue);

    inline WebRtc_Word32 Value() const;
private:
    void*        _ptrMemory;
    
    volatile WebRtc_Word32* _value;
};


inline Atomic32Impl::Atomic32Impl(WebRtc_Word32 initialValue)
    : _ptrMemory(NULL),
      _value(NULL)
{   
    
    
    _ptrMemory = malloc(sizeof(WebRtc_Word32)*2);
    _value = (WebRtc_Word32*) (((uintptr_t)_ptrMemory+3)&(~0x3));
    *_value = initialValue;
}

inline Atomic32Impl::~Atomic32Impl()
{
    if(_ptrMemory != NULL)
    {
        free(_ptrMemory);
    }
}

inline WebRtc_Word32 Atomic32Impl::operator++()
{
    WebRtc_Word32 returnValue = __sync_fetch_and_add(_value,1);
    returnValue++;
    return returnValue;
}

inline WebRtc_Word32 Atomic32Impl::operator--()
{
    WebRtc_Word32 returnValue = __sync_fetch_and_sub(_value,1);
    returnValue--;
    return returnValue;
}

inline Atomic32Impl& Atomic32Impl::operator=(const Atomic32Impl& rhs)
{
    *_value = *rhs._value;
    return *this;
}

inline Atomic32Impl& Atomic32Impl::operator=(WebRtc_Word32 rhs)
{
    *_value = rhs;
    return *this;
}

inline WebRtc_Word32 Atomic32Impl::operator+=(WebRtc_Word32 rhs)
{
    WebRtc_Word32 returnValue = __sync_fetch_and_add(_value,rhs);
    returnValue += rhs;
    return returnValue;
}

inline WebRtc_Word32 Atomic32Impl::operator-=(WebRtc_Word32 rhs)
{
    WebRtc_Word32 returnValue = __sync_fetch_and_sub(_value,rhs);
    returnValue -= rhs;
    return returnValue;
}

inline bool Atomic32Impl::CompareExchange(WebRtc_Word32 newValue,
                                          WebRtc_Word32 compareValue)
{
    return __sync_bool_compare_and_swap(_value,compareValue,newValue);
}

inline WebRtc_Word32 Atomic32Impl::Value() const
{
    return *_value;
}
} 

#endif 

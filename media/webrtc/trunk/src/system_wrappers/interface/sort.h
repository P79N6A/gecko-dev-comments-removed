











#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_SORT_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_SORT_H_

#include "typedefs.h"
#include "common_types.h"

namespace webrtc
{
    enum Type
    {
        TYPE_Word8,
        TYPE_UWord8,
        TYPE_Word16,
        TYPE_UWord16,
        TYPE_Word32,
        TYPE_UWord32,
        TYPE_Word64,
        TYPE_UWord64,
        TYPE_Float32,
        TYPE_Float64
    };
    
    
    
    
    
    
    
    
    WebRtc_Word32 Sort(void* data, WebRtc_UWord32 numOfElements, Type dataType);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word32 KeySort(void* data, void* key, WebRtc_UWord32 numOfElements,
                          WebRtc_UWord32 sizeOfElement, Type keyType);
}

#endif 

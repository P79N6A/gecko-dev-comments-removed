











#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_SORT_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_SORT_H_

#include "webrtc/common_types.h"
#include "webrtc/typedefs.h"

namespace webrtc {

enum Type {
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









WebRtc_Word32 Sort(void* data, WebRtc_UWord32 num_of_elements, Type data_type);

















WebRtc_Word32 KeySort(void* data, void* key, WebRtc_UWord32 num_of_elements,
                      WebRtc_UWord32 size_of_element, Type key_type);

}  

#endif  

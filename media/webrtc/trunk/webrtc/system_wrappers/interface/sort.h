











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









int32_t Sort(void* data, uint32_t num_of_elements, Type data_type);

















int32_t KeySort(void* data, void* key, uint32_t num_of_elements,
                uint32_t size_of_element, Type key_type);

}  

#endif  

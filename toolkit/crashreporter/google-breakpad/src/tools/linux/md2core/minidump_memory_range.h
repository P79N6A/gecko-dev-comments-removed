

































#ifndef TOOLS_LINUX_MD2CORE_MINIDUMP_MEMORY_RANGE_H_
#define TOOLS_LINUX_MD2CORE_MINIDUMP_MEMORY_RANGE_H_

#include <string>

#include "common/memory_range.h"
#include "google_breakpad/common/minidump_format.h"

namespace google_breakpad {




class MinidumpMemoryRange : public MemoryRange {
 public:
  MinidumpMemoryRange() {}

  MinidumpMemoryRange(const void* data, size_t length)
      : MemoryRange(data, length) {}

  
  
  
  
  MinidumpMemoryRange Subrange(size_t sub_offset, size_t sub_length) const {
    if (Covers(sub_offset, sub_length))
      return MinidumpMemoryRange(data() + sub_offset, sub_length);
    return MinidumpMemoryRange();
  }

  
  
  MinidumpMemoryRange Subrange(const MDLocationDescriptor& location) const {
    return MinidumpMemoryRange::Subrange(location.rva, location.data_size);
  }

  
  
  
  const std::string GetAsciiMDString(size_t sub_offset) const {
    std::string str;
    const MDString* md_str = GetData<MDString>(sub_offset);
    if (md_str) {
      const uint16_t* buffer = &md_str->buffer[0];
      for (uint32_t i = 0; i < md_str->length && buffer[i]; ++i) {
        str.push_back(buffer[i]);
      }
    }
    return str;
  }
};

}  

#endif  

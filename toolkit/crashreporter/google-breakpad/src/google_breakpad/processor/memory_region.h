




































#ifndef GOOGLE_BREAKPAD_PROCESSOR_MEMORY_REGION_H__
#define GOOGLE_BREAKPAD_PROCESSOR_MEMORY_REGION_H__


#include "google_breakpad/common/breakpad_types.h"


namespace google_breakpad {


class MemoryRegion {
 public:
  virtual ~MemoryRegion() {}

  
  virtual uint64_t GetBase() const = 0;

  
  virtual uint32_t GetSize() const = 0;

  
  
  
  
  
  
  
  
  virtual bool GetMemoryAtAddress(uint64_t address, uint8_t*  value) const = 0;
  virtual bool GetMemoryAtAddress(uint64_t address, uint16_t* value) const = 0;
  virtual bool GetMemoryAtAddress(uint64_t address, uint32_t* value) const = 0;
  virtual bool GetMemoryAtAddress(uint64_t address, uint64_t* value) const = 0;
};


}  


#endif  

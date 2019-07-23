




































#ifndef GOOGLE_BREAKPAD_PROCESSOR_MEMORY_REGION_H__
#define GOOGLE_BREAKPAD_PROCESSOR_MEMORY_REGION_H__


#include "google_breakpad/common/breakpad_types.h"


namespace google_breakpad {


class MemoryRegion {
 public:
  virtual ~MemoryRegion() {}

  
  virtual u_int64_t GetBase() = 0;

  
  virtual u_int32_t GetSize() = 0;

  
  
  
  
  
  
  
  
  virtual bool GetMemoryAtAddress(u_int64_t address, u_int8_t*  value) = 0;
  virtual bool GetMemoryAtAddress(u_int64_t address, u_int16_t* value) = 0;
  virtual bool GetMemoryAtAddress(u_int64_t address, u_int32_t* value) = 0;
  virtual bool GetMemoryAtAddress(u_int64_t address, u_int64_t* value) = 0;
};


}  


#endif  

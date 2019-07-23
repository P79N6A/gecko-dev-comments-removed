




































#ifndef GOOGLE_AIRBAG_PROCESSOR_MEMORY_REGION_H__
#define GOOGLE_AIRBAG_PROCESSOR_MEMORY_REGION_H__


#include "google_airbag/common/airbag_types.h"


namespace google_airbag {


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

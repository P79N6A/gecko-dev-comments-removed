
























































#ifndef COMMON_ARM_EX_READER_H__
#define COMMON_ARM_EX_READER_H__

#include "common/arm_ex_to_module.h"
#include "common/memory_range.h"

namespace arm_ex_reader {



class ExceptionTableInfo {
 public:
  ExceptionTableInfo(const char* exidx, size_t exidx_size,
                     const char* extab, size_t extab_size,
                     uint32_t text_last_svma,
                     arm_ex_to_module::ARMExToModule* handler,
                     const char* mapping_addr,
                     uint32_t loading_addr)
      : mr_exidx_(google_breakpad::MemoryRange(exidx, exidx_size)),
        mr_extab_(google_breakpad::MemoryRange(extab, extab_size)),
        text_last_svma_(text_last_svma),
        handler_(handler), mapping_addr_(mapping_addr),
        loading_addr_(loading_addr) { }

  ~ExceptionTableInfo() { }

  
  
  
  void Start();

 private:
  google_breakpad::MemoryRange mr_exidx_;
  google_breakpad::MemoryRange mr_extab_;
  uint32_t text_last_svma_;
  arm_ex_to_module::ARMExToModule* handler_;
  const char* mapping_addr_;
  uint32_t loading_addr_;

  enum ExExtractResult {
    ExSuccess,        
    ExInBufOverflow,  
    ExOutBufOverflow, 
    ExCantUnwind,     
    ExCantRepresent,  
    ExInvalid         
  };
  ExExtractResult
    ExtabEntryExtract(const struct arm_ex_to_module::exidx_entry* entry,
                      uint8_t* buf, size_t buf_size,
                      size_t* buf_used);

  int ExtabEntryDecode(const uint8_t* buf, size_t buf_size);
};

} 

#endif 

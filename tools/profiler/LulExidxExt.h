
































































#ifndef LulExidxExt_h
#define LulExidxExt_h

#include "LulMainInt.h"

using lul::LExpr;
using lul::RuleSet;
using lul::SecMap;

namespace lul {

typedef enum extab_cmd {
  ARM_EXIDX_CMD_FINISH,
  ARM_EXIDX_CMD_SUB_FROM_VSP,
  ARM_EXIDX_CMD_ADD_TO_VSP,
  ARM_EXIDX_CMD_REG_POP,
  ARM_EXIDX_CMD_REG_TO_SP,
  ARM_EXIDX_CMD_VFP_POP,
  ARM_EXIDX_CMD_WREG_POP,
  ARM_EXIDX_CMD_WCGR_POP,
  ARM_EXIDX_CMD_RESERVED,
  ARM_EXIDX_CMD_REFUSED,
} extab_cmd_t;

struct exidx_entry {
  uint32_t addr;
  uint32_t data;
};

struct extab_data {
  extab_cmd_t cmd;
  uint32_t data;
};

enum extab_cmd_flags {
  ARM_EXIDX_VFP_SHIFT_16 = 1 << 16,
  ARM_EXIDX_VFP_FSTMD = 1 << 17, 
};




class ARMExToModule {
 public:
   ARMExToModule(SecMap* smap, void(*log)(const char*)) : smap_(smap)
                                                        , log_(log) { }
  ~ARMExToModule() { }
  void AddStackFrame(uintptr_t addr, size_t size);
  int  ImproveStackFrame(const struct extab_data* edata);
  void DeleteStackFrame();
  void SubmitStackFrame();
 private:
  SecMap* smap_;
  LExpr   vsp_;        
  RuleSet curr_rules_; 
  
  void (*log_)(const char*);
  int TranslateCmd(const struct extab_data* edata, LExpr& vsp);
};












class MemoryRange {
 public:

  MemoryRange(const void* data, size_t length) {
    Set(data, length);
  }

  
  void Set(const void* data, size_t length) {
    data_ = reinterpret_cast<const uint8_t*>(data);
    
    length_ = data ? length : 0;
  }

  
  
  bool Covers(size_t sub_offset, size_t sub_length) const {
    
    
    
    
    return sub_offset < length_ &&
           sub_offset + sub_length >= sub_offset &&
           sub_offset + sub_length <= length_;
  }

  
  const uint8_t* data() const { return data_; }

  
  size_t length() const { return length_; }

 private:
  
  const uint8_t* data_;

  
  size_t length_;
};




class ExceptionTableInfo {
 public:
  ExceptionTableInfo(const char* exidx_avma, size_t exidx_size,
                     const char* extab_avma, size_t extab_size,
                     uint32_t text_last_avma,
                     lul::ARMExToModule* handler,
                     void (*log)(const char*))
      : mr_exidx_avma_(lul::MemoryRange(exidx_avma, exidx_size)),
        mr_extab_avma_(lul::MemoryRange(extab_avma, extab_size)),
        text_last_avma_(text_last_avma),
        handler_(handler),
        log_(log) { }

  ~ExceptionTableInfo() { }

  
  
  
  void Start();

 private:
  
  lul::MemoryRange mr_exidx_avma_;
  lul::MemoryRange mr_extab_avma_;
  
  uint32_t text_last_avma_;
  lul::ARMExToModule* handler_;
  
  void (*log_)(const char*);
  enum ExExtractResult {
    ExSuccess,        
    ExInBufOverflow,  
    ExOutBufOverflow, 
    ExCantUnwind,     
    ExCantRepresent,  
    ExInvalid         
  };
  ExExtractResult
    ExtabEntryExtract(const struct lul::exidx_entry* entry,
                      uint8_t* buf, size_t buf_size,
                      size_t* buf_used);

  int ExtabEntryDecode(const uint8_t* buf, size_t buf_size);
};

} 

#endif 

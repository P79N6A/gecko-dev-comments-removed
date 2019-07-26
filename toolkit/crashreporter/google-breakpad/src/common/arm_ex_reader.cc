

























































#include "common/arm_ex_reader.h"
#include "common/logging.h"

#include <assert.h>

































#define ARM_EXIDX_CANT_UNWIND 0x00000001
#define ARM_EXIDX_COMPACT     0x80000000
#define ARM_EXTBL_OP_FINISH   0xb0
#define ARM_EXIDX_TABLE_LIMIT (255*4)

namespace arm_ex_reader {

using arm_ex_to_module::ARM_EXIDX_CMD_FINISH;
using arm_ex_to_module::ARM_EXIDX_CMD_SUB_FROM_VSP;
using arm_ex_to_module::ARM_EXIDX_CMD_ADD_TO_VSP;
using arm_ex_to_module::ARM_EXIDX_CMD_REG_POP;
using arm_ex_to_module::ARM_EXIDX_CMD_REG_TO_SP;
using arm_ex_to_module::ARM_EXIDX_CMD_VFP_POP;
using arm_ex_to_module::ARM_EXIDX_CMD_WREG_POP;
using arm_ex_to_module::ARM_EXIDX_CMD_WCGR_POP;
using arm_ex_to_module::ARM_EXIDX_CMD_RESERVED;
using arm_ex_to_module::ARM_EXIDX_CMD_REFUSED;
using arm_ex_to_module::exidx_entry;
using arm_ex_to_module::ARM_EXIDX_VFP_SHIFT_16;
using arm_ex_to_module::ARM_EXIDX_VFP_FSTMD;
using google_breakpad::MemoryRange;


static void* Prel31ToAddr(const void* addr) 
{
  uint32_t offset32 = *reinterpret_cast<const uint32_t*>(addr);
  
  
  uint64_t offset64 = offset32;
  if (offset64 & (1ULL << 30))
    offset64 |= 0xFFFFFFFF80000000ULL;
  else
    offset64 &= 0x000000007FFFFFFFULL;
  return ((char*)addr) + (uintptr_t)offset64;
}






ExceptionTableInfo::ExExtractResult
ExceptionTableInfo::ExtabEntryExtract(const struct exidx_entry* entry,
                                      uint8_t* buf, size_t buf_size,
                                      size_t* buf_used)
{
  MemoryRange mr_out(buf, buf_size);

  *buf_used = 0;

# define PUT_BUF_U8(_byte) \
  do { if (!mr_out.Covers(*buf_used, 1)) return ExOutBufOverflow; \
       buf[(*buf_used)++] = (_byte); } while (0)

# define GET_EX_U32(_lval, _addr, _sec_mr) \
  do { if (!(_sec_mr).Covers(reinterpret_cast<const uint8_t*>(_addr) \
                             - (_sec_mr).data(), 4)) \
         return ExInBufOverflow; \
       (_lval) = *(reinterpret_cast<const uint32_t*>(_addr)); } while (0)

# define GET_EXIDX_U32(_lval, _addr) \
            GET_EX_U32(_lval, _addr, mr_exidx_)
# define GET_EXTAB_U32(_lval, _addr) \
            GET_EX_U32(_lval, _addr, mr_extab_)

  uint32_t data;
  GET_EXIDX_U32(data, &entry->data);

  
  
  if (data == ARM_EXIDX_CANT_UNWIND)
    return ExCantUnwind;

  uint32_t  pers;          
  uint32_t  extra;         
  uint32_t  extra_allowed; 
  uint32_t* extbl_data;    

  if (data & ARM_EXIDX_COMPACT) {
    
    
    
    
    
    
    
    
    extbl_data = NULL;
    pers  = (data >> 24) & 0x0F;
    extra = (data >> 16) & 0xFF;
    extra_allowed = 0;
  }
  else {
    
    
    
    extbl_data = reinterpret_cast<uint32_t*>(Prel31ToAddr(&entry->data));
    GET_EXTAB_U32(data, extbl_data);
    if (!(data & ARM_EXIDX_COMPACT)) {
      
      
      
      return ExCantRepresent;
    }
    
    
    pers  = (data >> 24) & 0x0F;
    extra = (data >> 16) & 0xFF;
    extra_allowed = 255;
    extbl_data++;
  }

  
  
  
  
  
  if (pers == 0) {
    
    
    PUT_BUF_U8(data >> 16);
    PUT_BUF_U8(data >> 8);
    PUT_BUF_U8(data);
  }
  else if ((pers == 1 || pers == 2) && extra <= extra_allowed) {
    
    
    PUT_BUF_U8(data >> 8);
    PUT_BUF_U8(data);
    for (uint32_t j = 0; j < extra; j++) {
      GET_EXTAB_U32(data, extbl_data);
      extbl_data++;
      PUT_BUF_U8(data >> 24);
      PUT_BUF_U8(data >> 16);
      PUT_BUF_U8(data >> 8);
      PUT_BUF_U8(data >> 0);
    }
  }
  else {
    
    return ExInvalid;
  }

  
  if (*buf_used > 0 && buf[(*buf_used) - 1] != ARM_EXTBL_OP_FINISH)
    PUT_BUF_U8(ARM_EXTBL_OP_FINISH);

  return ExSuccess;

# undef GET_EXTAB_U32
# undef GET_EXIDX_U32
# undef GET_U32
# undef PUT_BUF_U8
}











int ExceptionTableInfo::ExtabEntryDecode(const uint8_t* buf, size_t buf_size)
{
  if (buf == NULL || buf_size == 0)
    return -1;

  MemoryRange mr_in(buf, buf_size);
  const uint8_t* buf_initially = buf;

# define GET_BUF_U8(_lval) \
  do { if (!mr_in.Covers(buf - buf_initially, 1)) return -1; \
       (_lval) = *(buf++); } while (0)

  const uint8_t* end = buf + buf_size;

  while (buf < end) {
    struct arm_ex_to_module::extab_data edata;
    memset(&edata, 0, sizeof(edata));

    uint8_t op;
    GET_BUF_U8(op);
    if ((op & 0xc0) == 0x00) {
      
      edata.cmd = ARM_EXIDX_CMD_ADD_TO_VSP;
      edata.data = (((int)op & 0x3f) << 2) + 4;
    }
    else if ((op & 0xc0) == 0x40) {
      
      edata.cmd = ARM_EXIDX_CMD_SUB_FROM_VSP;
      edata.data = (((int)op & 0x3f) << 2) + 4;
    }
    else if ((op & 0xf0) == 0x80) {
      uint8_t op2;
      GET_BUF_U8(op2);
      if (op == 0x80 && op2 == 0x00) {
        
        edata.cmd = ARM_EXIDX_CMD_REFUSED;
      } else {
        
        edata.cmd = ARM_EXIDX_CMD_REG_POP;
        edata.data = ((op & 0xf) << 8) | op2;
        edata.data = edata.data << 4;
      }
    }
    else if ((op & 0xf0) == 0x90) {
      if (op == 0x9d || op == 0x9f) {
        
        
        edata.cmd = ARM_EXIDX_CMD_RESERVED;
      } else {
        
        edata.cmd = ARM_EXIDX_CMD_REG_TO_SP;
        edata.data = op & 0x0f;
      }
    }
    else if ((op & 0xf0) == 0xa0) {
      
      
      unsigned end = (op & 0x07);
      edata.data = (1 << (end + 1)) - 1;
      edata.data = edata.data << 4;
      if (op & 0x08) edata.data |= 1 << 14;
      edata.cmd = ARM_EXIDX_CMD_REG_POP;
    }
    else if (op == ARM_EXTBL_OP_FINISH) {
      
      edata.cmd = ARM_EXIDX_CMD_FINISH;
      buf = end;
    }
    else if (op == 0xb1) {
      uint8_t op2;
      GET_BUF_U8(op2);
      if (op2 == 0 || (op2 & 0xf0)) {
        
        edata.cmd = ARM_EXIDX_CMD_RESERVED;
      } else {
        
        edata.cmd = ARM_EXIDX_CMD_REG_POP;
        edata.data = op2 & 0x0f;
      }
    }
    else if (op == 0xb2) {
      
      uint64_t offset = 0;
      uint8_t byte, shift = 0;
      do {
        GET_BUF_U8(byte);
        offset |= (byte & 0x7f) << shift;
        shift += 7;
      } while ((byte & 0x80) && buf < end);
      edata.data = offset * 4 + 0x204;
      edata.cmd = ARM_EXIDX_CMD_ADD_TO_VSP;
    }
    else if (op == 0xb3 || op == 0xc8 || op == 0xc9) {
      
      
      
      edata.cmd = ARM_EXIDX_CMD_VFP_POP;
      GET_BUF_U8(edata.data);
      if (op == 0xc8) edata.data |= ARM_EXIDX_VFP_SHIFT_16;
      if (op != 0xb3) edata.data |= ARM_EXIDX_VFP_FSTMD;
    }
    else if ((op & 0xf8) == 0xb8 || (op & 0xf8) == 0xd0) {
      
      
      edata.cmd = ARM_EXIDX_CMD_VFP_POP;
      edata.data = 0x80 | (op & 0x07);
      if ((op & 0xf8) == 0xd0) edata.data |= ARM_EXIDX_VFP_FSTMD;
    }
    else if (op >= 0xc0 && op <= 0xc5) {
      
      edata.cmd = ARM_EXIDX_CMD_WREG_POP;
      edata.data = 0xa0 | (op & 0x07);
    }
    else if (op == 0xc6) {
      
      edata.cmd = ARM_EXIDX_CMD_WREG_POP;
      GET_BUF_U8(edata.data);
    }
    else if (op == 0xc7) {
      uint8_t op2;
      GET_BUF_U8(op2);
      if (op2 == 0 || (op2 & 0xf0)) {
        
        edata.cmd = ARM_EXIDX_CMD_RESERVED;
      } else {
        
        edata.cmd = ARM_EXIDX_CMD_WCGR_POP;
        edata.data = op2 & 0x0f;
      }
    }
    else {
      
      edata.cmd = ARM_EXIDX_CMD_RESERVED;
    }

    int ret = handler_->ImproveStackFrame(&edata);
    if (ret < 0) return ret;
  }
  return 0;

# undef GET_BUF_U8
}

void ExceptionTableInfo::Start()
{
  const struct exidx_entry* start
    = reinterpret_cast<const struct exidx_entry*>(mr_exidx_.data());
  const struct exidx_entry* end
    = reinterpret_cast<const struct exidx_entry*>(mr_exidx_.data()
                                                  + mr_exidx_.length());

  
  
  for (const struct exidx_entry* entry = start; entry < end; ++entry) {

    
    
    uint32_t addr = (reinterpret_cast<char*>(Prel31ToAddr(&entry->addr))
                     - mapping_addr_ + loading_addr_) & 0x7fffffff;
    uint32_t next_addr;
    if (entry < end - 1)
      next_addr = (reinterpret_cast<char*>(Prel31ToAddr(&((entry + 1)->addr)))
                   - mapping_addr_ + loading_addr_) & 0x7fffffff;
    else {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      bool plausible = false;
      next_addr = addr + 1;
      if (text_last_svma_ != 0) {
        uint32_t maybe_next_addr = text_last_svma_ + 1;
        if (maybe_next_addr > addr && maybe_next_addr - addr <= 4096) {
          next_addr = maybe_next_addr;
          plausible = true;
        }
      }
      if (!plausible)
        BPLOG(INFO) << "ExceptionTableInfo: implausible EXIDX last entry size "
                    << (int32_t)(text_last_svma_ - addr)
                    << "; using 1 instead.";
    }

    
    
    
    
    uint8_t buf[ARM_EXIDX_TABLE_LIMIT];
    size_t buf_used = 0;
    ExExtractResult res = ExtabEntryExtract(entry, buf, sizeof(buf), &buf_used);
    if (res != ExSuccess) {
      
      switch (res) {
        case ExInBufOverflow:
          BPLOG(INFO) << "ExtabEntryExtract: .exidx/.extab section overrun";
          break;
        case ExOutBufOverflow:
          BPLOG(INFO) << "ExtabEntryExtract: bytecode buffer overflow";
          break;
        case ExCantUnwind:
          BPLOG(INFO) << "ExtabEntryExtract: function is marked CANT_UNWIND";
          break;
        case ExCantRepresent:
          BPLOG(INFO) << "ExtabEntryExtract: bytecode can't be represented";
          break;
        case ExInvalid:
          BPLOG(INFO) << "ExtabEntryExtract: index table entry is invalid";
          break;
        default:
          BPLOG(INFO) << "ExtabEntryExtract: unknown error: " << (int)res;
          break;
      }
      continue;
    }

    
    
    
    
    if (!handler_->HasStackFrame(addr, next_addr - addr)) {
      handler_->AddStackFrame(addr, next_addr - addr);
      int ret = ExtabEntryDecode(buf, buf_used);
      if (ret < 0) {
	handler_->DeleteStackFrame();
	BPLOG(INFO) << "ExtabEntryDecode: failed with error code: " << ret;
	continue;
      }
      handler_->SubmitStackFrame();
    }

  } 
}

} 

































































































#include "mozilla/Assertions.h"

#include "LulExidxExt.h"


#define ARM_EXBUF_START(x) (((x) >> 4) & 0x0f)
#define ARM_EXBUF_COUNT(x) ((x) & 0x0f)
#define ARM_EXBUF_END(x)   (ARM_EXBUF_START(x) + ARM_EXBUF_COUNT(x))

namespace lul {


int ARMExToModule::TranslateCmd(const struct extab_data* edata,
                                LExpr& vsp) {
  int ret = 0;
  switch (edata->cmd) {
    case ARM_EXIDX_CMD_FINISH:
      
      if (curr_rules_.mR15expr.mHow == UNKNOWN) {
        if (curr_rules_.mR14expr.mHow == UNKNOWN) {
          curr_rules_.mR15expr = LExpr(NODEREF, DW_REG_ARM_R14, 0);
        } else {
          curr_rules_.mR15expr = curr_rules_.mR14expr;
        }
      }
      break;
    case ARM_EXIDX_CMD_SUB_FROM_VSP:
      vsp = vsp.add_delta(- static_cast<long>(edata->data));
      break;
    case ARM_EXIDX_CMD_ADD_TO_VSP:
      vsp = vsp.add_delta(static_cast<long>(edata->data));
      break;
    case ARM_EXIDX_CMD_REG_POP:
      for (unsigned int i = 0; i < 16; i++) {
        if (edata->data & (1 << i)) {
          
          
          
          
          
          LExpr* regI_exprP = curr_rules_.ExprForRegno((DW_REG_NUMBER)i);
          if (regI_exprP) {
            *regI_exprP = vsp.deref();
          }
          vsp = vsp.add_delta(4);
        }
      }
      
      if (edata->data & (1 << 13)) {
        vsp = curr_rules_.mR13expr;
      }
      break;
    case ARM_EXIDX_CMD_REG_TO_SP: {
      MOZ_ASSERT (edata->data < 16);
      int    reg_no    = edata->data;
      
      LExpr* reg_exprP = curr_rules_.ExprForRegno((DW_REG_NUMBER)reg_no);
      if (reg_exprP) {
        if (reg_exprP->mHow == UNKNOWN) {
          curr_rules_.mR13expr = LExpr(NODEREF, reg_no, 0);
        } else {
          curr_rules_.mR13expr = *reg_exprP;
        }
        vsp = curr_rules_.mR13expr;
      }
      break;
    }
    case ARM_EXIDX_CMD_VFP_POP:
      

      for (unsigned int i = ARM_EXBUF_START(edata->data);
           i <= ARM_EXBUF_END(edata->data); i++) {
        vsp = vsp.add_delta(8);
      }
      if (!(edata->data & ARM_EXIDX_VFP_FSTMD)) {
        vsp = vsp.add_delta(4);
      }
      break;
    case ARM_EXIDX_CMD_WREG_POP:
      for (unsigned int i = ARM_EXBUF_START(edata->data);
           i <= ARM_EXBUF_END(edata->data); i++) {
        vsp = vsp.add_delta(8);
      }
      break;
    case ARM_EXIDX_CMD_WCGR_POP:
      
      for (unsigned int i = 0; i < 4; i++) {
        if (edata->data & (1 << i)) {
          vsp = vsp.add_delta(4);
        }
      }
      break;
    case ARM_EXIDX_CMD_REFUSED:
    case ARM_EXIDX_CMD_RESERVED:
      ret = -1;
      break;
  }
  return ret;
}

void ARMExToModule::AddStackFrame(uintptr_t addr, size_t size) {
  
  
  
  vsp_ = LExpr(NODEREF, DW_REG_ARM_R13, 0);
  (void) new (&curr_rules_) RuleSet();
  curr_rules_.mAddr = (uintptr_t)addr;
  curr_rules_.mLen  = (uintptr_t)size;
  if (0) {
    char buf[100];
    sprintf(buf, "  AddStackFrame    %llx .. %llx",
            (uint64_t)addr, (uint64_t)(addr + size - 1));
    log_(buf);
  }
}

int ARMExToModule::ImproveStackFrame(const struct extab_data* edata) {
  return TranslateCmd(edata, vsp_) ;
}

void ARMExToModule::DeleteStackFrame() {
}

void ARMExToModule::SubmitStackFrame() {
  
  
  
  
  

  
  curr_rules_.mR13expr = vsp_;

  
  if (curr_rules_.mLen > 0) {

    
    
    


    if (curr_rules_.mR7expr.mHow == UNKNOWN) {
      curr_rules_.mR7expr = LExpr(NODEREF, DW_REG_ARM_R7, 0);
    }
    if (curr_rules_.mR11expr.mHow == UNKNOWN) {
      curr_rules_.mR11expr = LExpr(NODEREF, DW_REG_ARM_R11, 0);
    }
    if (curr_rules_.mR12expr.mHow == UNKNOWN) {
      curr_rules_.mR12expr = LExpr(NODEREF, DW_REG_ARM_R12, 0);
    }
    if (curr_rules_.mR14expr.mHow == UNKNOWN) {
      curr_rules_.mR14expr = LExpr(NODEREF, DW_REG_ARM_R14, 0);
    }

    
    smap_->AddRuleSet(&curr_rules_);

    if (0) {
      curr_rules_.Print(log_);
    }
    if (0) {
      char buf[100];
      sprintf(buf, "  SubmitStackFrame %llx .. %llx",
              (uint64_t)curr_rules_.mAddr,
              (uint64_t)(curr_rules_.mAddr + curr_rules_.mLen - 1));
      log_(buf);
    }
  }
}


#define ARM_EXIDX_CANT_UNWIND 0x00000001
#define ARM_EXIDX_COMPACT     0x80000000
#define ARM_EXTBL_OP_FINISH   0xb0
#define ARM_EXIDX_TABLE_LIMIT (255*4)

using lul::ARM_EXIDX_CMD_FINISH;
using lul::ARM_EXIDX_CMD_SUB_FROM_VSP;
using lul::ARM_EXIDX_CMD_ADD_TO_VSP;
using lul::ARM_EXIDX_CMD_REG_POP;
using lul::ARM_EXIDX_CMD_REG_TO_SP;
using lul::ARM_EXIDX_CMD_VFP_POP;
using lul::ARM_EXIDX_CMD_WREG_POP;
using lul::ARM_EXIDX_CMD_WCGR_POP;
using lul::ARM_EXIDX_CMD_RESERVED;
using lul::ARM_EXIDX_CMD_REFUSED;
using lul::exidx_entry;
using lul::ARM_EXIDX_VFP_SHIFT_16;
using lul::ARM_EXIDX_VFP_FSTMD;
using lul::MemoryRange;


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
            GET_EX_U32(_lval, _addr, mr_exidx_avma_)
# define GET_EXTAB_U32(_lval, _addr) \
            GET_EX_U32(_lval, _addr, mr_extab_avma_)

  uint32_t data;
  GET_EXIDX_U32(data, &entry->data);

  
  
  if (data == ARM_EXIDX_CANT_UNWIND)
    return ExCantUnwind;

  uint32_t  pers;          
  uint32_t  extra;         
  uint32_t  extra_allowed; 
  uint32_t* extbl_data;    

  if (data & ARM_EXIDX_COMPACT) {
    
    
    
    
    
    
    
    
    extbl_data = nullptr;
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
  if (buf == nullptr || buf_size == 0)
    return -1;

  MemoryRange mr_in(buf, buf_size);
  const uint8_t* buf_initially = buf;

# define GET_BUF_U8(_lval) \
  do { if (!mr_in.Covers(buf - buf_initially, 1)) return -1; \
       (_lval) = *(buf++); } while (0)

  const uint8_t* end = buf + buf_size;

  while (buf < end) {
    struct lul::extab_data edata;
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
    = reinterpret_cast<const struct exidx_entry*>(mr_exidx_avma_.data());
  const struct exidx_entry* end
    = reinterpret_cast<const struct exidx_entry*>(mr_exidx_avma_.data()
                                                  + mr_exidx_avma_.length());

  
  
  for (const struct exidx_entry* entry = start; entry < end; ++entry) {

    
    
    uint32_t avma = reinterpret_cast<uint32_t>(Prel31ToAddr(&entry->addr));
    uint32_t next_avma;
    if (entry < end - 1) {
      next_avma
        = reinterpret_cast<uint32_t>(Prel31ToAddr(&((entry + 1)->addr)));
    } else {
      
      
      
      
      
      
      
      
      
      
      
      
      
      

      bool plausible;
      uint32_t maybe_next_avma = text_last_avma_ + 1;
      if (maybe_next_avma > avma && maybe_next_avma - avma <= 4096) {
        next_avma = maybe_next_avma;
        plausible = true;
      } else {
        next_avma = avma + 1;
        plausible = false;
      }

      if (!plausible && avma != text_last_avma_ + 1) {
        char buf[100];
        snprintf(buf, sizeof(buf),
                 "ExceptionTableInfo: implausible EXIDX last entry size %d"
                 "; using 1 instead.", (int32_t)(text_last_avma_ - avma));
        buf[sizeof(buf)-1] = 0;
        log_(buf);
      }
    }

    
    
    
    
    uint8_t buf[ARM_EXIDX_TABLE_LIMIT];
    size_t buf_used = 0;
    ExExtractResult res = ExtabEntryExtract(entry, buf, sizeof(buf), &buf_used);
    if (res != ExSuccess) {
      
      switch (res) {
        case ExInBufOverflow:
          log_("ExtabEntryExtract: .exidx/.extab section overrun");
          break;
        case ExOutBufOverflow:
          log_("ExtabEntryExtract: bytecode buffer overflow");
          break;
        case ExCantUnwind:
          log_("ExtabEntryExtract: function is marked CANT_UNWIND");
          break;
        case ExCantRepresent:
          log_("ExtabEntryExtract: bytecode can't be represented");
          break;
        case ExInvalid:
          log_("ExtabEntryExtract: index table entry is invalid");
          break;
        default: {
          char buf[100];
          snprintf(buf, sizeof(buf),
                   "ExtabEntryExtract: unknown error: %d", (int)res);
          buf[sizeof(buf)-1] = 0;
          log_(buf);
          break;
        }
      }
      continue;
    }

    
    
    
    
    handler_->AddStackFrame(avma, next_avma - avma);
    int ret = ExtabEntryDecode(buf, buf_used);
    if (ret < 0) {
      handler_->DeleteStackFrame();
      char buf[100];
      snprintf(buf, sizeof(buf),
               "ExtabEntryDecode: failed with error code: %d", ret);
      buf[sizeof(buf)-1] = 0;
      log_(buf);
      continue;
    }
    handler_->SubmitStackFrame();
  } 
}

} 

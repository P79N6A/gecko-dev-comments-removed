





#ifndef LulMainInt_h
#define LulMainInt_h

#include "LulPlatformMacros.h"
#include "LulMain.h" 

#include <vector>

#include "mozilla/Assertions.h"






namespace lul {

using std::vector;








enum DW_REG_NUMBER {
  
  
  
  DW_REG_CFA = -1,
#if defined(LUL_ARCH_arm)
  
  DW_REG_ARM_R7  = 7,
  DW_REG_ARM_R11 = 11,
  DW_REG_ARM_R12 = 12,
  DW_REG_ARM_R13 = 13,
  DW_REG_ARM_R14 = 14,
  DW_REG_ARM_R15 = 15,
#elif defined(LUL_ARCH_x64)
  
  
  DW_REG_INTEL_XBP = 6,
  DW_REG_INTEL_XSP = 7,
  DW_REG_INTEL_XIP = 16,
#elif defined(LUL_ARCH_x86)
  DW_REG_INTEL_XBP = 5,
  DW_REG_INTEL_XSP = 4,
  DW_REG_INTEL_XIP = 8,
#else
# error "Unknown arch"
#endif
};






enum PfxExprOp {
  
  PX_Start,   
  PX_End,     
  PX_SImm32,  
  PX_DwReg,   
  PX_Deref,   
  PX_Add,     
  PX_Sub,     
  PX_And,     
  PX_Or,      
  PX_CmpGES,  
  PX_Shl      
};

struct PfxInstr {
  PfxInstr(PfxExprOp opcode, int32_t operand)
    : mOpcode(opcode)
    , mOperand(operand)
  {}
  explicit PfxInstr(PfxExprOp opcode)
    : mOpcode(opcode)
    , mOperand(0)
  {}
  bool operator==(const PfxInstr& other) {
    return mOpcode == other.mOpcode && mOperand == other.mOperand;
  }
  PfxExprOp mOpcode;
  int32_t   mOperand;
};

static_assert(sizeof(PfxInstr) <= 8, "PfxInstr size changed unexpectedly");






TaggedUWord EvaluatePfxExpr(int32_t start,
                            const UnwindRegs* aOldRegs,
                            TaggedUWord aCFA, const StackImage* aStackImg,
                            const vector<PfxInstr>& aPfxInstrs);














enum LExprHow {
  UNKNOWN=0, 
  NODEREF,   
  DEREF,     
  PFXEXPR    
};

inline static const char* NameOf_LExprHow(LExprHow how) {
  switch (how) {
    case UNKNOWN: return "UNKNOWN";
    case NODEREF: return "NODEREF";
    case DEREF:   return "DEREF";
    case PFXEXPR: return "PFXEXPR";
    default:      return "LExpr-??";
  }
}


struct LExpr {
  
  LExpr()
    : mHow(UNKNOWN)
    , mReg(0)
    , mOffset(0)
  {}

  
  LExpr(LExprHow how, int16_t reg, int32_t offset)
    : mHow(how)
    , mReg(reg)
    , mOffset(offset)
  {
    switch (how) {
      case UNKNOWN: MOZ_ASSERT(reg == 0 && offset == 0); break;
      case NODEREF: break;
      case DEREF:   break;
      case PFXEXPR: MOZ_ASSERT(reg == 0 && offset >= 0); break;
      default:      MOZ_ASSERT(0, "LExpr::LExpr: invalid how");
    }
  }

  
  LExpr add_delta(long delta)
  {
    MOZ_ASSERT(mHow == NODEREF);
    
    
    
    return (mHow == NODEREF) ? LExpr(mHow, mReg, mOffset+delta)
                             : LExpr(); 
  }

  
  LExpr deref()
  {
    MOZ_ASSERT(mHow == NODEREF);
    
    return (mHow == NODEREF) ? LExpr(DEREF, mReg, mOffset)
                             : LExpr(); 
  }

  
  
  string ShowRule(const char* aNewReg) const;

  
  
  
  
  
  
  
  TaggedUWord EvaluateExpr(const UnwindRegs* aOldRegs,
                           TaggedUWord aCFA, const StackImage* aStackImg,
                           const vector<PfxInstr>* aPfxInstrs) const;

  
  
  
  LExprHow mHow:8;
  int16_t  mReg;    
  int32_t  mOffset; 
};

static_assert(sizeof(LExpr) <= 8, "LExpr size changed unexpectedly");














































class RuleSet {
public:
  RuleSet();
  void   Print(void(*aLog)(const char*)) const;

  
  LExpr* ExprForRegno(DW_REG_NUMBER aRegno);

  uintptr_t mAddr;
  uintptr_t mLen;
  
  LExpr  mCfaExpr;
  
  
#if defined(LUL_ARCH_x64) || defined(LUL_ARCH_x86)
  LExpr  mXipExpr; 
  LExpr  mXspExpr;
  LExpr  mXbpExpr;
#elif defined(LUL_ARCH_arm)
  LExpr  mR15expr; 
  LExpr  mR14expr;
  LExpr  mR13expr;
  LExpr  mR12expr;
  LExpr  mR11expr;
  LExpr  mR7expr;
#else
#   error "Unknown arch"
#endif
};



static inline bool registerIsTracked(DW_REG_NUMBER reg) {
  switch (reg) {
#   if defined(LUL_ARCH_x64) || defined(LUL_ARCH_x86)
    case DW_REG_INTEL_XBP: case DW_REG_INTEL_XSP: case DW_REG_INTEL_XIP:
      return true;
#   elif defined(LUL_ARCH_arm)
    case DW_REG_ARM_R7:  case DW_REG_ARM_R11: case DW_REG_ARM_R12:
    case DW_REG_ARM_R13: case DW_REG_ARM_R14: case DW_REG_ARM_R15:
      return true;
#   else
#     error "Unknown arch"
#   endif
    default:
      return false;
  }
}











class SecMap {
public:
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  explicit SecMap(void(*aLog)(const char*));
  ~SecMap();

  
  
  
  RuleSet* FindRuleSet(uintptr_t ia);

  
  
  void AddRuleSet(const RuleSet* rs);

  
  
  uint32_t AddPfxInstr(PfxInstr pfxi);

  
  const vector<PfxInstr>* GetPfxInstrs() { return &mPfxInstrs; }

  
  
  
  void PrepareRuleSets(uintptr_t start, size_t len);

  bool IsEmpty();

  size_t Size() { return mRuleSets.size(); }

  
  
  uintptr_t mSummaryMinAddr;
  uintptr_t mSummaryMaxAddr;

private:
  
  
  bool mUsable;

  
  vector<RuleSet> mRuleSets;

  
  
  
  
  
  
  
  
  
  
  
  vector<PfxInstr> mPfxInstrs;

  
  void (*mLog)(const char*);
};

} 

#endif 

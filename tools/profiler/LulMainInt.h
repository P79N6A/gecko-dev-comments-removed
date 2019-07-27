





#ifndef LulMainInt_h
#define LulMainInt_h

#include "LulPlatformMacros.h"

#include <vector>

#include "mozilla/Assertions.h"






namespace lul {








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













struct LExpr {
  
  LExpr()
    : mHow(UNKNOWN)
    , mReg(0)
    , mOffset(0)
  {}

  
  LExpr(uint8_t how, int16_t reg, int32_t offset)
    : mHow(how)
    , mReg(reg)
    , mOffset(offset)
  {}

  
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

  
  
  

  enum { UNKNOWN=0, 
         NODEREF,   
         DEREF };   

  uint8_t mHow;    
  int16_t mReg;    
  int32_t mOffset; 
};

static_assert(sizeof(LExpr) <= 8, "LExpr size changed unexpectedly");














































class RuleSet {
public:
  RuleSet();
  void   Print(void(*aLog)(const char*));

  
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











class SecMap {
public:
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  explicit SecMap(void(*aLog)(const char*));
  ~SecMap();

  
  
  
  RuleSet* FindRuleSet(uintptr_t ia);

  
  
  void AddRuleSet(RuleSet* rs);

  
  
  
  void PrepareRuleSets(uintptr_t start, size_t len);

  bool IsEmpty();

  size_t Size() { return mRuleSets.size(); }

  
  
  uintptr_t mSummaryMinAddr;
  uintptr_t mSummaryMaxAddr;

private:
  
  
  bool mUsable;

  
  std::vector<RuleSet> mRuleSets;

  
  void (*mLog)(const char*);
};

} 

#endif 

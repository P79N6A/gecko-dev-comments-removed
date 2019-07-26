





#include "LulDwarfSummariser.h"

#include "mozilla/Assertions.h"


#define DEBUG_SUMMARISER 0

namespace lul {

Summariser::Summariser(SecMap* aSecMap, uintptr_t aTextBias,
                       void(*aLog)(const char*))
  : mSecMap(aSecMap)
  , mTextBias(aTextBias)
  , mLog(aLog)
{
  mCurrAddr = 0;
  mMax1Addr = 0; 

  
  new (&mCurrRules) RuleSet();
}

void
Summariser::Entry(uintptr_t aAddress, uintptr_t aLength)
{
  aAddress += mTextBias;
  if (DEBUG_SUMMARISER) {
    char buf[100];
    snprintf(buf, sizeof(buf), "LUL Entry(%llx, %llu)\n",
             (unsigned long long int)aAddress,
             (unsigned long long int)aLength);
    buf[sizeof(buf)-1] = 0;
    mLog(buf);
  }
  
  
  
  mCurrAddr = aAddress;
  mMax1Addr = aAddress + aLength;
  new (&mCurrRules) RuleSet();
}

void
Summariser::Rule(uintptr_t aAddress,
                 int aNewReg, int aOldReg, intptr_t aOffset, bool aDeref)
{
  aAddress += mTextBias;
  if (DEBUG_SUMMARISER) {
    char buf[100];
    snprintf(buf, sizeof(buf),
             "LUL  0x%llx  old-r%d = %sr%d + %ld%s\n",
             (unsigned long long int)aAddress, aNewReg,
             aDeref ? "*(" : "", aOldReg, (long)aOffset, aDeref ? ")" : "");
    buf[sizeof(buf)-1] = 0;
    mLog(buf);
  }
  if (mCurrAddr < aAddress) {
    
    mCurrRules.mAddr = mCurrAddr;
    mCurrRules.mLen  = aAddress - mCurrAddr;
    mSecMap->AddRuleSet(&mCurrRules);
    if (DEBUG_SUMMARISER) {
      mLog("LUL  "); mCurrRules.Print(mLog);
      mLog("\n");
    }
    mCurrAddr = aAddress;
  }

  

#if defined(LUL_ARCH_arm)

  

  
  
  
  switch (aNewReg) {

    case DW_REG_CFA:
      
      
      
      
      if (aDeref) {
        goto cant_summarise;
      }
      switch (aOldReg) {
        case DW_REG_ARM_R7:  case DW_REG_ARM_R11:
        case DW_REG_ARM_R12: case DW_REG_ARM_R13:
          break;
        default:
          goto cant_summarise;
      }
      mCurrRules.mCfaExpr = LExpr(LExpr::NODEREF, aOldReg, aOffset);
      break;

    case DW_REG_ARM_R7:  case DW_REG_ARM_R11: case DW_REG_ARM_R12:
    case DW_REG_ARM_R13: case DW_REG_ARM_R14: case DW_REG_ARM_R15: {
      
      switch (aOldReg) {
        case DW_REG_CFA:
        case DW_REG_ARM_R7:  case DW_REG_ARM_R11: case DW_REG_ARM_R12:
        case DW_REG_ARM_R13: case DW_REG_ARM_R14: case DW_REG_ARM_R15:
          break;
        default:
          goto cant_summarise;
      }
      
      
      
      LExpr expr = LExpr(aDeref ? LExpr::DEREF : LExpr::NODEREF,
                         aOldReg, aOffset);
      switch (aNewReg) {
        case DW_REG_ARM_R7:  mCurrRules.mR7expr  = expr; break;
        case DW_REG_ARM_R11: mCurrRules.mR11expr = expr; break;
        case DW_REG_ARM_R12: mCurrRules.mR12expr = expr; break;
        case DW_REG_ARM_R13: mCurrRules.mR13expr = expr; break;
        case DW_REG_ARM_R14: mCurrRules.mR14expr = expr; break;
        case DW_REG_ARM_R15: mCurrRules.mR15expr = expr; break;
        default: MOZ_ASSERT(0);
      }
      break;
    }

    default:
      goto cant_summarise;
  }

  
  
  
  if (mCurrRules.mR7expr.mHow == LExpr::UNKNOWN) {
    mCurrRules.mR7expr = LExpr(LExpr::NODEREF, DW_REG_ARM_R7, 0);
  }
  if (mCurrRules.mR11expr.mHow == LExpr::UNKNOWN) {
    mCurrRules.mR11expr = LExpr(LExpr::NODEREF, DW_REG_ARM_R11, 0);
  }
  if (mCurrRules.mR12expr.mHow == LExpr::UNKNOWN) {
    mCurrRules.mR12expr = LExpr(LExpr::NODEREF, DW_REG_ARM_R12, 0);
  }

  
  
  mCurrRules.mR13expr = LExpr(LExpr::NODEREF, DW_REG_CFA, 0);

  
  
  if (mCurrRules.mR15expr.mHow == LExpr::UNKNOWN) {
    mCurrRules.mR15expr = LExpr(LExpr::NODEREF, DW_REG_ARM_R14, 0);
  }

#elif defined(LUL_ARCH_x64) || defined(LUL_ARCH_x86)

  

  
  
  
  
  
  
  if (aNewReg == DW_REG_CFA) {
    
    
    if (!aDeref && aOffset == (intptr_t)(int32_t)aOffset &&
        (aOldReg == DW_REG_INTEL_XSP || aOldReg == DW_REG_INTEL_XBP)) {
      mCurrRules.mCfaExpr = LExpr(LExpr::NODEREF, aOldReg, aOffset);
    } else {
      goto cant_summarise;
    }
  }
  else
  if ((aNewReg == DW_REG_INTEL_XSP ||
       aNewReg == DW_REG_INTEL_XBP || aNewReg == DW_REG_INTEL_XIP) &&
      (aOldReg == DW_REG_CFA ||
       aOldReg == DW_REG_INTEL_XSP ||
       aOldReg == DW_REG_INTEL_XBP || aOldReg == DW_REG_INTEL_XIP) &&
      aOffset == (intptr_t)(int32_t)aOffset) {
    
    
    LExpr expr = LExpr(aDeref ? LExpr::DEREF : LExpr::NODEREF,
                       aOldReg, aOffset);
    switch (aNewReg) {
      case DW_REG_INTEL_XBP: mCurrRules.mXbpExpr = expr; break;
      case DW_REG_INTEL_XSP: mCurrRules.mXspExpr = expr; break;
      case DW_REG_INTEL_XIP: mCurrRules.mXipExpr = expr; break;
      default: MOZ_CRASH("impossible value for aNewReg");
    }
  }
  else {
    goto cant_summarise;
  }

  
  
  
  if (mCurrRules.mXspExpr.mHow == LExpr::UNKNOWN) {
    mCurrRules.mXspExpr = LExpr(LExpr::NODEREF, DW_REG_CFA, 0);
  }

  
  if (mCurrRules.mXbpExpr.mHow == LExpr::UNKNOWN) {
    mCurrRules.mXbpExpr = LExpr(LExpr::NODEREF, DW_REG_INTEL_XBP, 0);
  }

#else

# error "Unsupported arch"
#endif

  return;
 cant_summarise:
  if (0) {
    mLog("LUL  can't summarise\n");
  }
}

void
Summariser::End()
{
  if (DEBUG_SUMMARISER) {
    mLog("LUL End\n");
  }
  if (mCurrAddr < mMax1Addr) {
    mCurrRules.mAddr = mCurrAddr;
    mCurrRules.mLen  = mMax1Addr - mCurrAddr;
    mSecMap->AddRuleSet(&mCurrRules);
    if (DEBUG_SUMMARISER) {
      mLog("LUL  "); mCurrRules.Print(mLog);
      mLog("\n");
    }
  }
}

} 

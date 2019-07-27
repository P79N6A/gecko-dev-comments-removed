





#include "LulDwarfSummariser.h"

#include "mozilla/Assertions.h"


#define DEBUG_SUMMARISER 0

namespace lul {


static inline bool fitsIn32Bits(int64 s64) {
  return s64 == ((s64 & 0xffffffff) ^ 0x80000000) - 0x80000000;
}







static const char*
checkPfxExpr(const vector<PfxInstr>* pfxInstrs, int64_t start)
{
  size_t nInstrs = pfxInstrs->size();
  if (start < 0 || start >= (ssize_t)nInstrs) {
    return "bogus start point";
  }
  size_t i;
  for (i = start; i < nInstrs; i++) {
    PfxInstr pxi = (*pfxInstrs)[i];
    if (pxi.mOpcode == PX_End)
      break;
    if (pxi.mOpcode == PX_DwReg &&
        !registerIsTracked((DW_REG_NUMBER)pxi.mOperand)) {
      return "uses untracked reg";
    }
  }
  return nullptr; 
}


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
    snprintf_literal(buf, "LUL Entry(%llx, %llu)\n",
                     (unsigned long long int)aAddress,
                     (unsigned long long int)aLength);
    mLog(buf);
  }
  
  
  
  mCurrAddr = aAddress;
  mMax1Addr = aAddress + aLength;
  new (&mCurrRules) RuleSet();
}

void
Summariser::Rule(uintptr_t aAddress, int aNewReg,
                 LExprHow how, int16_t oldReg, int64_t offset)
{
  aAddress += mTextBias;
  if (DEBUG_SUMMARISER) {
    char buf[100];
    if (how == NODEREF || how == DEREF) {
      bool deref = how == DEREF;
      snprintf_literal(buf,
                       "LUL  0x%llx  old-r%d = %sr%d + %lld%s\n",
                       (unsigned long long int)aAddress, aNewReg,
                       deref ? "*(" : "", (int)oldReg, (long long int)offset,
                       deref ? ")" : "");
    } else if (how == PFXEXPR) {
      snprintf_literal(buf,
                       "LUL  0x%llx  old-r%d = pfx-expr-at %lld\n",
                       (unsigned long long int)aAddress, aNewReg,
                       (long long int)offset);
    } else {
      snprintf_literal(buf,
                       "LUL  0x%llx  old-r%d = (invalid LExpr!)\n",
                       (unsigned long long int)aAddress, aNewReg);
    }
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

  
  
  
  
  const char* reason1 = nullptr;
  const char* reason2 = nullptr;
  
  
  
  
  if (!fitsIn32Bits(offset)) {
    reason1 = "offset not in signed 32-bit range";
    goto cant_summarise;
  }

  

#if defined(LUL_ARCH_arm)

  

  
  
  
  switch (aNewReg) {

    case DW_REG_CFA:
      
      
      
      
      if (how != NODEREF) {
        reason1 = "rule for DW_REG_CFA: invalid |how|";
        goto cant_summarise;
      }
      switch (oldReg) {
        case DW_REG_ARM_R7:  case DW_REG_ARM_R11:
        case DW_REG_ARM_R12: case DW_REG_ARM_R13:
          break;
        default:
          reason1 = "rule for DW_REG_CFA: invalid |oldReg|";
          goto cant_summarise;
      }
      mCurrRules.mCfaExpr = LExpr(how, oldReg, offset);
      break;

    case DW_REG_ARM_R7:  case DW_REG_ARM_R11: case DW_REG_ARM_R12:
    case DW_REG_ARM_R13: case DW_REG_ARM_R14: case DW_REG_ARM_R15: {
      
      
      switch (how) {
        case NODEREF: case DEREF:
          
          if (!registerIsTracked((DW_REG_NUMBER)oldReg) &&
              oldReg != DW_REG_CFA) {
            reason1 = "rule for R7/11/12/13/14/15: uses untracked reg";
            goto cant_summarise;
          }
          break;
        case PFXEXPR: {
          
          const vector<PfxInstr>* pfxInstrs = mSecMap->GetPfxInstrs();
          reason2 = checkPfxExpr(pfxInstrs, offset);
          if (reason2) {
            reason1 = "rule for R7/11/12/13/14/15: ";
            goto cant_summarise;
          }
          break;
        }
        default:
          goto cant_summarise;
      }
      LExpr expr = LExpr(how, oldReg, offset);
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

  
  
  
  if (mCurrRules.mR7expr.mHow == UNKNOWN) {
    mCurrRules.mR7expr = LExpr(NODEREF, DW_REG_ARM_R7, 0);
  }
  if (mCurrRules.mR11expr.mHow == UNKNOWN) {
    mCurrRules.mR11expr = LExpr(NODEREF, DW_REG_ARM_R11, 0);
  }
  if (mCurrRules.mR12expr.mHow == UNKNOWN) {
    mCurrRules.mR12expr = LExpr(NODEREF, DW_REG_ARM_R12, 0);
  }

  
  
  mCurrRules.mR13expr = LExpr(NODEREF, DW_REG_CFA, 0);

  
  
  if (mCurrRules.mR15expr.mHow == UNKNOWN) {
    mCurrRules.mR15expr = LExpr(NODEREF, DW_REG_ARM_R14, 0);
  }

#elif defined(LUL_ARCH_x64) || defined(LUL_ARCH_x86)

  

  
  
  
  switch (aNewReg) {

    case DW_REG_CFA:
      
      
      if (how != NODEREF) {
        reason1 = "rule for DW_REG_CFA: invalid |how|";
        goto cant_summarise;
      }
      if (oldReg != DW_REG_INTEL_XSP && oldReg != DW_REG_INTEL_XBP) {
        reason1 = "rule for DW_REG_CFA: invalid |oldReg|";
        goto cant_summarise;
      }
      mCurrRules.mCfaExpr = LExpr(how, oldReg, offset);
      break;

    case DW_REG_INTEL_XSP: case DW_REG_INTEL_XBP: case DW_REG_INTEL_XIP: {
      
      switch (how) {
        case NODEREF: case DEREF:
          
          if (!registerIsTracked((DW_REG_NUMBER)oldReg) &&
              oldReg != DW_REG_CFA) {
            reason1 = "rule for XSP/XBP/XIP: uses untracked reg";
            goto cant_summarise;
          }
          break;
        case PFXEXPR: {
          
          const vector<PfxInstr>* pfxInstrs = mSecMap->GetPfxInstrs();
          reason2 = checkPfxExpr(pfxInstrs, offset);
          if (reason2) {
            reason1 = "rule for XSP/XBP/XIP: ";
            goto cant_summarise;
          }
          break;
        }
        default:
          goto cant_summarise;
      }
      LExpr expr = LExpr(how, oldReg, offset);
      switch (aNewReg) {
        case DW_REG_INTEL_XBP: mCurrRules.mXbpExpr = expr; break;
        case DW_REG_INTEL_XSP: mCurrRules.mXspExpr = expr; break;
        case DW_REG_INTEL_XIP: mCurrRules.mXipExpr = expr; break;
        default: MOZ_CRASH("impossible value for aNewReg");
      }
      break;
    }

    default:
      
      
      goto cant_summarise;

  }

  
  
  
  if (mCurrRules.mXspExpr.mHow == UNKNOWN) {
    mCurrRules.mXspExpr = LExpr(NODEREF, DW_REG_CFA, 0);
  }

  
  if (mCurrRules.mXbpExpr.mHow == UNKNOWN) {
    mCurrRules.mXbpExpr = LExpr(NODEREF, DW_REG_INTEL_XBP, 0);
  }

#else

# error "Unsupported arch"
#endif

  return;

 cant_summarise:
  if (reason1 || reason2) {
    char buf[200];
    snprintf_literal(buf, "LUL  can't summarise: "
                     "SVMA=0x%llx: %s%s, expr=LExpr(%s,%u,%lld)\n",
                     (unsigned long long int)(aAddress - mTextBias),
                     reason1 ? reason1 : "", reason2 ? reason2 : "",
                     NameOf_LExprHow(how),
                     (unsigned int)oldReg, (long long int)offset);
    mLog(buf);
  }
}

uint32_t
Summariser::AddPfxInstr(PfxInstr pfxi)
{
  return mSecMap->AddPfxInstr(pfxi);
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

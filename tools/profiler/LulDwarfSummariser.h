





#ifndef LulDwarfSummariser_h
#define LulDwarfSummariser_h

#include "LulMainInt.h"

namespace lul {

class Summariser
{
public:
  Summariser(SecMap* aSecMap, uintptr_t aTextBias, void(*aLog)(const char*));

  virtual void Entry(uintptr_t aAddress, uintptr_t aLength);
  virtual void End();

  
  
  
  
  
  
  virtual void Rule(uintptr_t aAddress, int aNewReg,
                    LExprHow how, int16_t oldReg, int64_t offset);

  virtual uint32_t AddPfxInstr(PfxInstr pfxi);

  
  virtual void Log(const char* str) { mLog(str); }
  
private:
  
  
  SecMap* mSecMap;

  
  RuleSet mCurrRules;

  
  
  uintptr_t mCurrAddr;

  
  
  
  
  
  uintptr_t mMax1Addr;

  
  
  uintptr_t mTextBias;

  
  void (*mLog)(const char* aFmt);
};

} 

#endif 

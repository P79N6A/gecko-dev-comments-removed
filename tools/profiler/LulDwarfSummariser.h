





#ifndef LulDwarfSummariser_h
#define LulDwarfSummariser_h

#include "LulMainInt.h"

namespace lul {

class Summariser
{
public:
  Summariser(SecMap* aSecMap, uintptr_t aTextBias, void(*aLog)(const char*));

  void Entry(uintptr_t aAddress, uintptr_t aLength);
  void End();
  void Rule(uintptr_t aAddress,
            int aNewReg, int aOldReg, intptr_t aOffset, bool aDeref);

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

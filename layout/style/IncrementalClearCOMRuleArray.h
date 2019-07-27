





#ifndef mozilla_IncrementalClearCOMRuleArray_h
#define mozilla_IncrementalClearCOMRuleArray_h

#include "nsCOMArray.h"

namespace mozilla {
namespace css {
class Rule;
} 

class IncrementalClearCOMRuleArray : public nsCOMArray<css::Rule>
{
public:
  IncrementalClearCOMRuleArray() {}
  ~IncrementalClearCOMRuleArray() { Clear(); }

  void Clear();
};

} 

#endif 

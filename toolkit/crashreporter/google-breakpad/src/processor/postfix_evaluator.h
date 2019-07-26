



































































#ifndef PROCESSOR_POSTFIX_EVALUATOR_H__
#define PROCESSOR_POSTFIX_EVALUATOR_H__


#include <map>
#include <string>
#include <vector>

#include "common/using_std_string.h"
#include "common/unique_string.h"
#include "common/module.h"

namespace google_breakpad {

using std::map;
using std::vector;

class MemoryRegion;


template<typename ValueType>
class StackElem {
 public:
  StackElem(ValueType val) { isValue = true; u.val = val; }
  StackElem(const UniqueString* ustr) { isValue = false; u.ustr = ustr; }
  bool isValue;
  union { ValueType val; const UniqueString* ustr; } u;
};

template<typename ValueType>
class PostfixEvaluator {
 public:
  typedef UniqueStringMap<ValueType> DictionaryType;
  typedef UniqueStringMap<bool>      DictionaryValidityType;

  
  
  
  
  
  
  PostfixEvaluator(DictionaryType *dictionary, const MemoryRegion *memory)
      : dictionary_(dictionary), memory_(memory), stack_() {}

  
  
  
  
  
  
  
  bool Evaluate(const Module::Expr &expr, DictionaryValidityType *assigned);

  
  
  
  
  bool EvaluateForValue(const Module::Expr& expression, ValueType* result);

  DictionaryType* dictionary() const { return dictionary_; }

  
  void set_dictionary(DictionaryType *dictionary) {dictionary_ = dictionary; }

 private:
  
  enum PopResult {
    POP_RESULT_FAIL = 0,
    POP_RESULT_VALUE,
    POP_RESULT_IDENTIFIER
  };

  
  
  
  
  
  
  PopResult PopValueOrIdentifier(ValueType *value,
                                 const UniqueString** identifier);

  
  
  
  
  bool PopValue(ValueType *value);

  
  void PushIdentifier(const UniqueString* ustr);

  
  
  
  bool PopValues(ValueType *value1, ValueType *value2);

  
  void PushValue(const ValueType &value);

  
  
  
  bool EvaluateInternal(const string &expression,
                        DictionaryValidityType *assigned);

  bool EvaluateToken(const string &token,
                     const string &expression,
                     DictionaryValidityType *assigned);

  
  
  
  DictionaryType *dictionary_;

  
  
  const MemoryRegion *memory_;

  
  
  
  vector<StackElem<ValueType> > stack_;
};

}  


#endif

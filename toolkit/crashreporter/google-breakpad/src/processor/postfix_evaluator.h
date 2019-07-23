
















































#ifndef PROCESSOR_POSTFIX_EVALUATOR_H__
#define PROCESSOR_POSTFIX_EVALUATOR_H__


#include <map>
#include <string>
#include <vector>

namespace google_breakpad {

using std::map;
using std::string;
using std::vector;

class MemoryRegion;

template<typename ValueType>
class PostfixEvaluator {
 public:
  typedef map<string, ValueType> DictionaryType;
  typedef map<string, bool> DictionaryValidityType;

  
  
  
  
  
  
  PostfixEvaluator(DictionaryType *dictionary, MemoryRegion *memory)
      : dictionary_(dictionary), memory_(memory), stack_() {}

  
  
  
  
  
  
  
  bool Evaluate(const string &expression, DictionaryValidityType *assigned);

  DictionaryType* dictionary() const { return dictionary_; }

  
  void set_dictionary(DictionaryType *dictionary) {dictionary_ = dictionary; }

 private:
  
  enum PopResult {
    POP_RESULT_FAIL = 0,
    POP_RESULT_VALUE,
    POP_RESULT_IDENTIFIER
  };

  
  
  
  
  
  
  PopResult PopValueOrIdentifier(ValueType *value, string *identifier);

  
  
  
  
  bool PopValue(ValueType *value);

  
  
  
  bool PopValues(ValueType *value1, ValueType *value2);

  
  void PushValue(const ValueType &value);

  
  
  
  DictionaryType *dictionary_;

  
  
  MemoryRegion *memory_;

  
  
  
  vector<string> stack_;
};

}  


#endif

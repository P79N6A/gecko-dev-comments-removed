
































#include <cstdio>
#include <map>
#include <string>

#include "processor/postfix_evaluator-inl.h"

#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/processor/memory_region.h"


namespace {


using std::map;
using std::string;
using google_breakpad::MemoryRegion;
using google_breakpad::PostfixEvaluator;





class FakeMemoryRegion : public MemoryRegion {
 public:
  virtual u_int64_t GetBase() { return 0; }
  virtual u_int32_t GetSize() { return 0; }
  virtual bool GetMemoryAtAddress(u_int64_t address, u_int8_t  *value) {
    *value = address + 1;
    return true;
  }
  virtual bool GetMemoryAtAddress(u_int64_t address, u_int16_t *value) {
    *value = address + 1;
    return true;
  }
  virtual bool GetMemoryAtAddress(u_int64_t address, u_int32_t *value) {
    *value = address + 1;
    return true;
  }
  virtual bool GetMemoryAtAddress(u_int64_t address, u_int64_t *value) {
    *value = address + 1;
    return true;
  }
};


struct EvaluateTest {
  
  const string expression;

  
  
  bool evaluable;
};


struct EvaluateTestSet {
  
  PostfixEvaluator<unsigned int>::DictionaryType *dictionary;

  
  const EvaluateTest *evaluate_tests;

  
  unsigned int evaluate_test_count;

  
  
  map<string, unsigned int> *validate_data;
};


static bool RunTests() {
  
  PostfixEvaluator<unsigned int>::DictionaryType dictionary_0;
  const EvaluateTest evaluate_tests_0[] = {
    { "$rAdd 2 2 + =",     true },   
    { "$rAdd $rAdd 2 + =", true },   
    { "$rAdd 2 $rAdd + =", true },   
    { "99",                false },  
    { "$rAdd2 2 2 + =",    true },   
    { "$rAdd2\t2\n2 + =",  true },   
    { "$rAdd2 2 2 + = ",   true },   
    { " $rAdd2 2 2 + =",   true },   
    { "$rAdd2  2 2 +   =", true },   
    { "$T0 2 = +",         false },  
    { "2 + =",             false },  
    { "2 +",               false },  
    { "+",                 false },  
    { "^",                 false },  
    { "=",                 false },  
    { "2 =",               false },  
    { "2 2 + =",           false },  
    { "2 2 =",             false },  
    { "k 2 =",             false },  
    { "2",                 false },  
    { "2 2 +",             false },  
    { "$rAdd",             false },  
    { "0 $T1 0 0 + =",     false },  
    { "$T2 $T2 2 + =",     false },  
    { "$rMul 9 6 * =",     true },   
    { "$rSub 9 6 - =",     true },   
    { "$rDivQ 9 6 / =",    true },   
    { "$rDivM 9 6 % =",    true },   
    { "$rDeref 9 ^ =",     true }    
  };
  map<string, unsigned int> validate_data_0;
  validate_data_0["$rAdd"]   = 8;
  validate_data_0["$rAdd2"]  = 4;
  validate_data_0["$rSub"]   = 3;
  validate_data_0["$rMul"]   = 54;
  validate_data_0["$rDivQ"]  = 1;
  validate_data_0["$rDivM"]  = 3;
  validate_data_0["$rDeref"] = 10;

  
  
  
  
  PostfixEvaluator<unsigned int>::DictionaryType dictionary_1;
  dictionary_1["$ebp"] = 0xbfff0010;
  dictionary_1["$eip"] = 0x10000000;
  dictionary_1["$esp"] = 0xbfff0000;
  dictionary_1[".cbSavedRegs"] = 4;
  dictionary_1[".cbParams"] = 4;
  dictionary_1[".raSearchStart"] = 0xbfff0020;
  const EvaluateTest evaluate_tests_1[] = {
    { "$T0 $ebp = $eip $T0 4 + ^ = $ebp $T0 ^ = $esp $T0 8 + = "
      "$L $T0 .cbSavedRegs - = $P $T0 8 + .cbParams + =", true },
    
    
    
    { "$T0 $ebp = $eip $T0 4 + ^ = $ebp $T0 ^ = $esp $T0 8 + = "
      "$L $T0 .cbSavedRegs - = $P $T0 8 + .cbParams + = $ebx $T0 28 - ^ =",
      true },
    
    
    
    
    { "$T0 $ebp = $T2 $esp = $T1 .raSearchStart = $eip $T1 ^ = $ebp $T0 = "
      "$esp $T1 4 + = $L $T0 .cbSavedRegs - = $P $T1 4 + .cbParams + = "
      "$ebx $T0 28 - ^ =",
      true }
  };
  map<string, unsigned int> validate_data_1;
  validate_data_1["$T0"]  = 0xbfff0012;
  validate_data_1["$T1"]  = 0xbfff0020;
  validate_data_1["$T2"]  = 0xbfff0019;
  validate_data_1["$eip"] = 0xbfff0021;
  validate_data_1["$ebp"] = 0xbfff0012;
  validate_data_1["$esp"] = 0xbfff0024;
  validate_data_1["$L"]   = 0xbfff000e;
  validate_data_1["$P"]   = 0xbfff0028;
  validate_data_1["$ebx"] = 0xbffefff7;
  validate_data_1[".cbSavedRegs"] = 4;
  validate_data_1[".cbParams"] = 4;

  EvaluateTestSet evaluate_test_sets[] = {
    { &dictionary_0, evaluate_tests_0,
          sizeof(evaluate_tests_0) / sizeof(EvaluateTest), &validate_data_0 },
    { &dictionary_1, evaluate_tests_1,
          sizeof(evaluate_tests_1) / sizeof(EvaluateTest), &validate_data_1 },
  };

  unsigned int evaluate_test_set_count = sizeof(evaluate_test_sets) /
                                         sizeof(EvaluateTestSet);

  FakeMemoryRegion fake_memory;
  PostfixEvaluator<unsigned int> postfix_evaluator =
      PostfixEvaluator<unsigned int>(NULL, &fake_memory);

  for (unsigned int evaluate_test_set_index = 0;
       evaluate_test_set_index < evaluate_test_set_count;
       ++evaluate_test_set_index) {
    EvaluateTestSet *evaluate_test_set =
        &evaluate_test_sets[evaluate_test_set_index];
    const EvaluateTest *evaluate_tests = evaluate_test_set->evaluate_tests;
    unsigned int evaluate_test_count = evaluate_test_set->evaluate_test_count;

    
    
    postfix_evaluator.set_dictionary(evaluate_test_set->dictionary);

    
    PostfixEvaluator<unsigned int>::DictionaryValidityType assigned;

    for (unsigned int evaluate_test_index = 0;
         evaluate_test_index < evaluate_test_count;
         ++evaluate_test_index) {
      const EvaluateTest *evaluate_test = &evaluate_tests[evaluate_test_index];

      
      bool result = postfix_evaluator.Evaluate(evaluate_test->expression,
                                               &assigned);
      if (result != evaluate_test->evaluable) {
        fprintf(stderr, "FAIL: evaluate set %d/%d, test %d/%d, "
                        "expression \"%s\", expected %s, observed %s\n",
                evaluate_test_set_index, evaluate_test_set_count,
                evaluate_test_index, evaluate_test_count,
                evaluate_test->expression.c_str(),
                evaluate_test->evaluable ? "evaluable" : "not evaluable",
                result ? "evaluted" : "not evaluated");
        return false;
      }
    }

    
    for (map<string, unsigned int>::const_iterator validate_iterator =
            evaluate_test_set->validate_data->begin();
        validate_iterator != evaluate_test_set->validate_data->end();
        ++validate_iterator) {
      const string identifier = validate_iterator->first;
      unsigned int expected_value = validate_iterator->second;

      map<string, unsigned int>::const_iterator dictionary_iterator =
          evaluate_test_set->dictionary->find(identifier);

      
      if (dictionary_iterator == evaluate_test_set->dictionary->end()) {
        fprintf(stderr, "FAIL: evaluate test set %d/%d, "
                        "validate identifier \"%s\", "
                        "expected %d, observed not found\n",
                evaluate_test_set_index, evaluate_test_set_count,
                identifier.c_str(), expected_value);
        return false;
      }

      
      unsigned int observed_value = dictionary_iterator->second;
      if (expected_value != observed_value) {
        fprintf(stderr, "FAIL: evaluate test set %d/%d, "
                        "validate identifier \"%s\", "
                        "expected %d, observed %d\n",
                evaluate_test_set_index, evaluate_test_set_count,
                identifier.c_str(), expected_value, observed_value);
        return false;
      }

      
      
      bool expected_assigned = identifier[0] == '$';
      bool observed_assigned = false;
      PostfixEvaluator<unsigned int>::DictionaryValidityType::const_iterator
          iterator_assigned = assigned.find(identifier);
      if (iterator_assigned != assigned.end()) {
        observed_assigned = iterator_assigned->second;
      }
      if (expected_assigned != observed_assigned) {
        fprintf(stderr, "FAIL: evaluate test set %d/%d, "
                        "validate assignment of \"%s\", "
                        "expected %d, observed %d\n",
                evaluate_test_set_index, evaluate_test_set_count,
                identifier.c_str(), expected_assigned, observed_assigned);
        return false;
      }
    }
  }

  return true;
}


}  


int main(int argc, char **argv) {
  return RunTests() ? 0 : 1;
}

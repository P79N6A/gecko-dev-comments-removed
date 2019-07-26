
































#include <stdio.h>

#include <map>
#include <string>

#include "processor/postfix_evaluator-inl.h"

#include "common/using_std_string.h"
#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/processor/memory_region.h"
#include "processor/logging.h"


namespace {


using std::map;
using google_breakpad::FromUniqueString;
using google_breakpad::MemoryRegion;
using google_breakpad::PostfixEvaluator;
using google_breakpad::ToUniqueString;
using google_breakpad::UniqueString;
using google_breakpad::ustr__ZDcbParams;
using google_breakpad::ustr__ZDcbSavedRegs;
using google_breakpad::ustr__ZDcfa;
using google_breakpad::ustr__ZDra;
using google_breakpad::ustr__ZDraSearchStart;
using google_breakpad::ustr__ZSebx;
using google_breakpad::ustr__ZSebp;
using google_breakpad::ustr__ZSedi;
using google_breakpad::ustr__ZSeip;
using google_breakpad::ustr__ZSesi;
using google_breakpad::ustr__ZSesp;





class FakeMemoryRegion : public MemoryRegion {
 public:
  virtual uint64_t GetBase() const { return 0; }
  virtual uint32_t GetSize() const { return 0; }
  virtual bool GetMemoryAtAddress(uint64_t address, uint8_t  *value) const {
    *value = address + 1;
    return true;
  }
  virtual bool GetMemoryAtAddress(uint64_t address, uint16_t *value) const {
    *value = address + 1;
    return true;
  }
  virtual bool GetMemoryAtAddress(uint64_t address, uint32_t *value) const {
    *value = address + 1;
    return true;
  }
  virtual bool GetMemoryAtAddress(uint64_t address, uint64_t *value) const {
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

  
  
  map<const UniqueString*, unsigned int> *validate_data;
};


struct EvaluateForValueTest {
  
  const string expression;
  
  
  
  bool evaluable;

  
  unsigned int value;
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
    { "$rDeref 9 ^ =",     true },   
    { "$rAlign 36 8 @ =",  true },   
    { "$rAdd3 2 2 + =$rMul2 9 6 * =", true } 
  };
  map<const UniqueString*, unsigned int> validate_data_0;
  validate_data_0[ToUniqueString("$rAdd")]   = 8;
  validate_data_0[ToUniqueString("$rAdd2")]  = 4;
  validate_data_0[ToUniqueString("$rSub")]   = 3;
  validate_data_0[ToUniqueString("$rMul")]   = 54;
  validate_data_0[ToUniqueString("$rDivQ")]  = 1;
  validate_data_0[ToUniqueString("$rDivM")]  = 3;
  validate_data_0[ToUniqueString("$rDeref")] = 10;
  validate_data_0[ToUniqueString("$rAlign")] = 32;
  validate_data_0[ToUniqueString("$rAdd3")]  = 4;
  validate_data_0[ToUniqueString("$rMul2")]  = 54;

  
  
  
  
  PostfixEvaluator<unsigned int>::DictionaryType dictionary_1;
  dictionary_1.set(ustr__ZSebp(), 0xbfff0010);
  dictionary_1.set(ustr__ZSeip(), 0x10000000);
  dictionary_1.set(ustr__ZSesp(), 0xbfff0000);
  dictionary_1.set(ustr__ZDcbSavedRegs(), 4);
  dictionary_1.set(ustr__ZDcbParams(), 4);
  dictionary_1.set(ustr__ZDraSearchStart(), 0xbfff0020);
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
  map<const UniqueString*, unsigned int> validate_data_1;
  validate_data_1[ToUniqueString("$T0")]  = 0xbfff0012;
  validate_data_1[ToUniqueString("$T1")]  = 0xbfff0020;
  validate_data_1[ToUniqueString("$T2")]  = 0xbfff0019;
  validate_data_1[ustr__ZSeip()] = 0xbfff0021;
  validate_data_1[ustr__ZSebp()] = 0xbfff0012;
  validate_data_1[ustr__ZSesp()] = 0xbfff0024;
  validate_data_1[ToUniqueString("$L")]   = 0xbfff000e;
  validate_data_1[ToUniqueString("$P")]   = 0xbfff0028;
  validate_data_1[ustr__ZSebx()] = 0xbffefff7;
  validate_data_1[ustr__ZDcbSavedRegs()] = 4;
  validate_data_1[ustr__ZDcbParams()] = 4;

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

    
    for (map<const UniqueString*, unsigned int>::const_iterator
            validate_iterator =
            evaluate_test_set->validate_data->begin();
        validate_iterator != evaluate_test_set->validate_data->end();
        ++validate_iterator) {
      const UniqueString* identifier = validate_iterator->first;
      unsigned int expected_value = validate_iterator->second;

      
      if (!evaluate_test_set->dictionary->have(identifier)) {
        fprintf(stderr, "FAIL: evaluate test set %d/%d, "
                        "validate identifier \"%s\", "
                        "expected %d, observed not found\n",
                evaluate_test_set_index, evaluate_test_set_count,
                FromUniqueString(identifier), expected_value);
        return false;
      }

      
      unsigned int observed_value =
          evaluate_test_set->dictionary->get(identifier);
      if (expected_value != observed_value) {
        fprintf(stderr, "FAIL: evaluate test set %d/%d, "
                        "validate identifier \"%s\", "
                        "expected %d, observed %d\n",
                evaluate_test_set_index, evaluate_test_set_count,
                FromUniqueString(identifier), expected_value, observed_value);
        return false;
      }

      
      
      bool expected_assigned = FromUniqueString(identifier)[0] == '$';
      bool observed_assigned = false;
      if (assigned.have(identifier)) {
        observed_assigned = assigned.get(identifier);
      }
      if (expected_assigned != observed_assigned) {
        fprintf(stderr, "FAIL: evaluate test set %d/%d, "
                        "validate assignment of \"%s\", "
                        "expected %d, observed %d\n",
                evaluate_test_set_index, evaluate_test_set_count,
                FromUniqueString(identifier), expected_assigned,
                observed_assigned);
        return false;
      }
    }
  }

  
  PostfixEvaluator<unsigned int>::DictionaryType dictionary_2;
  dictionary_2.set(ustr__ZSebp(), 0xbfff0010);
  dictionary_2.set(ustr__ZSeip(), 0x10000000);
  dictionary_2.set(ustr__ZSesp(), 0xbfff0000);
  dictionary_2.set(ustr__ZDcbSavedRegs(), 4);
  dictionary_2.set(ustr__ZDcbParams(), 4);
  dictionary_2.set(ustr__ZDraSearchStart(), 0xbfff0020);
  const EvaluateForValueTest evaluate_for_value_tests_2[] = {
    { "28907223",               true,  28907223 },      
    { "89854293 40010015 +",    true,  89854293 + 40010015 }, 
    { "-870245 8769343 +",      true,  7899098 },       
    { "$ebp $esp - $eip +",     true,  0x10000010 },    
    { "18929794 34015074",      false, 0 },             
    { "$ebp $ebp 4 - =",        false, 0 },             
    { "$new $eip = $new",       true,  0x10000000 },    
    { "$new 4 +",               true,  0x10000004 },    
    { ".cfa 42 = 10",           false, 0 }              
  };
  const int evaluate_for_value_tests_2_size
      = (sizeof (evaluate_for_value_tests_2)
         / sizeof (evaluate_for_value_tests_2[0]));
  map<const UniqueString*, unsigned int> validate_data_2;
  validate_data_2[ustr__ZSeip()] = 0x10000000;
  validate_data_2[ustr__ZSebp()] = 0xbfff000c;
  validate_data_2[ustr__ZSesp()] = 0xbfff0000;
  validate_data_2[ToUniqueString("$new")] = 0x10000000;
  validate_data_2[ustr__ZDcbSavedRegs()] = 4;
  validate_data_2[ustr__ZDcbParams()] = 4;
  validate_data_2[ustr__ZDraSearchStart()] = 0xbfff0020;

  postfix_evaluator.set_dictionary(&dictionary_2);
  for (int i = 0; i < evaluate_for_value_tests_2_size; i++) {
    const EvaluateForValueTest *test = &evaluate_for_value_tests_2[i];
    unsigned int result;
    if (postfix_evaluator.EvaluateForValue(test->expression, &result)
        != test->evaluable) {
      fprintf(stderr, "FAIL: evaluate for value test %d, "
              "expected evaluation to %s, but it %s\n",
              i, test->evaluable ? "succeed" : "fail",
              test->evaluable ? "failed" : "succeeded");
      return false;
    }
    if (test->evaluable && result != test->value) {
      fprintf(stderr, "FAIL: evaluate for value test %d, "
              "expected value to be 0x%x, but it was 0x%x\n",
              i, test->value, result);
      return false;
    }
  }

  map<const UniqueString*, unsigned int> dictionary_2_map;
  dictionary_2.copy_to_map(&dictionary_2_map);
  for (map<const UniqueString*, unsigned int>::iterator v =
         validate_data_2.begin();
       v != validate_data_2.end(); v++) {
    map<const UniqueString*, unsigned int>::iterator a =
        dictionary_2_map.find(v->first);
    if (a == dictionary_2_map.end()) {
      fprintf(stderr, "FAIL: evaluate for value dictionary check: "
              "expected dict[\"%s\"] to be 0x%x, but it was unset\n",
              FromUniqueString(v->first), v->second);
      return false;
    } else if (a->second != v->second) {
      fprintf(stderr, "FAIL: evaluate for value dictionary check: "
              "expected dict[\"%s\"] to be 0x%x, but it was 0x%x\n",
              FromUniqueString(v->first), v->second, a->second);
      return false;
    }
    dictionary_2_map.erase(a);
  }

  map<const UniqueString*, unsigned int>::iterator remaining =
      dictionary_2_map.begin();
  if (remaining != dictionary_2_map.end()) {
    fprintf(stderr, "FAIL: evaluation of test expressions put unexpected "
            "values in dictionary:\n");
    for (; remaining != dictionary_2_map.end(); remaining++)
      fprintf(stderr, "    dict[\"%s\"] == 0x%x\n",
              FromUniqueString(remaining->first), remaining->second);
    return false;
  }

  return true;
}


}  


int main(int argc, char **argv) {
  BPLOG_INIT(&argc, &argv);

  return RunTests() ? 0 : 1;
}

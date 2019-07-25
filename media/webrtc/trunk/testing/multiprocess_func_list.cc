



#include "multiprocess_func_list.h"

#include <map>



namespace multi_process_function_list {

namespace {

typedef std::map<std::string, ChildFunctionPtr> MultiProcessTestMap;


MultiProcessTestMap &GetMultiprocessFuncMap() {
  static MultiProcessTestMap test_name_to_func_ptr_map;
  return test_name_to_func_ptr_map;
}

}  

AppendMultiProcessTest::AppendMultiProcessTest(std::string test_name,
                                               ChildFunctionPtr func_ptr) {
  GetMultiprocessFuncMap()[test_name] = func_ptr;
}

int InvokeChildProcessTest(std::string test_name) {
  MultiProcessTestMap &func_lookup_table = GetMultiprocessFuncMap();
  MultiProcessTestMap::iterator it = func_lookup_table.find(test_name);
  if (it != func_lookup_table.end()) {
    ChildFunctionPtr func = it->second;
    if (func) {
      return (*func)();
    }
  }

  return -1;
}

}  

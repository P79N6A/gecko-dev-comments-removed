



#ifndef TESTING_MULTIPROCESS_FUNC_LIST_H_
#define TESTING_MULTIPROCESS_FUNC_LIST_H_

#include <string>




















namespace multi_process_function_list {


typedef int (*ChildFunctionPtr)();



class AppendMultiProcessTest {
 public:
  AppendMultiProcessTest(std::string test_name, ChildFunctionPtr func_ptr);
};



int InvokeChildProcessTest(std::string test_name);



#define MULTIPROCESS_TEST_MAIN(test_main) \
  int test_main(); \
  namespace { \
    multi_process_function_list::AppendMultiProcessTest \
    AddMultiProcessTest##_##test_main(#test_main, (test_main)); \
  } \
  int test_main()

}  

#endif  

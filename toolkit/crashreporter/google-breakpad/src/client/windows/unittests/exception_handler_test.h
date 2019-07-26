




























#ifndef CLIENT_WINDOWS_UNITTESTS_EXCEPTION_HANDLER_TEST_H_
#define CLIENT_WINDOWS_UNITTESTS_EXCEPTION_HANDLER_TEST_H_

namespace testing {
















class DisableExceptionHandlerInScope {
 public:
  DisableExceptionHandlerInScope();
  ~DisableExceptionHandlerInScope();

 private:
  bool catch_exceptions_;
};

}  

#endif  

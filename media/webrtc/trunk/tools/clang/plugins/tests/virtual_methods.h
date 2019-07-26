



#ifndef VIRTUAL_METHODS_H_
#define VIRTUAL_METHODS_H_


class VirtualMethodsInHeaders {
 public:
  
  virtual void MethodIsAbstract() = 0;
  virtual void MethodHasNoArguments();
  virtual void MethodHasEmptyDefaultImpl() {}

  
  virtual bool ComplainAboutThis() { return true; }
};


class WarnOnMissingVirtual : public VirtualMethodsInHeaders {
 public:
  void MethodHasNoArguments() override;
};


namespace testing {
struct TestStruct {};
}  

class VirtualMethodsInHeadersTesting : public VirtualMethodsInHeaders {
 public:
  
  void MethodHasNoArguments();
 private:
  testing::TestStruct tester_;
};

#endif  

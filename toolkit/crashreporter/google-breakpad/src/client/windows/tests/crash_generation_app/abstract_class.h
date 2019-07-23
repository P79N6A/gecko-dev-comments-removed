




























#ifndef CLIENT_WINDOWS_TESTS_CRASH_GENERATION_APP_ABSTRACT_CLASS_H__
#define CLIENT_WINDOWS_TESTS_CRASH_GENERATION_APP_ABSTRACT_CLASS_H__

namespace google_breakpad {



class Derived;

class Base {
 public:
  Base(Derived* derived);
  virtual ~Base();
  virtual void DoSomething() = 0;

 private:
  Derived* derived_;
};

class Derived : public Base {
 public:
  Derived();
  virtual void DoSomething();
};

}  

#endif  

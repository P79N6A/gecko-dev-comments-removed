



#include "overridden_methods.h"


void DerivedClass::SomeMethod() {}
void DerivedClass::SomeOtherMethod() {}
void DerivedClass::WebKitModifiedSomething() {}

class ImplementationInterimClass : public BaseClass {
 public:
  
  virtual void SomeMethod() = 0;
};

class ImplementationDerivedClass : public ImplementationInterimClass,
                                   public webkit_glue::WebKitObserverImpl {
 public:
  
  virtual ~ImplementationDerivedClass() {}
  
  virtual void SomeMethod();
  
  virtual void SomeOtherMethod() override;
  
  virtual void SomeInlineMethod() {}
  
  virtual void WebKitModifiedSomething();
  
  virtual void SomeNonPureBaseMethod() {}
};

int main() {
  DerivedClass something;
  ImplementationDerivedClass something_else;
}

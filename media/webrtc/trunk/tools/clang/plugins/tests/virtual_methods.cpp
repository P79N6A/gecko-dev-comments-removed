



#include "virtual_methods.h"


class VirtualMethodsInImplementation {
 public:
  virtual void MethodIsAbstract() = 0;
  virtual void MethodHasNoArguments();
  virtual void MethodHasEmptyDefaultImpl() {}
  virtual bool ComplainAboutThis() { return true; }
};


class ConcreteVirtualMethodsInHeaders : public VirtualMethodsInHeaders {
 public:
  virtual void MethodIsAbstract() override {}
};

class ConcreteVirtualMethodsInImplementation
    : public VirtualMethodsInImplementation {
 public:
  virtual void MethodIsAbstract() override {}
};


void VirtualMethodsInHeaders::MethodHasNoArguments() {}
void WarnOnMissingVirtual::MethodHasNoArguments() {}
void VirtualMethodsInImplementation::MethodHasNoArguments() {}

int main() {
  ConcreteVirtualMethodsInHeaders one;
  ConcreteVirtualMethodsInImplementation two;
}

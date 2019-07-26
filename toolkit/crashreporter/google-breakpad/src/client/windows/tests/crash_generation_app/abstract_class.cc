




























#include "client/windows/tests/crash_generation_app/abstract_class.h"

namespace google_breakpad {

Base::Base(Derived* derived)
    : derived_(derived) {
}

Base::~Base() {
  derived_->DoSomething();
}

#pragma warning(push)
#pragma warning(disable:4355)

Derived::Derived()
    : Base(this) {  
}
#pragma warning(pop)

void Derived::DoSomething() {
}

}  

#include "nscore.h"

struct S {
  virtual NS_MUST_OVERRIDE void f();
  virtual void g();
};

struct B : S { virtual NS_MUST_OVERRIDE void f(); }; 
struct F : B { }; 


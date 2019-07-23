#include "nscore.h"

struct S {
  virtual NS_MUST_OVERRIDE void f();
  virtual void g();
};

struct A : S { virtual void f(); }; 
struct B : S { virtual NS_MUST_OVERRIDE void f(); }; 
struct E : A { }; 

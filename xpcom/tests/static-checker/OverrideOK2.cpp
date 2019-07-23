#include "nscore.h"

struct Base {
  NS_MUST_OVERRIDE virtual void f();  
  NS_MUST_OVERRIDE void g();          
  NS_MUST_OVERRIDE static void h();   
};

void Base::f() {} 

struct Derived1 : Base { 
  NS_MUST_OVERRIDE virtual void f();
  NS_MUST_OVERRIDE void g();
  NS_MUST_OVERRIDE static void h();
};

struct Derived2 : Derived1 { 
  virtual void f();
  void g();
  static void h();
};

struct Derived3 : Derived2 { 
};

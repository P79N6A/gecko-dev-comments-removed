#define MOZ_MUST_OVERRIDE __attribute__((annotate("moz_must_override")))

#pragma GCC diagnostic ignored "-Woverloaded-virtual"

struct S {
  virtual void f() MOZ_MUST_OVERRIDE; 
  virtual void g() MOZ_MUST_OVERRIDE;
  virtual void h() MOZ_MUST_OVERRIDE; 
};
struct C : S { 
  virtual void g() MOZ_MUST_OVERRIDE; 
  virtual void h(int);
  void q() MOZ_MUST_OVERRIDE; 
};
struct D : C { 
  virtual void f();
};

struct Base {
  virtual void VirtMethod() MOZ_MUST_OVERRIDE; 
  void NonVirtMethod() MOZ_MUST_OVERRIDE; 
  static void StaticMethod() MOZ_MUST_OVERRIDE;
};

struct DoesNotPropagate : Base {
  virtual void VirtMethod();
  void NonVirtMethod();
  static void StaticMethod();
};

struct Final : DoesNotPropagate { };

struct Propagates : Base {
  virtual void VirtMethod() MOZ_MUST_OVERRIDE; 
  void NonVirtMethod() MOZ_MUST_OVERRIDE; 
  static void StaticMethod() MOZ_MUST_OVERRIDE; 
};

struct FailsFinal : Propagates { }; 

struct WrongOverload : Base { 
  virtual void VirtMethod() const;
  void NonVirtMethod(int param);
  static void StaticMethod();
};

namespace A { namespace B { namespace C {
  struct Param {};
  struct Base {
    void f(Param p) MOZ_MUST_OVERRIDE; 
  };
}}}

struct Param {};

struct Derived : A::B::C::Base {
  typedef A::B::C::Param Typedef;
  void f(Typedef t);
};

struct BadDerived : A::B::C::Base { 
  void f(Param p);
};

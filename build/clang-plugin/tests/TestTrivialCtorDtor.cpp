#define MOZ_TRIVIAL_CTOR_DTOR __attribute__((annotate("moz_trivial_ctor_dtor")))

struct MOZ_TRIVIAL_CTOR_DTOR EmptyClass{};

template <class T>
struct MOZ_TRIVIAL_CTOR_DTOR TemplateEmptyClass{};

struct MOZ_TRIVIAL_CTOR_DTOR BadUserDefinedCtor { 
  BadUserDefinedCtor() {}
};

struct MOZ_TRIVIAL_CTOR_DTOR BadUserDefinedDtor { 
  ~BadUserDefinedDtor() {}
};

struct MOZ_TRIVIAL_CTOR_DTOR BadVirtualDtor { 
  virtual ~BadVirtualDtor() {}
};

struct MOZ_TRIVIAL_CTOR_DTOR BadVirtualMember { 
  virtual void f();
};

void foo();
struct MOZ_TRIVIAL_CTOR_DTOR BadNonEmptyCtorDtor { 
  BadNonEmptyCtorDtor() { foo(); }
  ~BadNonEmptyCtorDtor() { foo(); }
};

struct NonTrivialCtor {
  NonTrivialCtor() { foo(); }
};

struct NonTrivialDtor {
  ~NonTrivialDtor() { foo(); }
};

struct VirtualMember {
  virtual void f();
};

struct MOZ_TRIVIAL_CTOR_DTOR BadNonTrivialCtorInBase : NonTrivialCtor { 
};

struct MOZ_TRIVIAL_CTOR_DTOR BadNonTrivialDtorInBase : NonTrivialDtor { 
};

struct MOZ_TRIVIAL_CTOR_DTOR BadNonTrivialCtorInMember { 
  NonTrivialCtor m;
};

struct MOZ_TRIVIAL_CTOR_DTOR BadNonTrivialDtorInMember { 
  NonTrivialDtor m;
};

struct MOZ_TRIVIAL_CTOR_DTOR BadVirtualMemberInMember { 
  VirtualMember m;
};

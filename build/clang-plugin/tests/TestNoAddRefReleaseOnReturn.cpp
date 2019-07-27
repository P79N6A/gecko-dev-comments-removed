#define MOZ_NO_ADDREF_RELEASE_ON_RETURN __attribute__((annotate("moz_no_addref_release_on_return")))

struct Test {
  void AddRef();
  void Release();
  void foo();
};

struct TestD : Test {};

struct S {
  Test* f() MOZ_NO_ADDREF_RELEASE_ON_RETURN;
  Test& g() MOZ_NO_ADDREF_RELEASE_ON_RETURN;
  Test  h() MOZ_NO_ADDREF_RELEASE_ON_RETURN;
};

struct SD {
  TestD* f() MOZ_NO_ADDREF_RELEASE_ON_RETURN;
  TestD& g() MOZ_NO_ADDREF_RELEASE_ON_RETURN;
  TestD  h() MOZ_NO_ADDREF_RELEASE_ON_RETURN;
};

template<class T>
struct X {
  T* f() MOZ_NO_ADDREF_RELEASE_ON_RETURN;
  T& g() MOZ_NO_ADDREF_RELEASE_ON_RETURN;
  T  h() MOZ_NO_ADDREF_RELEASE_ON_RETURN;
};

template<class T>
struct SP {
  T* operator->() MOZ_NO_ADDREF_RELEASE_ON_RETURN;
};

Test* f() MOZ_NO_ADDREF_RELEASE_ON_RETURN;
Test& g() MOZ_NO_ADDREF_RELEASE_ON_RETURN;
Test  h() MOZ_NO_ADDREF_RELEASE_ON_RETURN;

TestD* fd() MOZ_NO_ADDREF_RELEASE_ON_RETURN;
TestD& gd() MOZ_NO_ADDREF_RELEASE_ON_RETURN;
TestD  hd() MOZ_NO_ADDREF_RELEASE_ON_RETURN;

void test() {
  S s;
  s.f()->AddRef(); 
  s.f()->Release(); 
  s.f()->foo();
  s.g().AddRef(); 
  s.g().Release(); 
  s.g().foo();
  s.h().AddRef(); 
  s.h().Release(); 
  s.h().foo();
  SD sd;
  sd.f()->AddRef(); 
  sd.f()->Release(); 
  sd.f()->foo();
  sd.g().AddRef(); 
  sd.g().Release(); 
  sd.g().foo();
  sd.h().AddRef(); 
  sd.h().Release(); 
  sd.h().foo();
  X<Test> x;
  x.f()->AddRef(); 
  x.f()->Release(); 
  x.f()->foo();
  x.g().AddRef(); 
  x.g().Release(); 
  x.g().foo();
  x.h().AddRef(); 
  x.h().Release(); 
  x.h().foo();
  X<TestD> xd;
  xd.f()->AddRef(); 
  xd.f()->Release(); 
  xd.f()->foo();
  xd.g().AddRef(); 
  xd.g().Release(); 
  xd.g().foo();
  xd.h().AddRef(); 
  xd.h().Release(); 
  xd.h().foo();
  SP<Test> sp;
  sp->AddRef(); 
  sp->Release(); 
  sp->foo();
  SP<TestD> spd;
  spd->AddRef(); 
  spd->Release(); 
  spd->foo();
  f()->AddRef(); 
  f()->Release(); 
  f()->foo();
  g().AddRef(); 
  g().Release(); 
  g().foo();
  h().AddRef(); 
  h().Release(); 
  h().foo();
  fd()->AddRef(); 
  fd()->Release(); 
  fd()->foo();
  gd().AddRef(); 
  gd().Release(); 
  gd().foo();
  hd().AddRef(); 
  hd().Release(); 
  hd().foo();
}

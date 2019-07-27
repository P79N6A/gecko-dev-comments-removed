





#include "mozilla/RefPtr.h"

using mozilla::RefCounted;
using mozilla::RefPtr;
using mozilla::TemporaryRef;

class Foo : public RefCounted<Foo>
{
public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(Foo)

  Foo() : mDead(false) {}

  static int sNumDestroyed;

  ~Foo()
  {
    MOZ_ASSERT(!mDead);
    mDead = true;
    sNumDestroyed++;
  }

private:
  bool mDead;
};
int Foo::sNumDestroyed;

struct Bar : public Foo {};

TemporaryRef<Foo>
NewFoo()
{
  return RefPtr<Foo>(new Foo());
}

TemporaryRef<Foo>
NewBar()
{
  return new Bar();
}

void
GetNewFoo(Foo** aFoo)
{
  *aFoo = new Bar();
  
  (*aFoo)->AddRef();
}

void
GetPassedFoo(Foo** aFoo)
{
  
  (*aFoo)->AddRef();
}

void
GetNewFoo(RefPtr<Foo>* aFoo)
{
  *aFoo = new Bar();
}

void
GetPassedFoo(RefPtr<Foo>* aFoo)
{}

TemporaryRef<Foo>
GetNullFoo()
{
  return 0;
}

int
main()
{
  MOZ_RELEASE_ASSERT(0 == Foo::sNumDestroyed);
  {
    RefPtr<Foo> f = new Foo();
    MOZ_RELEASE_ASSERT(f->refCount() == 1);
  }
  MOZ_RELEASE_ASSERT(1 == Foo::sNumDestroyed);

  {
    RefPtr<Foo> f1 = NewFoo();
    RefPtr<Foo> f2(NewFoo());
    MOZ_RELEASE_ASSERT(1 == Foo::sNumDestroyed);
  }
  MOZ_RELEASE_ASSERT(3 == Foo::sNumDestroyed);

  {
    RefPtr<Foo> b = NewBar();
    MOZ_RELEASE_ASSERT(3 == Foo::sNumDestroyed);
  }
  MOZ_RELEASE_ASSERT(4 == Foo::sNumDestroyed);

  {
    RefPtr<Foo> f1;
    {
      f1 = new Foo();
      RefPtr<Foo> f2(f1);
      RefPtr<Foo> f3 = f2;
      MOZ_RELEASE_ASSERT(4 == Foo::sNumDestroyed);
    }
    MOZ_RELEASE_ASSERT(4 == Foo::sNumDestroyed);
  }
  MOZ_RELEASE_ASSERT(5 == Foo::sNumDestroyed);

  {
    RefPtr<Foo> f = new Foo();
    f.forget();
    MOZ_RELEASE_ASSERT(6 == Foo::sNumDestroyed);
  }

  {
    RefPtr<Foo> f = new Foo();
    GetNewFoo(byRef(f));
    MOZ_RELEASE_ASSERT(7 == Foo::sNumDestroyed);
  }
  MOZ_RELEASE_ASSERT(8 == Foo::sNumDestroyed);

  {
    RefPtr<Foo> f = new Foo();
    GetPassedFoo(byRef(f));
    MOZ_RELEASE_ASSERT(8 == Foo::sNumDestroyed);
  }
  MOZ_RELEASE_ASSERT(9 == Foo::sNumDestroyed);

  {
    RefPtr<Foo> f = new Foo();
    GetNewFoo(&f);
    MOZ_RELEASE_ASSERT(10 == Foo::sNumDestroyed);
  }
  MOZ_RELEASE_ASSERT(11 == Foo::sNumDestroyed);

  {
    RefPtr<Foo> f = new Foo();
    GetPassedFoo(&f);
    MOZ_RELEASE_ASSERT(11 == Foo::sNumDestroyed);
  }
  MOZ_RELEASE_ASSERT(12 == Foo::sNumDestroyed);

  {
    RefPtr<Foo> f1 = new Bar();
  }
  MOZ_RELEASE_ASSERT(13 == Foo::sNumDestroyed);

  {
    RefPtr<Foo> f = GetNullFoo();
    MOZ_RELEASE_ASSERT(13 == Foo::sNumDestroyed);
  }
  MOZ_RELEASE_ASSERT(13 == Foo::sNumDestroyed);

  return 0;
}


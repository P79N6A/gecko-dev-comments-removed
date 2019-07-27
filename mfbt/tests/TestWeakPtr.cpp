





#include "mozilla/WeakPtr.h"

using mozilla::SupportsWeakPtr;
using mozilla::WeakPtr;


class C : public SupportsWeakPtr<C>
{
public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(C)
  int mNum;
  void act() {}
};

static void
Example()
{
  C* ptr = new C();

  
  
  
  
  WeakPtr<C> weak = ptr;
  WeakPtr<C> other = ptr;

  
  if (weak) {
    weak->mNum = 17;
    weak->act();
  }

  
  delete ptr;

  MOZ_RELEASE_ASSERT(!weak, "Deleting |ptr| clears weak pointers to it.");
  MOZ_RELEASE_ASSERT(!other, "Deleting |ptr| clears all weak pointers to it.");
}

struct A : public SupportsWeakPtr<A>
{
  MOZ_DECLARE_REFCOUNTED_TYPENAME(A)
  int mData;
};

int
main()
{
  A* a = new A;

  
  
  A* a2 = new A;

  a->mData = 5;
  WeakPtr<A> ptr = a;
  {
    WeakPtr<A> ptr2 = a;
    MOZ_RELEASE_ASSERT(ptr->mData == 5);
    WeakPtr<A> ptr3 = a;
    MOZ_RELEASE_ASSERT(ptr->mData == 5);
  }

  delete a;
  MOZ_RELEASE_ASSERT(!ptr);

  delete a2;

  Example();

  return 0;
}

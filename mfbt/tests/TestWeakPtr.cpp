



#include "mozilla/WeakPtr.h"

using mozilla::SupportsWeakPtr;
using mozilla::WeakPtr;


class C : public SupportsWeakPtr<C>
{
  public:
    int num;
    void act() {}
};

static void
Example()
{

  C* ptr =  new C();

  
  
  
  
  WeakPtr<C> weak = ptr->asWeakPtr();
  WeakPtr<C> other = ptr->asWeakPtr();

  
  if (weak) {
    weak->num = 17;
    weak->act();
  }

  
  delete ptr;

  MOZ_ASSERT(!weak, "Deleting |ptr| clears weak pointers to it.");
  MOZ_ASSERT(!other, "Deleting |ptr| clears all weak pointers to it.");
}

struct A : public SupportsWeakPtr<A>
{
    int data;
};


int
main()
{

  A* a = new A;

  
  
  A* a2 = new A;

  a->data = 5;
  WeakPtr<A> ptr = a->asWeakPtr();
  {
      WeakPtr<A> ptr2 = a->asWeakPtr();
      MOZ_ASSERT(ptr->data == 5);
      WeakPtr<A> ptr3 = a->asWeakPtr();
      MOZ_ASSERT(ptr->data == 5);
  }

  delete a;
  MOZ_ASSERT(!ptr);

  delete a2;

  Example();

  return 0;
}

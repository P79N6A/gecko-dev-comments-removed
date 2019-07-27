





#include "mozilla/WeakPtr.h"

using mozilla::SupportsWeakPtr;
using mozilla::WeakPtr;


class C : public SupportsWeakPtr<C>
{
public:
  MOZ_DECLARE_WEAKREFERENCE_TYPENAME(C)

  int mNum;

  C()
    : mNum(0)
  {}

  ~C()
  {
    
    mNum = 0xDEAD;
  }

  void act() {}

  bool isConst() {
    return false;
  }

  bool isConst() const {
    return true;
  }
};

bool isConst(C*)
{
  return false;
}

bool isConst(const C*)
{
  return true;
}

int
main()
{
  C* c1 = new C;
  MOZ_RELEASE_ASSERT(c1->mNum == 0);

  
  
  
  
  WeakPtr<C> w1 = c1;
  
  MOZ_RELEASE_ASSERT(w1);
  MOZ_RELEASE_ASSERT(w1 == c1);
  w1->mNum = 1;
  w1->act();

  
  WeakPtr<C> w2 = c1;
  MOZ_RELEASE_ASSERT(w2);
  MOZ_RELEASE_ASSERT(w2 == c1);
  MOZ_RELEASE_ASSERT(w2 == w1);
  MOZ_RELEASE_ASSERT(w2->mNum == 1);

  
  WeakPtr<const C> w3const = c1;
  MOZ_RELEASE_ASSERT(w3const);
  MOZ_RELEASE_ASSERT(w3const == c1);
  MOZ_RELEASE_ASSERT(w3const == w1);
  MOZ_RELEASE_ASSERT(w3const == w2);
  MOZ_RELEASE_ASSERT(w3const->mNum == 1);

  
  MOZ_RELEASE_ASSERT(!w1->isConst());
  MOZ_RELEASE_ASSERT(w3const->isConst());
  MOZ_RELEASE_ASSERT(!isConst(w1));
  MOZ_RELEASE_ASSERT(isConst(w3const));

  
  
  
  {
    WeakPtr<C> w4local = c1;
    MOZ_RELEASE_ASSERT(w4local == c1);
  }
  
  
  MOZ_RELEASE_ASSERT(c1->mNum == 1);
  
  MOZ_RELEASE_ASSERT(w1 == c1);
  MOZ_RELEASE_ASSERT(w2 == c1);

  
  C* c2 = new C;
  c2->mNum = 2;
  MOZ_RELEASE_ASSERT(w2->mNum == 1); 
  w2 = c2;
  MOZ_RELEASE_ASSERT(w2);
  MOZ_RELEASE_ASSERT(w2 == c2);
  MOZ_RELEASE_ASSERT(w2 != c1);
  MOZ_RELEASE_ASSERT(w2 != w1);
  MOZ_RELEASE_ASSERT(w2->mNum == 2);

  
  
  delete c1;
  MOZ_RELEASE_ASSERT(!w1, "Deleting an object should clear WeakPtr's to it.");
  MOZ_RELEASE_ASSERT(!w3const, "Deleting an object should clear WeakPtr's to it.");
  MOZ_RELEASE_ASSERT(w2, "Deleting an object should not clear WeakPtr that are not pointing to it.");

  delete c2;
  MOZ_RELEASE_ASSERT(!w2, "Deleting an object should clear WeakPtr's to it.");
}

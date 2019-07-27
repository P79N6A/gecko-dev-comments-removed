







#include "mozilla/SegmentedVector.h"

#include "mozilla/Alignment.h"
#include "mozilla/Assertions.h"

using mozilla::SegmentedVector;



class InfallibleAllocPolicy
{
public:
  template <typename T>
  T* pod_malloc(size_t aNumElems)
  {
    if (aNumElems & mozilla::tl::MulOverflowMask<sizeof(T)>::value) {
      MOZ_CRASH("TestSegmentedVector.cpp: overflow");
    }
    T* rv = static_cast<T*>(malloc(aNumElems * sizeof(T)));
    if (!rv) {
      MOZ_CRASH("TestSegmentedVector.cpp: out of memory");
    }
    return rv;
  }

  void free_(void* aPtr) { free(aPtr); }
};






static int gDummy;


void TestBasics()
{
  
  typedef SegmentedVector<int, 1024, InfallibleAllocPolicy> MyVector;
  MyVector v;
  int i, n;

  MOZ_RELEASE_ASSERT(v.IsEmpty());

  
  i = 0;
  for ( ; i < 100; i++) {
    gDummy = v.Append(mozilla::Move(i));
  }
  MOZ_RELEASE_ASSERT(!v.IsEmpty());
  MOZ_RELEASE_ASSERT(v.Length() == 100);

  n = 0;
  for (auto iter = v.Iter(); !iter.Done(); iter.Next()) {
    MOZ_RELEASE_ASSERT(iter.Get() == n);
    n++;
  }
  MOZ_RELEASE_ASSERT(n == 100);

  
  for ( ; i < 1000; i++) {
    v.InfallibleAppend(mozilla::Move(i));
  }
  MOZ_RELEASE_ASSERT(!v.IsEmpty());
  MOZ_RELEASE_ASSERT(v.Length() == 1000);

  n = 0;
  for (auto iter = v.Iter(); !iter.Done(); iter.Next()) {
    MOZ_RELEASE_ASSERT(iter.Get() == n);
    n++;
  }
  MOZ_RELEASE_ASSERT(n == 1000);

  v.Clear();
  MOZ_RELEASE_ASSERT(v.IsEmpty());
  MOZ_RELEASE_ASSERT(v.Length() == 0);
}

static size_t gNumDefaultCtors;
static size_t gNumExplicitCtors;
static size_t gNumCopyCtors;
static size_t gNumMoveCtors;
static size_t gNumDtors;

struct NonPOD
{
  NonPOD()                { gNumDefaultCtors++; }
  explicit NonPOD(int x)  { gNumExplicitCtors++; }
  NonPOD(NonPOD&)         { gNumCopyCtors++; }
  NonPOD(NonPOD&&)        { gNumMoveCtors++; }
  ~NonPOD()               { gNumDtors++; }
};



void TestConstructorsAndDestructors()
{
  {
    
    NonPOD x(1);                          
    SegmentedVector<NonPOD, 64, InfallibleAllocPolicy> v;
                                          
    MOZ_RELEASE_ASSERT(v.IsEmpty());
    gDummy = v.Append(x);                 
    NonPOD y(1);                          
    gDummy = v.Append(mozilla::Move(y));  
    NonPOD z(1);                          
    v.InfallibleAppend(mozilla::Move(z)); 
    v.Clear();                            

    MOZ_RELEASE_ASSERT(gNumDefaultCtors  == 0);
    MOZ_RELEASE_ASSERT(gNumExplicitCtors == 3);
    MOZ_RELEASE_ASSERT(gNumCopyCtors     == 1);
    MOZ_RELEASE_ASSERT(gNumMoveCtors     == 2);
    MOZ_RELEASE_ASSERT(gNumDtors         == 3);
  }                                       
  MOZ_RELEASE_ASSERT(gNumDtors == 6);
}

struct A { int mX; int mY; };
struct B { int mX; char mY; double mZ; };
struct C { A mA; B mB; };
struct D { char mBuf[101]; };
struct E { };



void TestSegmentCapacitiesAndAlignments()
{
  
  
  
  
  
  
  SegmentedVector<double, 512> v1(512);
  SegmentedVector<A, 1024> v2(1024);
  SegmentedVector<B, 999> v3(999);
  SegmentedVector<C, 10> v4(10);
  SegmentedVector<D, 1234> v5(1234);
  SegmentedVector<E> v6(4096);  
  SegmentedVector<mozilla::AlignedElem<16>, 100> v7(100);
}

int main(void)
{
  TestBasics();
  TestConstructorsAndDestructors();
  TestSegmentCapacitiesAndAlignments();

  return 0;
}

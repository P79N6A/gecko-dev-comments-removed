





#include "mozilla/Assertions.h"
#include "mozilla/BinarySearch.h"
#include "mozilla/Vector.h"

using mozilla::Vector;
using mozilla::BinarySearch;

struct Person
{
  int mAge;
  int mId;
  Person(int aAge, int aId) : mAge(aAge), mId(aId) {}
};

struct GetAge
{
  Vector<Person>& mV;
  explicit GetAge(Vector<Person>& aV) : mV(aV) {}
  int operator[](size_t index) const { return mV[index].mAge; }
};

int
main()
{
  size_t m;

  Vector<int> v1;
  v1.append(2);
  v1.append(4);
  v1.append(6);
  v1.append(8);

  MOZ_RELEASE_ASSERT(!BinarySearch(v1, 0, v1.length(), 1, &m) && m == 0);
  MOZ_RELEASE_ASSERT( BinarySearch(v1, 0, v1.length(), 2, &m) && m == 0);
  MOZ_RELEASE_ASSERT(!BinarySearch(v1, 0, v1.length(), 3, &m) && m == 1);
  MOZ_RELEASE_ASSERT( BinarySearch(v1, 0, v1.length(), 4, &m) && m == 1);
  MOZ_RELEASE_ASSERT(!BinarySearch(v1, 0, v1.length(), 5, &m) && m == 2);
  MOZ_RELEASE_ASSERT( BinarySearch(v1, 0, v1.length(), 6, &m) && m == 2);
  MOZ_RELEASE_ASSERT(!BinarySearch(v1, 0, v1.length(), 7, &m) && m == 3);
  MOZ_RELEASE_ASSERT( BinarySearch(v1, 0, v1.length(), 8, &m) && m == 3);
  MOZ_RELEASE_ASSERT(!BinarySearch(v1, 0, v1.length(), 9, &m) && m == 4);

  MOZ_RELEASE_ASSERT(!BinarySearch(v1, 1, 3, 1, &m) && m == 1);
  MOZ_RELEASE_ASSERT(!BinarySearch(v1, 1, 3, 2, &m) && m == 1);
  MOZ_RELEASE_ASSERT(!BinarySearch(v1, 1, 3, 3, &m) && m == 1);
  MOZ_RELEASE_ASSERT( BinarySearch(v1, 1, 3, 4, &m) && m == 1);
  MOZ_RELEASE_ASSERT(!BinarySearch(v1, 1, 3, 5, &m) && m == 2);
  MOZ_RELEASE_ASSERT( BinarySearch(v1, 1, 3, 6, &m) && m == 2);
  MOZ_RELEASE_ASSERT(!BinarySearch(v1, 1, 3, 7, &m) && m == 3);
  MOZ_RELEASE_ASSERT(!BinarySearch(v1, 1, 3, 8, &m) && m == 3);
  MOZ_RELEASE_ASSERT(!BinarySearch(v1, 1, 3, 9, &m) && m == 3);

  MOZ_RELEASE_ASSERT(!BinarySearch(v1, 0, 0, 0, &m) && m == 0);
  MOZ_RELEASE_ASSERT(!BinarySearch(v1, 0, 0, 9, &m) && m == 0);

  Vector<int> v2;
  MOZ_RELEASE_ASSERT(!BinarySearch(v2, 0, 0, 0, &m) && m == 0);
  MOZ_RELEASE_ASSERT(!BinarySearch(v2, 0, 0, 9, &m) && m == 0);

  Vector<Person> v3;
  v3.append(Person(2, 42));
  v3.append(Person(4, 13));
  v3.append(Person(6, 360));

  #define A(a) MOZ_RELEASE_ASSERT(a)
  A(!BinarySearch(GetAge(v3), 0, v3.length(), 1, &m) && m == 0);
  A( BinarySearch(GetAge(v3), 0, v3.length(), 2, &m) && m == 0);
  A(!BinarySearch(GetAge(v3), 0, v3.length(), 3, &m) && m == 1);
  A( BinarySearch(GetAge(v3), 0, v3.length(), 4, &m) && m == 1);
  A(!BinarySearch(GetAge(v3), 0, v3.length(), 5, &m) && m == 2);
  A( BinarySearch(GetAge(v3), 0, v3.length(), 6, &m) && m == 2);
  A(!BinarySearch(GetAge(v3), 0, v3.length(), 7, &m) && m == 3);
  return 0;
}







#include "nsTArray.h"
#include "gtest/gtest.h"

using namespace mozilla;

namespace TestTArray {

const nsTArray<int>& DummyArray()
{
  static nsTArray<int> sArray;
  if (sArray.IsEmpty()) {
    const int data[] = {4, 1, 2, 8};
    sArray.AppendElements(data, ArrayLength(data));
  }
  return sArray;
}



#ifdef DEBUG
const nsTArray<int>& FakeHugeArray()
{
  static nsTArray<int> sArray;
  if (sArray.IsEmpty()) {
    sArray.AppendElement();
    ((nsTArrayHeader*)sArray.DebugGetHeader())->mLength = UINT32_MAX;
  }
  return sArray;
}
#endif

TEST(TArray, Assign)
{
  nsTArray<int> array;
  array.Assign(DummyArray());
  ASSERT_EQ(DummyArray(), array);

  ASSERT_TRUE(array.Assign(DummyArray(), fallible));
  ASSERT_EQ(DummyArray(), array);

#ifdef DEBUG
  ASSERT_FALSE(array.Assign(FakeHugeArray(), fallible));
#endif

  nsTArray<int> array2;
  array2.Assign(Move(array));
  ASSERT_TRUE(array.IsEmpty());
  ASSERT_EQ(DummyArray(), array2);
}

TEST(TArray, AssignmentOperatorSelfAssignment)
{
  nsTArray<int> array;
  array = DummyArray();

  array = array;
  ASSERT_EQ(DummyArray(), array);
  array = Move(array);
  ASSERT_EQ(DummyArray(), array);
}

} 

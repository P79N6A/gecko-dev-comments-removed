






























#include "gtest/internal/gtest-tuple.h"
#include <utility>
#include "gtest/gtest.h"

namespace {

using ::std::tr1::get;
using ::std::tr1::make_tuple;
using ::std::tr1::tuple;
using ::std::tr1::tuple_element;
using ::std::tr1::tuple_size;
using ::testing::StaticAssertTypeEq;


TEST(tuple_element_Test, ReturnsElementType) {
  StaticAssertTypeEq<int, tuple_element<0, tuple<int, char> >::type>();
  StaticAssertTypeEq<int&, tuple_element<1, tuple<double, int&> >::type>();
  StaticAssertTypeEq<bool, tuple_element<2, tuple<double, int, bool> >::type>();
}



TEST(tuple_size_Test, ReturnsNumberOfFields) {
  EXPECT_EQ(0, +tuple_size<tuple<> >::value);
  EXPECT_EQ(1, +tuple_size<tuple<void*> >::value);
  EXPECT_EQ(1, +tuple_size<tuple<char> >::value);
  EXPECT_EQ(1, +(tuple_size<tuple<tuple<int, double> > >::value));
  EXPECT_EQ(2, +(tuple_size<tuple<int&, const char> >::value));
  EXPECT_EQ(3, +(tuple_size<tuple<char*, void, const bool&> >::value));
}


TEST(ComparisonTest, ComparesWithSelf) {
  const tuple<int, char, bool> a(5, 'a', false);

  EXPECT_TRUE(a == a);
  EXPECT_FALSE(a != a);
}


TEST(ComparisonTest, ComparesEqualTuples) {
  const tuple<int, bool> a(5, true), b(5, true);

  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
}


TEST(ComparisonTest, ComparesUnequalTuplesWithoutReferenceFields) {
  typedef tuple<const int, char> FooTuple;

  const FooTuple a(0, 'x');
  const FooTuple b(1, 'a');

  EXPECT_TRUE(a != b);
  EXPECT_FALSE(a == b);

  const FooTuple c(1, 'b');

  EXPECT_TRUE(b != c);
  EXPECT_FALSE(b == c);
}


TEST(ComparisonTest, ComparesUnequalTuplesWithReferenceFields) {
  typedef tuple<int&, const char&> FooTuple;

  int i = 5;
  const char ch = 'a';
  const FooTuple a(i, ch);

  int j = 6;
  const FooTuple b(j, ch);

  EXPECT_TRUE(a != b);
  EXPECT_FALSE(a == b);

  j = 5;
  const char ch2 = 'b';
  const FooTuple c(j, ch2);

  EXPECT_TRUE(b != c);
  EXPECT_FALSE(b == c);
}



TEST(ReferenceFieldTest, IsAliasOfReferencedVariable) {
  int n = 0;
  tuple<bool, int&> t(true, n);

  n = 1;
  EXPECT_EQ(n, get<1>(t))
      << "Changing a underlying variable should update the reference field.";

  
  
  EXPECT_EQ(&n, &(get<1>(t)))
      << "The address of a reference field should equal the address of "
      << "the underlying variable.";

  get<1>(t) = 2;
  EXPECT_EQ(2, n)
      << "Changing a reference field should update the underlying variable.";
}



TEST(TupleConstructorTest, DefaultConstructorDefaultInitializesEachField) {
  
  
  
  
  

  tuple<> empty;

  tuple<int> a1, b1;
  b1 = a1;
  EXPECT_EQ(0, get<0>(b1));

  tuple<int, double> a2, b2;
  b2 = a2;
  EXPECT_EQ(0, get<0>(b2));
  EXPECT_EQ(0.0, get<1>(b2));

  tuple<double, char, bool*> a3, b3;
  b3 = a3;
  EXPECT_EQ(0.0, get<0>(b3));
  EXPECT_EQ('\0', get<1>(b3));
  EXPECT_TRUE(get<2>(b3) == NULL);

  tuple<int, int, int, int, int, int, int, int, int, int> a10, b10;
  b10 = a10;
  EXPECT_EQ(0, get<0>(b10));
  EXPECT_EQ(0, get<1>(b10));
  EXPECT_EQ(0, get<2>(b10));
  EXPECT_EQ(0, get<3>(b10));
  EXPECT_EQ(0, get<4>(b10));
  EXPECT_EQ(0, get<5>(b10));
  EXPECT_EQ(0, get<6>(b10));
  EXPECT_EQ(0, get<7>(b10));
  EXPECT_EQ(0, get<8>(b10));
  EXPECT_EQ(0, get<9>(b10));
}


TEST(TupleConstructorTest, ConstructsFromFields) {
  int n = 1;
  
  tuple<int&> a(n);
  EXPECT_EQ(&n, &(get<0>(a)));

  
  tuple<int, char> b(5, 'a');
  EXPECT_EQ(5, get<0>(b));
  EXPECT_EQ('a', get<1>(b));

  
  const int m = 2;
  tuple<bool, const int&> c(true, m);
  EXPECT_TRUE(get<0>(c));
  EXPECT_EQ(&m, &(get<1>(c)));
}


TEST(TupleConstructorTest, CopyConstructor) {
  tuple<double, bool> a(0.0, true);
  tuple<double, bool> b(a);

  EXPECT_DOUBLE_EQ(0.0, get<0>(b));
  EXPECT_TRUE(get<1>(b));
}



TEST(TupleConstructorTest, ConstructsFromDifferentTupleType) {
  tuple<int, int, char> a(0, 1, 'a');
  tuple<double, long, int> b(a);

  EXPECT_DOUBLE_EQ(0.0, get<0>(b));
  EXPECT_EQ(1, get<1>(b));
  EXPECT_EQ('a', get<2>(b));
}


TEST(TupleConstructorTest, ConstructsFromPair) {
  ::std::pair<int, char> a(1, 'a');
  tuple<int, char> b(a);
  tuple<int, const char&> c(a);
}


TEST(TupleAssignmentTest, AssignsToSameTupleType) {
  const tuple<int, long> a(5, 7L);
  tuple<int, long> b;
  b = a;
  EXPECT_EQ(5, get<0>(b));
  EXPECT_EQ(7L, get<1>(b));
}



TEST(TupleAssignmentTest, AssignsToDifferentTupleType) {
  const tuple<int, long, bool> a(1, 7L, true);
  tuple<long, int, bool> b;
  b = a;
  EXPECT_EQ(1L, get<0>(b));
  EXPECT_EQ(7, get<1>(b));
  EXPECT_TRUE(get<2>(b));
}


TEST(TupleAssignmentTest, AssignsFromPair) {
  const ::std::pair<int, bool> a(5, true);
  tuple<int, bool> b;
  b = a;
  EXPECT_EQ(5, get<0>(b));
  EXPECT_TRUE(get<1>(b));

  tuple<long, bool> c;
  c = a;
  EXPECT_EQ(5L, get<0>(c));
  EXPECT_TRUE(get<1>(c));
}


class BigTupleTest : public testing::Test {
 protected:
  typedef tuple<int, int, int, int, int, int, int, int, int, int> BigTuple;

  BigTupleTest() :
      a_(1, 0, 0, 0, 0, 0, 0, 0, 0, 2),
      b_(1, 0, 0, 0, 0, 0, 0, 0, 0, 3) {}

  BigTuple a_, b_;
};


TEST_F(BigTupleTest, Construction) {
  BigTuple a;
  BigTuple b(b_);
}


TEST_F(BigTupleTest, get) {
  EXPECT_EQ(1, get<0>(a_));
  EXPECT_EQ(2, get<9>(a_));

  
  const BigTuple a(a_);
  EXPECT_EQ(1, get<0>(a));
  EXPECT_EQ(2, get<9>(a));
}


TEST_F(BigTupleTest, Comparisons) {
  EXPECT_TRUE(a_ == a_);
  EXPECT_FALSE(a_ != a_);

  EXPECT_TRUE(a_ != b_);
  EXPECT_FALSE(a_ == b_);
}

TEST(MakeTupleTest, WorksForScalarTypes) {
  tuple<bool, int> a;
  a = make_tuple(true, 5);
  EXPECT_TRUE(get<0>(a));
  EXPECT_EQ(5, get<1>(a));

  tuple<char, int, long> b;
  b = make_tuple('a', 'b', 5);
  EXPECT_EQ('a', get<0>(b));
  EXPECT_EQ('b', get<1>(b));
  EXPECT_EQ(5, get<2>(b));
}

TEST(MakeTupleTest, WorksForPointers) {
  int a[] = { 1, 2, 3, 4 };
  const char* const str = "hi";
  int* const p = a;

  tuple<const char*, int*> t;
  t = make_tuple(str, p);
  EXPECT_EQ(str, get<0>(t));
  EXPECT_EQ(p, get<1>(t));
}

}  

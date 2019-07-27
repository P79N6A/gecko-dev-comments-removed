







































#include "gtest/gtest.h"


TEST(Foo, Bar1) {
}

TEST(Foo, Bar2) {
}

TEST(Foo, DISABLED_Bar3) {
}

TEST(Abc, Xyz) {
}

TEST(Abc, Def) {
}

TEST(FooBar, Baz) {
}

class FooTest : public testing::Test {
};

TEST_F(FooTest, Test1) {
}

TEST_F(FooTest, DISABLED_Test2) {
}

TEST_F(FooTest, Test3) {
}

TEST(FooDeathTest, Test1) {
}



class MyType {
 public:
  explicit MyType(const std::string& a_value) : value_(a_value) {}

  const std::string& value() const { return value_; }

 private:
  std::string value_;
};


void PrintTo(const MyType& x, std::ostream* os) {
  *os << x.value();
}

class ValueParamTest : public testing::TestWithParam<MyType> {
};

TEST_P(ValueParamTest, TestA) {
}

TEST_P(ValueParamTest, TestB) {
}

INSTANTIATE_TEST_CASE_P(
    MyInstantiation, ValueParamTest,
    testing::Values(MyType("one line"),
                    MyType("two\nlines"),
                    MyType("a very\nloooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong line")));  





class VeryLoooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooogName {  
};

template <typename T>
class TypedTest : public testing::Test {
};

template <typename T, int kSize>
class MyArray {
};

typedef testing::Types<VeryLoooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooogName,  
                       int*, MyArray<bool, 42> > MyTypes;

TYPED_TEST_CASE(TypedTest, MyTypes);

TYPED_TEST(TypedTest, TestA) {
}

TYPED_TEST(TypedTest, TestB) {
}



template <typename T>
class TypeParamTest : public testing::Test {
};

TYPED_TEST_CASE_P(TypeParamTest);

TYPED_TEST_P(TypeParamTest, TestA) {
}

TYPED_TEST_P(TypeParamTest, TestB) {
}

REGISTER_TYPED_TEST_CASE_P(TypeParamTest, TestA, TestB);

INSTANTIATE_TYPED_TEST_CASE_P(My, TypeParamTest, MyTypes);

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}

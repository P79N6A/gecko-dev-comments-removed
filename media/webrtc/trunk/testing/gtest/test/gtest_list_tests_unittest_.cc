







































#include "gtest/gtest.h"

namespace {


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

}  

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}

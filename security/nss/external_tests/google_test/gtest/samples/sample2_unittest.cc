









































#include "sample2.h"
#include "gtest/gtest.h"




TEST(MyString, DefaultConstructor) {
  const MyString s;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  EXPECT_STREQ(NULL, s.c_string());

  EXPECT_EQ(0u, s.Length());
}

const char kHelloString[] = "Hello, world!";


TEST(MyString, ConstructorFromCString) {
  const MyString s(kHelloString);
  EXPECT_EQ(0, strcmp(s.c_string(), kHelloString));
  EXPECT_EQ(sizeof(kHelloString)/sizeof(kHelloString[0]) - 1,
            s.Length());
}


TEST(MyString, CopyConstructor) {
  const MyString s1(kHelloString);
  const MyString s2 = s1;
  EXPECT_EQ(0, strcmp(s2.c_string(), kHelloString));
}


TEST(MyString, Set) {
  MyString s;

  s.Set(kHelloString);
  EXPECT_EQ(0, strcmp(s.c_string(), kHelloString));

  
  
  s.Set(s.c_string());
  EXPECT_EQ(0, strcmp(s.c_string(), kHelloString));

  
  s.Set(NULL);
  EXPECT_STREQ(NULL, s.c_string());
}




































#include "gmock/gmock-more-actions.h"

#include <functional>
#include <sstream>
#include <string>
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "gtest/internal/gtest-linked_ptr.h"

namespace testing {
namespace gmock_more_actions_test {

using ::std::plus;
using ::std::string;
using ::std::tr1::get;
using ::std::tr1::make_tuple;
using ::std::tr1::tuple;
using ::std::tr1::tuple_element;
using testing::_;
using testing::Action;
using testing::ActionInterface;
using testing::DeleteArg;
using testing::Invoke;
using testing::Return;
using testing::ReturnArg;
using testing::ReturnPointee;
using testing::SaveArg;
using testing::SaveArgPointee;
using testing::SetArgReferee;
using testing::StaticAssertTypeEq;
using testing::Unused;
using testing::WithArg;
using testing::WithoutArgs;
using testing::internal::linked_ptr;


inline short Short(short n) { return n; }  
inline char Char(char ch) { return ch; }


int Nullary() { return 1; }

class NullaryFunctor {
 public:
  int operator()() { return 2; }
};

bool g_done = false;
void VoidNullary() { g_done = true; }

class VoidNullaryFunctor {
 public:
  void operator()() { g_done = true; }
};

bool Unary(int x) { return x < 0; }

const char* Plus1(const char* s) { return s + 1; }

void VoidUnary(int ) { g_done = true; }

bool ByConstRef(const string& s) { return s == "Hi"; }

const double g_double = 0;
bool ReferencesGlobalDouble(const double& x) { return &x == &g_double; }

string ByNonConstRef(string& s) { return s += "+"; }  

struct UnaryFunctor {
  int operator()(bool x) { return x ? 1 : -1; }
};

const char* Binary(const char* input, short n) { return input + n; }  

void VoidBinary(int, char) { g_done = true; }

int Ternary(int x, char y, short z) { return x + y + z; }  

void VoidTernary(int, char, bool) { g_done = true; }

int SumOf4(int a, int b, int c, int d) { return a + b + c + d; }

int SumOfFirst2(int a, int b, Unused, Unused) { return a + b; }

void VoidFunctionWithFourArguments(char, int, float, double) { g_done = true; }

string Concat4(const char* s1, const char* s2, const char* s3,
               const char* s4) {
  return string(s1) + s2 + s3 + s4;
}

int SumOf5(int a, int b, int c, int d, int e) { return a + b + c + d + e; }

struct SumOf5Functor {
  int operator()(int a, int b, int c, int d, int e) {
    return a + b + c + d + e;
  }
};

string Concat5(const char* s1, const char* s2, const char* s3,
               const char* s4, const char* s5) {
  return string(s1) + s2 + s3 + s4 + s5;
}

int SumOf6(int a, int b, int c, int d, int e, int f) {
  return a + b + c + d + e + f;
}

struct SumOf6Functor {
  int operator()(int a, int b, int c, int d, int e, int f) {
    return a + b + c + d + e + f;
  }
};

string Concat6(const char* s1, const char* s2, const char* s3,
               const char* s4, const char* s5, const char* s6) {
  return string(s1) + s2 + s3 + s4 + s5 + s6;
}

string Concat7(const char* s1, const char* s2, const char* s3,
               const char* s4, const char* s5, const char* s6,
               const char* s7) {
  return string(s1) + s2 + s3 + s4 + s5 + s6 + s7;
}

string Concat8(const char* s1, const char* s2, const char* s3,
               const char* s4, const char* s5, const char* s6,
               const char* s7, const char* s8) {
  return string(s1) + s2 + s3 + s4 + s5 + s6 + s7 + s8;
}

string Concat9(const char* s1, const char* s2, const char* s3,
               const char* s4, const char* s5, const char* s6,
               const char* s7, const char* s8, const char* s9) {
  return string(s1) + s2 + s3 + s4 + s5 + s6 + s7 + s8 + s9;
}

string Concat10(const char* s1, const char* s2, const char* s3,
                const char* s4, const char* s5, const char* s6,
                const char* s7, const char* s8, const char* s9,
                const char* s10) {
  return string(s1) + s2 + s3 + s4 + s5 + s6 + s7 + s8 + s9 + s10;
}

class Foo {
 public:
  Foo() : value_(123) {}

  int Nullary() const { return value_; }

  short Unary(long x) { return static_cast<short>(value_ + x); }  

  string Binary(const string& str, char c) const { return str + c; }

  int Ternary(int x, bool y, char z) { return value_ + x + y*z; }

  int SumOf4(int a, int b, int c, int d) const {
    return a + b + c + d + value_;
  }

  int SumOfLast2(Unused, Unused, int a, int b) const { return a + b; }

  int SumOf5(int a, int b, int c, int d, int e) { return a + b + c + d + e; }

  int SumOf6(int a, int b, int c, int d, int e, int f) {
    return a + b + c + d + e + f;
  }

  string Concat7(const char* s1, const char* s2, const char* s3,
                 const char* s4, const char* s5, const char* s6,
                 const char* s7) {
    return string(s1) + s2 + s3 + s4 + s5 + s6 + s7;
  }

  string Concat8(const char* s1, const char* s2, const char* s3,
                 const char* s4, const char* s5, const char* s6,
                 const char* s7, const char* s8) {
    return string(s1) + s2 + s3 + s4 + s5 + s6 + s7 + s8;
  }

  string Concat9(const char* s1, const char* s2, const char* s3,
                 const char* s4, const char* s5, const char* s6,
                 const char* s7, const char* s8, const char* s9) {
    return string(s1) + s2 + s3 + s4 + s5 + s6 + s7 + s8 + s9;
  }

  string Concat10(const char* s1, const char* s2, const char* s3,
                  const char* s4, const char* s5, const char* s6,
                  const char* s7, const char* s8, const char* s9,
                  const char* s10) {
    return string(s1) + s2 + s3 + s4 + s5 + s6 + s7 + s8 + s9 + s10;
  }
 private:
  int value_;
};


TEST(InvokeTest, Nullary) {
  Action<int()> a = Invoke(Nullary);  
  EXPECT_EQ(1, a.Perform(make_tuple()));
}


TEST(InvokeTest, Unary) {
  Action<bool(int)> a = Invoke(Unary);  
  EXPECT_FALSE(a.Perform(make_tuple(1)));
  EXPECT_TRUE(a.Perform(make_tuple(-1)));
}


TEST(InvokeTest, Binary) {
  Action<const char*(const char*, short)> a = Invoke(Binary);  
  const char* p = "Hello";
  EXPECT_EQ(p + 2, a.Perform(make_tuple(p, Short(2))));
}


TEST(InvokeTest, Ternary) {
  Action<int(int, char, short)> a = Invoke(Ternary);  
  EXPECT_EQ(6, a.Perform(make_tuple(1, '\2', Short(3))));
}


TEST(InvokeTest, FunctionThatTakes4Arguments) {
  Action<int(int, int, int, int)> a = Invoke(SumOf4);  
  EXPECT_EQ(1234, a.Perform(make_tuple(1000, 200, 30, 4)));
}


TEST(InvokeTest, FunctionThatTakes5Arguments) {
  Action<int(int, int, int, int, int)> a = Invoke(SumOf5);  
  EXPECT_EQ(12345, a.Perform(make_tuple(10000, 2000, 300, 40, 5)));
}


TEST(InvokeTest, FunctionThatTakes6Arguments) {
  Action<int(int, int, int, int, int, int)> a = Invoke(SumOf6);  
  EXPECT_EQ(123456, a.Perform(make_tuple(100000, 20000, 3000, 400, 50, 6)));
}



inline const char* CharPtr(const char* s) { return s; }


TEST(InvokeTest, FunctionThatTakes7Arguments) {
  Action<string(const char*, const char*, const char*, const char*,
                const char*, const char*, const char*)> a =
      Invoke(Concat7);
  EXPECT_EQ("1234567",
            a.Perform(make_tuple(CharPtr("1"), CharPtr("2"), CharPtr("3"),
                                 CharPtr("4"), CharPtr("5"), CharPtr("6"),
                                 CharPtr("7"))));
}


TEST(InvokeTest, FunctionThatTakes8Arguments) {
  Action<string(const char*, const char*, const char*, const char*,
                const char*, const char*, const char*, const char*)> a =
      Invoke(Concat8);
  EXPECT_EQ("12345678",
            a.Perform(make_tuple(CharPtr("1"), CharPtr("2"), CharPtr("3"),
                                 CharPtr("4"), CharPtr("5"), CharPtr("6"),
                                 CharPtr("7"), CharPtr("8"))));
}


TEST(InvokeTest, FunctionThatTakes9Arguments) {
  Action<string(const char*, const char*, const char*, const char*,
                const char*, const char*, const char*, const char*,
                const char*)> a = Invoke(Concat9);
  EXPECT_EQ("123456789",
            a.Perform(make_tuple(CharPtr("1"), CharPtr("2"), CharPtr("3"),
                                 CharPtr("4"), CharPtr("5"), CharPtr("6"),
                                 CharPtr("7"), CharPtr("8"), CharPtr("9"))));
}


TEST(InvokeTest, FunctionThatTakes10Arguments) {
  Action<string(const char*, const char*, const char*, const char*,
                const char*, const char*, const char*, const char*,
                const char*, const char*)> a = Invoke(Concat10);
  EXPECT_EQ("1234567890",
            a.Perform(make_tuple(CharPtr("1"), CharPtr("2"), CharPtr("3"),
                                 CharPtr("4"), CharPtr("5"), CharPtr("6"),
                                 CharPtr("7"), CharPtr("8"), CharPtr("9"),
                                 CharPtr("0"))));
}


TEST(InvokeTest, FunctionWithUnusedParameters) {
  Action<int(int, int, double, const string&)> a1 =
      Invoke(SumOfFirst2);
  EXPECT_EQ(12, a1.Perform(make_tuple(10, 2, 5.6, CharPtr("hi"))));

  Action<int(int, int, bool, int*)> a2 =
      Invoke(SumOfFirst2);
  EXPECT_EQ(23, a2.Perform(make_tuple(20, 3, true, static_cast<int*>(NULL))));
}


TEST(InvokeTest, MethodWithUnusedParameters) {
  Foo foo;
  Action<int(string, bool, int, int)> a1 =
      Invoke(&foo, &Foo::SumOfLast2);
  EXPECT_EQ(12, a1.Perform(make_tuple(CharPtr("hi"), true, 10, 2)));

  Action<int(char, double, int, int)> a2 =
      Invoke(&foo, &Foo::SumOfLast2);
  EXPECT_EQ(23, a2.Perform(make_tuple('a', 2.5, 20, 3)));
}


TEST(InvokeTest, Functor) {
  Action<long(long, int)> a = Invoke(plus<long>());  
  EXPECT_EQ(3L, a.Perform(make_tuple(1, 2)));
}


TEST(InvokeTest, FunctionWithCompatibleType) {
  Action<long(int, short, char, bool)> a = Invoke(SumOf4);  
  EXPECT_EQ(4321, a.Perform(make_tuple(4000, Short(300), Char(20), true)));
}




TEST(InvokeMethodTest, Nullary) {
  Foo foo;
  Action<int()> a = Invoke(&foo, &Foo::Nullary);  
  EXPECT_EQ(123, a.Perform(make_tuple()));
}


TEST(InvokeMethodTest, Unary) {
  Foo foo;
  Action<short(long)> a = Invoke(&foo, &Foo::Unary);  
  EXPECT_EQ(4123, a.Perform(make_tuple(4000)));
}


TEST(InvokeMethodTest, Binary) {
  Foo foo;
  Action<string(const string&, char)> a = Invoke(&foo, &Foo::Binary);
  string s("Hell");
  EXPECT_EQ("Hello", a.Perform(make_tuple(s, 'o')));
}


TEST(InvokeMethodTest, Ternary) {
  Foo foo;
  Action<int(int, bool, char)> a = Invoke(&foo, &Foo::Ternary);  
  EXPECT_EQ(1124, a.Perform(make_tuple(1000, true, Char(1))));
}


TEST(InvokeMethodTest, MethodThatTakes4Arguments) {
  Foo foo;
  Action<int(int, int, int, int)> a = Invoke(&foo, &Foo::SumOf4);  
  EXPECT_EQ(1357, a.Perform(make_tuple(1000, 200, 30, 4)));
}


TEST(InvokeMethodTest, MethodThatTakes5Arguments) {
  Foo foo;
  Action<int(int, int, int, int, int)> a = Invoke(&foo, &Foo::SumOf5);  
  EXPECT_EQ(12345, a.Perform(make_tuple(10000, 2000, 300, 40, 5)));
}


TEST(InvokeMethodTest, MethodThatTakes6Arguments) {
  Foo foo;
  Action<int(int, int, int, int, int, int)> a =  
      Invoke(&foo, &Foo::SumOf6);
  EXPECT_EQ(123456, a.Perform(make_tuple(100000, 20000, 3000, 400, 50, 6)));
}


TEST(InvokeMethodTest, MethodThatTakes7Arguments) {
  Foo foo;
  Action<string(const char*, const char*, const char*, const char*,
                const char*, const char*, const char*)> a =
      Invoke(&foo, &Foo::Concat7);
  EXPECT_EQ("1234567",
            a.Perform(make_tuple(CharPtr("1"), CharPtr("2"), CharPtr("3"),
                                 CharPtr("4"), CharPtr("5"), CharPtr("6"),
                                 CharPtr("7"))));
}


TEST(InvokeMethodTest, MethodThatTakes8Arguments) {
  Foo foo;
  Action<string(const char*, const char*, const char*, const char*,
                const char*, const char*, const char*, const char*)> a =
      Invoke(&foo, &Foo::Concat8);
  EXPECT_EQ("12345678",
            a.Perform(make_tuple(CharPtr("1"), CharPtr("2"), CharPtr("3"),
                                 CharPtr("4"), CharPtr("5"), CharPtr("6"),
                                 CharPtr("7"), CharPtr("8"))));
}


TEST(InvokeMethodTest, MethodThatTakes9Arguments) {
  Foo foo;
  Action<string(const char*, const char*, const char*, const char*,
                const char*, const char*, const char*, const char*,
                const char*)> a = Invoke(&foo, &Foo::Concat9);
  EXPECT_EQ("123456789",
            a.Perform(make_tuple(CharPtr("1"), CharPtr("2"), CharPtr("3"),
                                 CharPtr("4"), CharPtr("5"), CharPtr("6"),
                                 CharPtr("7"), CharPtr("8"), CharPtr("9"))));
}


TEST(InvokeMethodTest, MethodThatTakes10Arguments) {
  Foo foo;
  Action<string(const char*, const char*, const char*, const char*,
                const char*, const char*, const char*, const char*,
                const char*, const char*)> a = Invoke(&foo, &Foo::Concat10);
  EXPECT_EQ("1234567890",
            a.Perform(make_tuple(CharPtr("1"), CharPtr("2"), CharPtr("3"),
                                 CharPtr("4"), CharPtr("5"), CharPtr("6"),
                                 CharPtr("7"), CharPtr("8"), CharPtr("9"),
                                 CharPtr("0"))));
}


TEST(InvokeMethodTest, MethodWithCompatibleType) {
  Foo foo;
  Action<long(int, short, char, bool)> a =  
      Invoke(&foo, &Foo::SumOf4);
  EXPECT_EQ(4444, a.Perform(make_tuple(4000, Short(300), Char(20), true)));
}


TEST(WithoutArgsTest, NoArg) {
  Action<int(int n)> a = WithoutArgs(Invoke(Nullary));  
  EXPECT_EQ(1, a.Perform(make_tuple(2)));
}


TEST(WithArgTest, OneArg) {
  Action<bool(double x, int n)> b = WithArg<1>(Invoke(Unary));  
  EXPECT_TRUE(b.Perform(make_tuple(1.5, -1)));
  EXPECT_FALSE(b.Perform(make_tuple(1.5, 1)));
}

TEST(ReturnArgActionTest, WorksForOneArgIntArg0) {
  const Action<int(int)> a = ReturnArg<0>();
  EXPECT_EQ(5, a.Perform(make_tuple(5)));
}

TEST(ReturnArgActionTest, WorksForMultiArgBoolArg0) {
  const Action<bool(bool, bool, bool)> a = ReturnArg<0>();
  EXPECT_TRUE(a.Perform(make_tuple(true, false, false)));
}

TEST(ReturnArgActionTest, WorksForMultiArgStringArg2) {
  const Action<string(int, int, string, int)> a = ReturnArg<2>();
  EXPECT_EQ("seven", a.Perform(make_tuple(5, 6, string("seven"), 8)));
}

TEST(SaveArgActionTest, WorksForSameType) {
  int result = 0;
  const Action<void(int n)> a1 = SaveArg<0>(&result);
  a1.Perform(make_tuple(5));
  EXPECT_EQ(5, result);
}

TEST(SaveArgActionTest, WorksForCompatibleType) {
  int result = 0;
  const Action<void(bool, char)> a1 = SaveArg<1>(&result);
  a1.Perform(make_tuple(true, 'a'));
  EXPECT_EQ('a', result);
}

TEST(SaveArgPointeeActionTest, WorksForSameType) {
  int result = 0;
  const int value = 5;
  const Action<void(const int*)> a1 = SaveArgPointee<0>(&result);
  a1.Perform(make_tuple(&value));
  EXPECT_EQ(5, result);
}

TEST(SaveArgPointeeActionTest, WorksForCompatibleType) {
  int result = 0;
  char value = 'a';
  const Action<void(bool, char*)> a1 = SaveArgPointee<1>(&result);
  a1.Perform(make_tuple(true, &value));
  EXPECT_EQ('a', result);
}

TEST(SaveArgPointeeActionTest, WorksForLinkedPtr) {
  int result = 0;
  linked_ptr<int> value(new int(5));
  const Action<void(linked_ptr<int>)> a1 = SaveArgPointee<0>(&result);
  a1.Perform(make_tuple(value));
  EXPECT_EQ(5, result);
}

TEST(SetArgRefereeActionTest, WorksForSameType) {
  int value = 0;
  const Action<void(int&)> a1 = SetArgReferee<0>(1);
  a1.Perform(tuple<int&>(value));
  EXPECT_EQ(1, value);
}

TEST(SetArgRefereeActionTest, WorksForCompatibleType) {
  int value = 0;
  const Action<void(int, int&)> a1 = SetArgReferee<1>('a');
  a1.Perform(tuple<int, int&>(0, value));
  EXPECT_EQ('a', value);
}

TEST(SetArgRefereeActionTest, WorksWithExtraArguments) {
  int value = 0;
  const Action<void(bool, int, int&, const char*)> a1 = SetArgReferee<2>('a');
  a1.Perform(tuple<bool, int, int&, const char*>(true, 0, value, "hi"));
  EXPECT_EQ('a', value);
}



class DeletionTester {
 public:
  explicit DeletionTester(bool* is_deleted)
    : is_deleted_(is_deleted) {
    
    *is_deleted_ = false;
  }

  ~DeletionTester() {
    *is_deleted_ = true;
  }

 private:
  bool* is_deleted_;
};

TEST(DeleteArgActionTest, OneArg) {
  bool is_deleted = false;
  DeletionTester* t = new DeletionTester(&is_deleted);
  const Action<void(DeletionTester*)> a1 = DeleteArg<0>();      
  EXPECT_FALSE(is_deleted);
  a1.Perform(make_tuple(t));
  EXPECT_TRUE(is_deleted);
}

TEST(DeleteArgActionTest, TenArgs) {
  bool is_deleted = false;
  DeletionTester* t = new DeletionTester(&is_deleted);
  const Action<void(bool, int, int, const char*, bool,
                    int, int, int, int, DeletionTester*)> a1 = DeleteArg<9>();
  EXPECT_FALSE(is_deleted);
  a1.Perform(make_tuple(true, 5, 6, CharPtr("hi"), false, 7, 8, 9, 10, t));
  EXPECT_TRUE(is_deleted);
}

#if GTEST_HAS_EXCEPTIONS

TEST(ThrowActionTest, ThrowsGivenExceptionInVoidFunction) {
  const Action<void(int n)> a = Throw('a');
  EXPECT_THROW(a.Perform(make_tuple(0)), char);
}

class MyException {};

TEST(ThrowActionTest, ThrowsGivenExceptionInNonVoidFunction) {
  const Action<double(char ch)> a = Throw(MyException());
  EXPECT_THROW(a.Perform(make_tuple('0')), MyException);
}

TEST(ThrowActionTest, ThrowsGivenExceptionInNullaryFunction) {
  const Action<double()> a = Throw(MyException());
  EXPECT_THROW(a.Perform(make_tuple()), MyException);
}

#endif  



TEST(SetArrayArgumentTest, SetsTheNthArray) {
  typedef void MyFunction(bool, int*, char*);
  int numbers[] = { 1, 2, 3 };
  Action<MyFunction> a = SetArrayArgument<1>(numbers, numbers + 3);

  int n[4] = {};
  int* pn = n;
  char ch[4] = {};
  char* pch = ch;
  a.Perform(make_tuple(true, pn, pch));
  EXPECT_EQ(1, n[0]);
  EXPECT_EQ(2, n[1]);
  EXPECT_EQ(3, n[2]);
  EXPECT_EQ(0, n[3]);
  EXPECT_EQ('\0', ch[0]);
  EXPECT_EQ('\0', ch[1]);
  EXPECT_EQ('\0', ch[2]);
  EXPECT_EQ('\0', ch[3]);

  
  std::string letters = "abc";
  a = SetArrayArgument<2>(letters.begin(), letters.end());
  std::fill_n(n, 4, 0);
  std::fill_n(ch, 4, '\0');
  a.Perform(make_tuple(true, pn, pch));
  EXPECT_EQ(0, n[0]);
  EXPECT_EQ(0, n[1]);
  EXPECT_EQ(0, n[2]);
  EXPECT_EQ(0, n[3]);
  EXPECT_EQ('a', ch[0]);
  EXPECT_EQ('b', ch[1]);
  EXPECT_EQ('c', ch[2]);
  EXPECT_EQ('\0', ch[3]);
}


TEST(SetArrayArgumentTest, SetsTheNthArrayWithEmptyRange) {
  typedef void MyFunction(bool, int*);
  int numbers[] = { 1, 2, 3 };
  Action<MyFunction> a = SetArrayArgument<1>(numbers, numbers);

  int n[4] = {};
  int* pn = n;
  a.Perform(make_tuple(true, pn));
  EXPECT_EQ(0, n[0]);
  EXPECT_EQ(0, n[1]);
  EXPECT_EQ(0, n[2]);
  EXPECT_EQ(0, n[3]);
}



TEST(SetArrayArgumentTest, SetsTheNthArrayWithConvertibleType) {
  typedef void MyFunction(bool, char*);
  int codes[] = { 97, 98, 99 };
  Action<MyFunction> a = SetArrayArgument<1>(codes, codes + 3);

  char ch[4] = {};
  char* pch = ch;
  a.Perform(make_tuple(true, pch));
  EXPECT_EQ('a', ch[0]);
  EXPECT_EQ('b', ch[1]);
  EXPECT_EQ('c', ch[2]);
  EXPECT_EQ('\0', ch[3]);
}


TEST(SetArrayArgumentTest, SetsTheNthArrayWithIteratorArgument) {
  typedef void MyFunction(bool, std::back_insert_iterator<std::string>);
  std::string letters = "abc";
  Action<MyFunction> a = SetArrayArgument<1>(letters.begin(), letters.end());

  std::string s;
  a.Perform(make_tuple(true, back_inserter(s)));
  EXPECT_EQ(letters, s);
}

TEST(ReturnPointeeTest, Works) {
  int n = 42;
  const Action<int()> a = ReturnPointee(&n);
  EXPECT_EQ(42, a.Perform(make_tuple()));

  n = 43;
  EXPECT_EQ(43, a.Perform(make_tuple()));
}

}  
}  

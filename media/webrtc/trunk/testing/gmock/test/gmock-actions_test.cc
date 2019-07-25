


































#include "gmock/gmock-actions.h"
#include <algorithm>
#include <iterator>
#include <string>
#include "gmock/gmock.h"
#include "gmock/internal/gmock-port.h"
#include "gtest/gtest.h"
#include "gtest/gtest-spi.h"

namespace {

using ::std::tr1::get;
using ::std::tr1::make_tuple;
using ::std::tr1::tuple;
using ::std::tr1::tuple_element;
using testing::internal::BuiltInDefaultValue;
using testing::internal::Int64;
using testing::internal::UInt64;

using testing::_;
using testing::Action;
using testing::ActionInterface;
using testing::Assign;
using testing::ByRef;
using testing::DefaultValue;
using testing::DoDefault;
using testing::IgnoreResult;
using testing::Invoke;
using testing::InvokeWithoutArgs;
using testing::MakePolymorphicAction;
using testing::Ne;
using testing::PolymorphicAction;
using testing::Return;
using testing::ReturnNull;
using testing::ReturnRef;
using testing::ReturnRefOfCopy;
using testing::SetArgPointee;
using testing::SetArgumentPointee;

#if !GTEST_OS_WINDOWS_MOBILE
using testing::SetErrnoAndReturn;
#endif

#if GTEST_HAS_PROTOBUF_
using testing::internal::TestMessage;
#endif  


TEST(BuiltInDefaultValueTest, IsNullForPointerTypes) {
  EXPECT_TRUE(BuiltInDefaultValue<int*>::Get() == NULL);
  EXPECT_TRUE(BuiltInDefaultValue<const char*>::Get() == NULL);
  EXPECT_TRUE(BuiltInDefaultValue<void*>::Get() == NULL);
}


TEST(BuiltInDefaultValueTest, ExistsForPointerTypes) {
  EXPECT_TRUE(BuiltInDefaultValue<int*>::Exists());
  EXPECT_TRUE(BuiltInDefaultValue<const char*>::Exists());
  EXPECT_TRUE(BuiltInDefaultValue<void*>::Exists());
}



TEST(BuiltInDefaultValueTest, IsZeroForNumericTypes) {
  EXPECT_EQ(0U, BuiltInDefaultValue<unsigned char>::Get());
  EXPECT_EQ(0, BuiltInDefaultValue<signed char>::Get());
  EXPECT_EQ(0, BuiltInDefaultValue<char>::Get());
#if GMOCK_HAS_SIGNED_WCHAR_T_
  EXPECT_EQ(0U, BuiltInDefaultValue<unsigned wchar_t>::Get());
  EXPECT_EQ(0, BuiltInDefaultValue<signed wchar_t>::Get());
#endif
#if GMOCK_WCHAR_T_IS_NATIVE_
  EXPECT_EQ(0, BuiltInDefaultValue<wchar_t>::Get());
#endif
  EXPECT_EQ(0U, BuiltInDefaultValue<unsigned short>::Get());  
  EXPECT_EQ(0, BuiltInDefaultValue<signed short>::Get());  
  EXPECT_EQ(0, BuiltInDefaultValue<short>::Get());  
  EXPECT_EQ(0U, BuiltInDefaultValue<unsigned int>::Get());
  EXPECT_EQ(0, BuiltInDefaultValue<signed int>::Get());
  EXPECT_EQ(0, BuiltInDefaultValue<int>::Get());
  EXPECT_EQ(0U, BuiltInDefaultValue<unsigned long>::Get());  
  EXPECT_EQ(0, BuiltInDefaultValue<signed long>::Get());  
  EXPECT_EQ(0, BuiltInDefaultValue<long>::Get());  
  EXPECT_EQ(0U, BuiltInDefaultValue<UInt64>::Get());
  EXPECT_EQ(0, BuiltInDefaultValue<Int64>::Get());
  EXPECT_EQ(0, BuiltInDefaultValue<float>::Get());
  EXPECT_EQ(0, BuiltInDefaultValue<double>::Get());
}



TEST(BuiltInDefaultValueTest, ExistsForNumericTypes) {
  EXPECT_TRUE(BuiltInDefaultValue<unsigned char>::Exists());
  EXPECT_TRUE(BuiltInDefaultValue<signed char>::Exists());
  EXPECT_TRUE(BuiltInDefaultValue<char>::Exists());
#if GMOCK_HAS_SIGNED_WCHAR_T_
  EXPECT_TRUE(BuiltInDefaultValue<unsigned wchar_t>::Exists());
  EXPECT_TRUE(BuiltInDefaultValue<signed wchar_t>::Exists());
#endif
#if GMOCK_WCHAR_T_IS_NATIVE_
  EXPECT_TRUE(BuiltInDefaultValue<wchar_t>::Exists());
#endif
  EXPECT_TRUE(BuiltInDefaultValue<unsigned short>::Exists());  
  EXPECT_TRUE(BuiltInDefaultValue<signed short>::Exists());  
  EXPECT_TRUE(BuiltInDefaultValue<short>::Exists());  
  EXPECT_TRUE(BuiltInDefaultValue<unsigned int>::Exists());
  EXPECT_TRUE(BuiltInDefaultValue<signed int>::Exists());
  EXPECT_TRUE(BuiltInDefaultValue<int>::Exists());
  EXPECT_TRUE(BuiltInDefaultValue<unsigned long>::Exists());  
  EXPECT_TRUE(BuiltInDefaultValue<signed long>::Exists());  
  EXPECT_TRUE(BuiltInDefaultValue<long>::Exists());  
  EXPECT_TRUE(BuiltInDefaultValue<UInt64>::Exists());
  EXPECT_TRUE(BuiltInDefaultValue<Int64>::Exists());
  EXPECT_TRUE(BuiltInDefaultValue<float>::Exists());
  EXPECT_TRUE(BuiltInDefaultValue<double>::Exists());
}


TEST(BuiltInDefaultValueTest, IsFalseForBool) {
  EXPECT_FALSE(BuiltInDefaultValue<bool>::Get());
}


TEST(BuiltInDefaultValueTest, BoolExists) {
  EXPECT_TRUE(BuiltInDefaultValue<bool>::Exists());
}



TEST(BuiltInDefaultValueTest, IsEmptyStringForString) {
#if GTEST_HAS_GLOBAL_STRING
  EXPECT_EQ("", BuiltInDefaultValue< ::string>::Get());
#endif  

  EXPECT_EQ("", BuiltInDefaultValue< ::std::string>::Get());
}



TEST(BuiltInDefaultValueTest, ExistsForString) {
#if GTEST_HAS_GLOBAL_STRING
  EXPECT_TRUE(BuiltInDefaultValue< ::string>::Exists());
#endif  

  EXPECT_TRUE(BuiltInDefaultValue< ::std::string>::Exists());
}



TEST(BuiltInDefaultValueTest, WorksForConstTypes) {
  EXPECT_EQ("", BuiltInDefaultValue<const std::string>::Get());
  EXPECT_EQ(0, BuiltInDefaultValue<const int>::Get());
  EXPECT_TRUE(BuiltInDefaultValue<char* const>::Get() == NULL);
  EXPECT_FALSE(BuiltInDefaultValue<const bool>::Get());
}



struct UserType {
  UserType() : value(0) {}

  int value;
};

TEST(BuiltInDefaultValueTest, UserTypeHasNoDefault) {
  EXPECT_FALSE(BuiltInDefaultValue<UserType>::Exists());
}


TEST(BuiltInDefaultValueDeathTest, IsUndefinedForReferences) {
  EXPECT_DEATH_IF_SUPPORTED({
    BuiltInDefaultValue<int&>::Get();
  }, "");
  EXPECT_DEATH_IF_SUPPORTED({
    BuiltInDefaultValue<const char&>::Get();
  }, "");
}

TEST(BuiltInDefaultValueDeathTest, IsUndefinedForUserTypes) {
  EXPECT_DEATH_IF_SUPPORTED({
    BuiltInDefaultValue<UserType>::Get();
  }, "");
}


TEST(DefaultValueTest, IsInitiallyUnset) {
  EXPECT_FALSE(DefaultValue<int>::IsSet());
  EXPECT_FALSE(DefaultValue<const UserType>::IsSet());
}


TEST(DefaultValueTest, CanBeSetAndUnset) {
  EXPECT_TRUE(DefaultValue<int>::Exists());
  EXPECT_FALSE(DefaultValue<const UserType>::Exists());

  DefaultValue<int>::Set(1);
  DefaultValue<const UserType>::Set(UserType());

  EXPECT_EQ(1, DefaultValue<int>::Get());
  EXPECT_EQ(0, DefaultValue<const UserType>::Get().value);

  EXPECT_TRUE(DefaultValue<int>::Exists());
  EXPECT_TRUE(DefaultValue<const UserType>::Exists());

  DefaultValue<int>::Clear();
  DefaultValue<const UserType>::Clear();

  EXPECT_FALSE(DefaultValue<int>::IsSet());
  EXPECT_FALSE(DefaultValue<const UserType>::IsSet());

  EXPECT_TRUE(DefaultValue<int>::Exists());
  EXPECT_FALSE(DefaultValue<const UserType>::Exists());
}




TEST(DefaultValueDeathTest, GetReturnsBuiltInDefaultValueWhenUnset) {
  EXPECT_FALSE(DefaultValue<int>::IsSet());
  EXPECT_TRUE(DefaultValue<int>::Exists());
  EXPECT_FALSE(DefaultValue<UserType>::IsSet());
  EXPECT_FALSE(DefaultValue<UserType>::Exists());

  EXPECT_EQ(0, DefaultValue<int>::Get());

  EXPECT_DEATH_IF_SUPPORTED({
    DefaultValue<UserType>::Get();
  }, "");
}


TEST(DefaultValueTest, GetWorksForVoid) {
  return DefaultValue<void>::Get();
}




TEST(DefaultValueOfReferenceTest, IsInitiallyUnset) {
  EXPECT_FALSE(DefaultValue<int&>::IsSet());
  EXPECT_FALSE(DefaultValue<UserType&>::IsSet());
}


TEST(DefaultValueOfReferenceTest, IsInitiallyNotExisting) {
  EXPECT_FALSE(DefaultValue<int&>::Exists());
  EXPECT_FALSE(DefaultValue<UserType&>::Exists());
}


TEST(DefaultValueOfReferenceTest, CanBeSetAndUnset) {
  int n = 1;
  DefaultValue<const int&>::Set(n);
  UserType u;
  DefaultValue<UserType&>::Set(u);

  EXPECT_TRUE(DefaultValue<const int&>::Exists());
  EXPECT_TRUE(DefaultValue<UserType&>::Exists());

  EXPECT_EQ(&n, &(DefaultValue<const int&>::Get()));
  EXPECT_EQ(&u, &(DefaultValue<UserType&>::Get()));

  DefaultValue<const int&>::Clear();
  DefaultValue<UserType&>::Clear();

  EXPECT_FALSE(DefaultValue<const int&>::Exists());
  EXPECT_FALSE(DefaultValue<UserType&>::Exists());

  EXPECT_FALSE(DefaultValue<const int&>::IsSet());
  EXPECT_FALSE(DefaultValue<UserType&>::IsSet());
}




TEST(DefaultValueOfReferenceDeathTest, GetReturnsBuiltInDefaultValueWhenUnset) {
  EXPECT_FALSE(DefaultValue<int&>::IsSet());
  EXPECT_FALSE(DefaultValue<UserType&>::IsSet());

  EXPECT_DEATH_IF_SUPPORTED({
    DefaultValue<int&>::Get();
  }, "");
  EXPECT_DEATH_IF_SUPPORTED({
    DefaultValue<UserType>::Get();
  }, "");
}




typedef int MyFunction(bool, int);

class MyActionImpl : public ActionInterface<MyFunction> {
 public:
  virtual int Perform(const tuple<bool, int>& args) {
    return get<0>(args) ? get<1>(args) : 0;
  }
};

TEST(ActionInterfaceTest, CanBeImplementedByDefiningPerform) {
  MyActionImpl my_action_impl;
  (void)my_action_impl;
}

TEST(ActionInterfaceTest, MakeAction) {
  Action<MyFunction> action = MakeAction(new MyActionImpl);

  
  
  
  
  
  EXPECT_EQ(5, action.Perform(make_tuple(true, 5)));
}



TEST(ActionTest, CanBeConstructedFromActionInterface) {
  Action<MyFunction> action(new MyActionImpl);
}


TEST(ActionTest, DelegatesWorkToActionInterface) {
  const Action<MyFunction> action(new MyActionImpl);

  EXPECT_EQ(5, action.Perform(make_tuple(true, 5)));
  EXPECT_EQ(0, action.Perform(make_tuple(false, 1)));
}


TEST(ActionTest, IsCopyable) {
  Action<MyFunction> a1(new MyActionImpl);
  Action<MyFunction> a2(a1);  

  
  EXPECT_EQ(5, a1.Perform(make_tuple(true, 5)));
  EXPECT_EQ(0, a1.Perform(make_tuple(false, 1)));

  
  EXPECT_EQ(5, a2.Perform(make_tuple(true, 5)));
  EXPECT_EQ(0, a2.Perform(make_tuple(false, 1)));

  a2 = a1;  

  
  EXPECT_EQ(5, a1.Perform(make_tuple(true, 5)));
  EXPECT_EQ(0, a1.Perform(make_tuple(false, 1)));

  
  EXPECT_EQ(5, a2.Perform(make_tuple(true, 5)));
  EXPECT_EQ(0, a2.Perform(make_tuple(false, 1)));
}




class IsNotZero : public ActionInterface<bool(int)> {  
 public:
  virtual bool Perform(const tuple<int>& arg) {
    return get<0>(arg) != 0;
  }
};

#if !GTEST_OS_SYMBIAN





TEST(ActionTest, CanBeConvertedToOtherActionType) {
  const Action<bool(int)> a1(new IsNotZero);  
  const Action<int(char)> a2 = Action<int(char)>(a1);  
  EXPECT_EQ(1, a2.Perform(make_tuple('a')));
  EXPECT_EQ(0, a2.Perform(make_tuple('\0')));
}
#endif  





class ReturnSecondArgumentAction {
 public:
  
  
  
  template <typename Result, typename ArgumentTuple>
  Result Perform(const ArgumentTuple& args) { return get<1>(args); }
};



class ReturnZeroFromNullaryFunctionAction {
 public:
  
  
  
  
  
  
  
  template <typename Result>
  Result Perform(const tuple<>&) const { return 0; }
};




PolymorphicAction<ReturnSecondArgumentAction> ReturnSecondArgument() {
  return MakePolymorphicAction(ReturnSecondArgumentAction());
}

PolymorphicAction<ReturnZeroFromNullaryFunctionAction>
ReturnZeroFromNullaryFunction() {
  return MakePolymorphicAction(ReturnZeroFromNullaryFunctionAction());
}



TEST(MakePolymorphicActionTest, ConstructsActionFromImpl) {
  Action<int(bool, int, double)> a1 = ReturnSecondArgument();  
  EXPECT_EQ(5, a1.Perform(make_tuple(false, 5, 2.0)));
}



TEST(MakePolymorphicActionTest, WorksWhenPerformHasOneTemplateParameter) {
  Action<int()> a1 = ReturnZeroFromNullaryFunction();
  EXPECT_EQ(0, a1.Perform(make_tuple()));

  Action<void*()> a2 = ReturnZeroFromNullaryFunction();
  EXPECT_TRUE(a2.Perform(make_tuple()) == NULL);
}



TEST(ReturnTest, WorksForVoid) {
  const Action<void(int)> ret = Return();  
  return ret.Perform(make_tuple(1));
}


TEST(ReturnTest, ReturnsGivenValue) {
  Action<int()> ret = Return(1);  
  EXPECT_EQ(1, ret.Perform(make_tuple()));

  ret = Return(-5);
  EXPECT_EQ(-5, ret.Perform(make_tuple()));
}


TEST(ReturnTest, AcceptsStringLiteral) {
  Action<const char*()> a1 = Return("Hello");
  EXPECT_STREQ("Hello", a1.Perform(make_tuple()));

  Action<std::string()> a2 = Return("world");
  EXPECT_EQ("world", a2.Perform(make_tuple()));
}



struct Base {
  bool operator==(const Base&) { return true; }
};

struct Derived : public Base {
  bool operator==(const Derived&) { return true; }
};

TEST(ReturnTest, IsCovariant) {
  Base base;
  Derived derived;
  Action<Base*()> ret = Return(&base);
  EXPECT_EQ(&base, ret.Perform(make_tuple()));

  ret = Return(&derived);
  EXPECT_EQ(&derived, ret.Perform(make_tuple()));
}





class FromType {
 public:
  FromType(bool* is_converted) : converted_(is_converted) {}
  bool* converted() const { return converted_; }

 private:
  bool* const converted_;

  GTEST_DISALLOW_ASSIGN_(FromType);
};

class ToType {
 public:
  ToType(const FromType& x) { *x.converted() = true; }
};

TEST(ReturnTest, ConvertsArgumentWhenConverted) {
  bool converted = false;
  FromType x(&converted);
  Action<ToType()> action(Return(x));
  EXPECT_TRUE(converted) << "Return must convert its argument in its own "
                         << "conversion operator.";
  converted = false;
  action.Perform(tuple<>());
  EXPECT_FALSE(converted) << "Action must NOT convert its argument "
                          << "when performed." ;
}

class DestinationType {};

class SourceType {
 public:
  
  operator DestinationType() { return DestinationType(); }
};

TEST(ReturnTest, CanConvertArgumentUsingNonConstTypeCastOperator) {
  SourceType s;
  Action<DestinationType()> action(Return(s));
}


TEST(ReturnNullTest, WorksInPointerReturningFunction) {
  const Action<int*()> a1 = ReturnNull();
  EXPECT_TRUE(a1.Perform(make_tuple()) == NULL);

  const Action<const char*(bool)> a2 = ReturnNull();  
  EXPECT_TRUE(a2.Perform(make_tuple(true)) == NULL);
}


TEST(ReturnRefTest, WorksForReference) {
  const int n = 0;
  const Action<const int&(bool)> ret = ReturnRef(n);  

  EXPECT_EQ(&n, &ret.Perform(make_tuple(true)));
}


TEST(ReturnRefTest, IsCovariant) {
  Base base;
  Derived derived;
  Action<Base&()> a = ReturnRef(base);
  EXPECT_EQ(&base, &a.Perform(make_tuple()));

  a = ReturnRef(derived);
  EXPECT_EQ(&derived, &a.Perform(make_tuple()));
}


TEST(ReturnRefOfCopyTest, WorksForReference) {
  int n = 42;
  const Action<const int&()> ret = ReturnRefOfCopy(n);

  EXPECT_NE(&n, &ret.Perform(make_tuple()));
  EXPECT_EQ(42, ret.Perform(make_tuple()));

  n = 43;
  EXPECT_NE(&n, &ret.Perform(make_tuple()));
  EXPECT_EQ(42, ret.Perform(make_tuple()));
}


TEST(ReturnRefOfCopyTest, IsCovariant) {
  Base base;
  Derived derived;
  Action<Base&()> a = ReturnRefOfCopy(base);
  EXPECT_NE(&base, &a.Perform(make_tuple()));

  a = ReturnRefOfCopy(derived);
  EXPECT_NE(&derived, &a.Perform(make_tuple()));
}



class MyClass {};

class MockClass {
 public:
  MockClass() {}

  MOCK_METHOD1(IntFunc, int(bool flag));  
  MOCK_METHOD0(Foo, MyClass());

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(MockClass);
};



TEST(DoDefaultTest, ReturnsBuiltInDefaultValueByDefault) {
  MockClass mock;
  EXPECT_CALL(mock, IntFunc(_))
      .WillOnce(DoDefault());
  EXPECT_EQ(0, mock.IntFunc(true));
}



TEST(DoDefaultDeathTest, DiesForUnknowType) {
  MockClass mock;
  EXPECT_CALL(mock, Foo())
      .WillRepeatedly(DoDefault());
  EXPECT_DEATH_IF_SUPPORTED({
    mock.Foo();
  }, "");
}




void VoidFunc(bool ) {}

TEST(DoDefaultDeathTest, DiesIfUsedInCompositeAction) {
  MockClass mock;
  EXPECT_CALL(mock, IntFunc(_))
      .WillRepeatedly(DoAll(Invoke(VoidFunc),
                            DoDefault()));

  
  
  
  
  EXPECT_DEATH_IF_SUPPORTED({
    mock.IntFunc(true);
  }, "");
}



TEST(DoDefaultTest, ReturnsUserSpecifiedPerTypeDefaultValueWhenThereIsOne) {
  DefaultValue<int>::Set(1);
  MockClass mock;
  EXPECT_CALL(mock, IntFunc(_))
      .WillOnce(DoDefault());
  EXPECT_EQ(1, mock.IntFunc(false));
  DefaultValue<int>::Clear();
}


TEST(DoDefaultTest, DoesWhatOnCallSpecifies) {
  MockClass mock;
  ON_CALL(mock, IntFunc(_))
      .WillByDefault(Return(2));
  EXPECT_CALL(mock, IntFunc(_))
      .WillOnce(DoDefault());
  EXPECT_EQ(2, mock.IntFunc(false));
}


TEST(DoDefaultTest, CannotBeUsedInOnCall) {
  MockClass mock;
  EXPECT_NONFATAL_FAILURE({  
    ON_CALL(mock, IntFunc(_))
      .WillByDefault(DoDefault());
  }, "DoDefault() cannot be used in ON_CALL()");
}



TEST(SetArgPointeeTest, SetsTheNthPointee) {
  typedef void MyFunction(bool, int*, char*);
  Action<MyFunction> a = SetArgPointee<1>(2);

  int n = 0;
  char ch = '\0';
  a.Perform(make_tuple(true, &n, &ch));
  EXPECT_EQ(2, n);
  EXPECT_EQ('\0', ch);

  a = SetArgPointee<2>('a');
  n = 0;
  ch = '\0';
  a.Perform(make_tuple(true, &n, &ch));
  EXPECT_EQ(0, n);
  EXPECT_EQ('a', ch);
}

#if !((GTEST_GCC_VER_ && GTEST_GCC_VER_ < 40000) || GTEST_OS_SYMBIAN)


TEST(SetArgPointeeTest, AcceptsStringLiteral) {
  typedef void MyFunction(std::string*, const char**);
  Action<MyFunction> a = SetArgPointee<0>("hi");
  std::string str;
  const char* ptr = NULL;
  a.Perform(make_tuple(&str, &ptr));
  EXPECT_EQ("hi", str);
  EXPECT_TRUE(ptr == NULL);

  a = SetArgPointee<1>("world");
  str = "";
  a.Perform(make_tuple(&str, &ptr));
  EXPECT_EQ("", str);
  EXPECT_STREQ("world", ptr);
}

TEST(SetArgPointeeTest, AcceptsWideStringLiteral) {
  typedef void MyFunction(const wchar_t**);
  Action<MyFunction> a = SetArgPointee<0>(L"world");
  const wchar_t* ptr = NULL;
  a.Perform(make_tuple(&ptr));
  EXPECT_STREQ(L"world", ptr);

# if GTEST_HAS_STD_WSTRING

  typedef void MyStringFunction(std::wstring*);
  Action<MyStringFunction> a2 = SetArgPointee<0>(L"world");
  std::wstring str = L"";
  a2.Perform(make_tuple(&str));
  EXPECT_EQ(L"world", str);

# endif
}
#endif


TEST(SetArgPointeeTest, AcceptsCharPointer) {
  typedef void MyFunction(bool, std::string*, const char**);
  const char* const hi = "hi";
  Action<MyFunction> a = SetArgPointee<1>(hi);
  std::string str;
  const char* ptr = NULL;
  a.Perform(make_tuple(true, &str, &ptr));
  EXPECT_EQ("hi", str);
  EXPECT_TRUE(ptr == NULL);

  char world_array[] = "world";
  char* const world = world_array;
  a = SetArgPointee<2>(world);
  str = "";
  a.Perform(make_tuple(true, &str, &ptr));
  EXPECT_EQ("", str);
  EXPECT_EQ(world, ptr);
}

TEST(SetArgPointeeTest, AcceptsWideCharPointer) {
  typedef void MyFunction(bool, const wchar_t**);
  const wchar_t* const hi = L"hi";
  Action<MyFunction> a = SetArgPointee<1>(hi);
  const wchar_t* ptr = NULL;
  a.Perform(make_tuple(true, &ptr));
  EXPECT_EQ(hi, ptr);

# if GTEST_HAS_STD_WSTRING

  typedef void MyStringFunction(bool, std::wstring*);
  wchar_t world_array[] = L"world";
  wchar_t* const world = world_array;
  Action<MyStringFunction> a2 = SetArgPointee<1>(world);
  std::wstring str;
  a2.Perform(make_tuple(true, &str));
  EXPECT_EQ(world_array, str);
# endif
}

#if GTEST_HAS_PROTOBUF_



TEST(SetArgPointeeTest, SetsTheNthPointeeOfProtoBufferType) {
  TestMessage* const msg = new TestMessage;
  msg->set_member("yes");
  TestMessage orig_msg;
  orig_msg.CopyFrom(*msg);

  Action<void(bool, TestMessage*)> a = SetArgPointee<1>(*msg);
  
  
  
  
  delete msg;

  TestMessage dest;
  EXPECT_FALSE(orig_msg.Equals(dest));
  a.Perform(make_tuple(true, &dest));
  EXPECT_TRUE(orig_msg.Equals(dest));
}




TEST(SetArgPointeeTest, SetsTheNthPointeeOfProtoBufferBaseType) {
  TestMessage* const msg = new TestMessage;
  msg->set_member("yes");
  TestMessage orig_msg;
  orig_msg.CopyFrom(*msg);

  Action<void(bool, ::ProtocolMessage*)> a = SetArgPointee<1>(*msg);
  
  
  
  
  delete msg;

  TestMessage dest;
  ::ProtocolMessage* const dest_base = &dest;
  EXPECT_FALSE(orig_msg.Equals(dest));
  a.Perform(make_tuple(true, dest_base));
  EXPECT_TRUE(orig_msg.Equals(dest));
}




TEST(SetArgPointeeTest, SetsTheNthPointeeOfProto2BufferType) {
  using testing::internal::FooMessage;
  FooMessage* const msg = new FooMessage;
  msg->set_int_field(2);
  msg->set_string_field("hi");
  FooMessage orig_msg;
  orig_msg.CopyFrom(*msg);

  Action<void(bool, FooMessage*)> a = SetArgPointee<1>(*msg);
  
  
  
  
  delete msg;

  FooMessage dest;
  dest.set_int_field(0);
  a.Perform(make_tuple(true, &dest));
  EXPECT_EQ(2, dest.int_field());
  EXPECT_EQ("hi", dest.string_field());
}




TEST(SetArgPointeeTest, SetsTheNthPointeeOfProto2BufferBaseType) {
  using testing::internal::FooMessage;
  FooMessage* const msg = new FooMessage;
  msg->set_int_field(2);
  msg->set_string_field("hi");
  FooMessage orig_msg;
  orig_msg.CopyFrom(*msg);

  Action<void(bool, ::proto2::Message*)> a = SetArgPointee<1>(*msg);
  
  
  
  
  delete msg;

  FooMessage dest;
  dest.set_int_field(0);
  ::proto2::Message* const dest_base = &dest;
  a.Perform(make_tuple(true, dest_base));
  EXPECT_EQ(2, dest.int_field());
  EXPECT_EQ("hi", dest.string_field());
}

#endif  



TEST(SetArgumentPointeeTest, SetsTheNthPointee) {
  typedef void MyFunction(bool, int*, char*);
  Action<MyFunction> a = SetArgumentPointee<1>(2);

  int n = 0;
  char ch = '\0';
  a.Perform(make_tuple(true, &n, &ch));
  EXPECT_EQ(2, n);
  EXPECT_EQ('\0', ch);

  a = SetArgumentPointee<2>('a');
  n = 0;
  ch = '\0';
  a.Perform(make_tuple(true, &n, &ch));
  EXPECT_EQ(0, n);
  EXPECT_EQ('a', ch);
}

#if GTEST_HAS_PROTOBUF_



TEST(SetArgumentPointeeTest, SetsTheNthPointeeOfProtoBufferType) {
  TestMessage* const msg = new TestMessage;
  msg->set_member("yes");
  TestMessage orig_msg;
  orig_msg.CopyFrom(*msg);

  Action<void(bool, TestMessage*)> a = SetArgumentPointee<1>(*msg);
  
  
  
  
  delete msg;

  TestMessage dest;
  EXPECT_FALSE(orig_msg.Equals(dest));
  a.Perform(make_tuple(true, &dest));
  EXPECT_TRUE(orig_msg.Equals(dest));
}




TEST(SetArgumentPointeeTest, SetsTheNthPointeeOfProtoBufferBaseType) {
  TestMessage* const msg = new TestMessage;
  msg->set_member("yes");
  TestMessage orig_msg;
  orig_msg.CopyFrom(*msg);

  Action<void(bool, ::ProtocolMessage*)> a = SetArgumentPointee<1>(*msg);
  
  
  
  
  delete msg;

  TestMessage dest;
  ::ProtocolMessage* const dest_base = &dest;
  EXPECT_FALSE(orig_msg.Equals(dest));
  a.Perform(make_tuple(true, dest_base));
  EXPECT_TRUE(orig_msg.Equals(dest));
}




TEST(SetArgumentPointeeTest, SetsTheNthPointeeOfProto2BufferType) {
  using testing::internal::FooMessage;
  FooMessage* const msg = new FooMessage;
  msg->set_int_field(2);
  msg->set_string_field("hi");
  FooMessage orig_msg;
  orig_msg.CopyFrom(*msg);

  Action<void(bool, FooMessage*)> a = SetArgumentPointee<1>(*msg);
  
  
  
  
  delete msg;

  FooMessage dest;
  dest.set_int_field(0);
  a.Perform(make_tuple(true, &dest));
  EXPECT_EQ(2, dest.int_field());
  EXPECT_EQ("hi", dest.string_field());
}




TEST(SetArgumentPointeeTest, SetsTheNthPointeeOfProto2BufferBaseType) {
  using testing::internal::FooMessage;
  FooMessage* const msg = new FooMessage;
  msg->set_int_field(2);
  msg->set_string_field("hi");
  FooMessage orig_msg;
  orig_msg.CopyFrom(*msg);

  Action<void(bool, ::proto2::Message*)> a = SetArgumentPointee<1>(*msg);
  
  
  
  
  delete msg;

  FooMessage dest;
  dest.set_int_field(0);
  ::proto2::Message* const dest_base = &dest;
  a.Perform(make_tuple(true, dest_base));
  EXPECT_EQ(2, dest.int_field());
  EXPECT_EQ("hi", dest.string_field());
}

#endif  


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

bool ByConstRef(const std::string& s) { return s == "Hi"; }

const double g_double = 0;
bool ReferencesGlobalDouble(const double& x) { return &x == &g_double; }

std::string ByNonConstRef(std::string& s) { return s += "+"; }  

struct UnaryFunctor {
  int operator()(bool x) { return x ? 1 : -1; }
};

const char* Binary(const char* input, short n) { return input + n; }  

void VoidBinary(int, char) { g_done = true; }

int Ternary(int x, char y, short z) { return x + y + z; }  

void VoidTernary(int, char, bool) { g_done = true; }

int SumOf4(int a, int b, int c, int d) { return a + b + c + d; }

void VoidFunctionWithFourArguments(char, int, float, double) { g_done = true; }

int SumOf5(int a, int b, int c, int d, int e) { return a + b + c + d + e; }

struct SumOf5Functor {
  int operator()(int a, int b, int c, int d, int e) {
    return a + b + c + d + e;
  }
};

int SumOf6(int a, int b, int c, int d, int e, int f) {
  return a + b + c + d + e + f;
}

struct SumOf6Functor {
  int operator()(int a, int b, int c, int d, int e, int f) {
    return a + b + c + d + e + f;
  }
};

class Foo {
 public:
  Foo() : value_(123) {}

  int Nullary() const { return value_; }
  short Unary(long x) { return static_cast<short>(value_ + x); }  
  std::string Binary(const std::string& str, char c) const { return str + c; }
  int Ternary(int x, bool y, char z) { return value_ + x + y*z; }
  int SumOf4(int a, int b, int c, int d) const {
    return a + b + c + d + value_;
  }
  int SumOf5(int a, int b, int c, int d, int e) { return a + b + c + d + e; }
  int SumOf6(int a, int b, int c, int d, int e, int f) {
    return a + b + c + d + e + f;
  }
 private:
  int value_;
};


TEST(InvokeWithoutArgsTest, Function) {
  
  Action<int(int)> a = InvokeWithoutArgs(Nullary);  
  EXPECT_EQ(1, a.Perform(make_tuple(2)));

  
  Action<int(int, double)> a2 = InvokeWithoutArgs(Nullary);  
  EXPECT_EQ(1, a2.Perform(make_tuple(2, 3.5)));

  
  Action<void(int)> a3 = InvokeWithoutArgs(VoidNullary);  
  g_done = false;
  a3.Perform(make_tuple(1));
  EXPECT_TRUE(g_done);
}


TEST(InvokeWithoutArgsTest, Functor) {
  
  Action<int()> a = InvokeWithoutArgs(NullaryFunctor());  
  EXPECT_EQ(2, a.Perform(make_tuple()));

  
  Action<int(int, double, char)> a2 =  
      InvokeWithoutArgs(NullaryFunctor());
  EXPECT_EQ(2, a2.Perform(make_tuple(3, 3.5, 'a')));

  
  Action<void()> a3 = InvokeWithoutArgs(VoidNullaryFunctor());
  g_done = false;
  a3.Perform(make_tuple());
  EXPECT_TRUE(g_done);
}


TEST(InvokeWithoutArgsTest, Method) {
  Foo foo;
  Action<int(bool, char)> a =  
      InvokeWithoutArgs(&foo, &Foo::Nullary);
  EXPECT_EQ(123, a.Perform(make_tuple(true, 'a')));
}


TEST(IgnoreResultTest, PolymorphicAction) {
  Action<void(int)> a = IgnoreResult(Return(5));  
  a.Perform(make_tuple(1));
}



int ReturnOne() {
  g_done = true;
  return 1;
}

TEST(IgnoreResultTest, MonomorphicAction) {
  g_done = false;
  Action<void()> a = IgnoreResult(Invoke(ReturnOne));
  a.Perform(make_tuple());
  EXPECT_TRUE(g_done);
}



MyClass ReturnMyClass(double ) {
  g_done = true;
  return MyClass();
}

TEST(IgnoreResultTest, ActionReturningClass) {
  g_done = false;
  Action<void(int)> a = IgnoreResult(Invoke(ReturnMyClass));  
  a.Perform(make_tuple(2));
  EXPECT_TRUE(g_done);
}

TEST(AssignTest, Int) {
  int x = 0;
  Action<void(int)> a = Assign(&x, 5);
  a.Perform(make_tuple(0));
  EXPECT_EQ(5, x);
}

TEST(AssignTest, String) {
  ::std::string x;
  Action<void(void)> a = Assign(&x, "Hello, world");
  a.Perform(make_tuple());
  EXPECT_EQ("Hello, world", x);
}

TEST(AssignTest, CompatibleTypes) {
  double x = 0;
  Action<void(int)> a = Assign(&x, 5);
  a.Perform(make_tuple(0));
  EXPECT_DOUBLE_EQ(5, x);
}

#if !GTEST_OS_WINDOWS_MOBILE

class SetErrnoAndReturnTest : public testing::Test {
 protected:
  virtual void SetUp() { errno = 0; }
  virtual void TearDown() { errno = 0; }
};

TEST_F(SetErrnoAndReturnTest, Int) {
  Action<int(void)> a = SetErrnoAndReturn(ENOTTY, -5);
  EXPECT_EQ(-5, a.Perform(make_tuple()));
  EXPECT_EQ(ENOTTY, errno);
}

TEST_F(SetErrnoAndReturnTest, Ptr) {
  int x;
  Action<int*(void)> a = SetErrnoAndReturn(ENOTTY, &x);
  EXPECT_EQ(&x, a.Perform(make_tuple()));
  EXPECT_EQ(ENOTTY, errno);
}

TEST_F(SetErrnoAndReturnTest, CompatibleTypes) {
  Action<double()> a = SetErrnoAndReturn(EINVAL, 5);
  EXPECT_DOUBLE_EQ(5.0, a.Perform(make_tuple()));
  EXPECT_EQ(EINVAL, errno);
}

#endif  




TEST(ByRefTest, IsCopyable) {
  const std::string s1 = "Hi";
  const std::string s2 = "Hello";

  ::testing::internal::ReferenceWrapper<const std::string> ref_wrapper = ByRef(s1);
  const std::string& r1 = ref_wrapper;
  EXPECT_EQ(&s1, &r1);

  
  ref_wrapper = ByRef(s2);
  const std::string& r2 = ref_wrapper;
  EXPECT_EQ(&s2, &r2);

  ::testing::internal::ReferenceWrapper<const std::string> ref_wrapper1 = ByRef(s1);
  
  ref_wrapper = ref_wrapper1;
  const std::string& r3 = ref_wrapper;
  EXPECT_EQ(&s1, &r3);
}


TEST(ByRefTest, ConstValue) {
  const int n = 0;
  
                           
  const int& const_ref = ByRef(n);
  EXPECT_EQ(&n, &const_ref);
}


TEST(ByRefTest, NonConstValue) {
  int n = 0;

  
  int& ref = ByRef(n);
  EXPECT_EQ(&n, &ref);

  
  const int& const_ref = ByRef(n);
  EXPECT_EQ(&n, &const_ref);
}


TEST(ByRefTest, ExplicitType) {
  int n = 0;
  const int& r1 = ByRef<const int>(n);
  EXPECT_EQ(&n, &r1);

  
                      

  Derived d;
  Derived& r2 = ByRef<Derived>(d);
  EXPECT_EQ(&d, &r2);

  const Derived& r3 = ByRef<const Derived>(d);
  EXPECT_EQ(&d, &r3);

  Base& r4 = ByRef<Base>(d);
  EXPECT_EQ(&d, &r4);

  const Base& r5 = ByRef<const Base>(d);
  EXPECT_EQ(&d, &r5);

  
  
  
  
  
}


TEST(ByRefTest, PrintsCorrectly) {
  int n = 42;
  ::std::stringstream expected, actual;
  testing::internal::UniversalPrinter<const int&>::Print(n, &expected);
  testing::internal::UniversalPrint(ByRef(n), &actual);
  EXPECT_EQ(expected.str(), actual.str());
}

}  

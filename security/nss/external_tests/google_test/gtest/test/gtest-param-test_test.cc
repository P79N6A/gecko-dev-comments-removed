


































#include "gtest/gtest.h"

#if GTEST_HAS_PARAM_TEST

# include <algorithm>
# include <iostream>
# include <list>
# include <sstream>
# include <string>
# include <vector>


# define GTEST_IMPLEMENTATION_ 1
# include "src/gtest-internal-inl.h"  
# undef GTEST_IMPLEMENTATION_

# include "test/gtest-param-test_test.h"

using ::std::vector;
using ::std::sort;

using ::testing::AddGlobalTestEnvironment;
using ::testing::Bool;
using ::testing::Message;
using ::testing::Range;
using ::testing::TestWithParam;
using ::testing::Values;
using ::testing::ValuesIn;

# if GTEST_HAS_COMBINE
using ::testing::Combine;
using ::testing::get;
using ::testing::make_tuple;
using ::testing::tuple;
# endif  

using ::testing::internal::ParamGenerator;
using ::testing::internal::UnitTestOptions;







template <typename T>
::std::string PrintValue(const T& value) {
  ::std::stringstream stream;
  stream << value;
  return stream.str();
}

# if GTEST_HAS_COMBINE







template <typename T1, typename T2>
::std::string PrintValue(const tuple<T1, T2>& value) {
  ::std::stringstream stream;
  stream << "(" << get<0>(value) << ", " << get<1>(value) << ")";
  return stream.str();
}

template <typename T1, typename T2, typename T3>
::std::string PrintValue(const tuple<T1, T2, T3>& value) {
  ::std::stringstream stream;
  stream << "(" << get<0>(value) << ", " << get<1>(value)
         << ", "<< get<2>(value) << ")";
  return stream.str();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10>
::std::string PrintValue(
    const tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10>& value) {
  ::std::stringstream stream;
  stream << "(" << get<0>(value) << ", " << get<1>(value)
         << ", "<< get<2>(value) << ", " << get<3>(value)
         << ", "<< get<4>(value) << ", " << get<5>(value)
         << ", "<< get<6>(value) << ", " << get<7>(value)
         << ", "<< get<8>(value) << ", " << get<9>(value) << ")";
  return stream.str();
}

# endif  




template <typename T, size_t N>
void VerifyGenerator(const ParamGenerator<T>& generator,
                     const T (&expected_values)[N]) {
  typename ParamGenerator<T>::iterator it = generator.begin();
  for (size_t i = 0; i < N; ++i) {
    ASSERT_FALSE(it == generator.end())
        << "At element " << i << " when accessing via an iterator "
        << "created with the copy constructor.\n";
    
    
    EXPECT_TRUE(expected_values[i] == *it)
        << "where i is " << i
        << ", expected_values[i] is " << PrintValue(expected_values[i])
        << ", *it is " << PrintValue(*it)
        << ", and 'it' is an iterator created with the copy constructor.\n";
    it++;
  }
  EXPECT_TRUE(it == generator.end())
        << "At the presumed end of sequence when accessing via an iterator "
        << "created with the copy constructor.\n";

  
  
  
  
  it = generator.begin();
  for (size_t i = 0; i < N; ++i) {
    ASSERT_FALSE(it == generator.end())
        << "At element " << i << " when accessing via an iterator "
        << "created with the assignment operator.\n";
    EXPECT_TRUE(expected_values[i] == *it)
        << "where i is " << i
        << ", expected_values[i] is " << PrintValue(expected_values[i])
        << ", *it is " << PrintValue(*it)
        << ", and 'it' is an iterator created with the copy constructor.\n";
    it++;
  }
  EXPECT_TRUE(it == generator.end())
        << "At the presumed end of sequence when accessing via an iterator "
        << "created with the assignment operator.\n";
}

template <typename T>
void VerifyGeneratorIsEmpty(const ParamGenerator<T>& generator) {
  typename ParamGenerator<T>::iterator it = generator.begin();
  EXPECT_TRUE(it == generator.end());

  it = generator.begin();
  EXPECT_TRUE(it == generator.end());
}










TEST(IteratorTest, ParamIteratorConformsToForwardIteratorConcept) {
  const ParamGenerator<int> gen = Range(0, 10);
  ParamGenerator<int>::iterator it = gen.begin();

  
  ParamGenerator<int>::iterator it2 = it;
  EXPECT_TRUE(*it == *it2) << "Initialized iterators must point to the "
                           << "element same as its source points to";

  
  it++;
  EXPECT_FALSE(*it == *it2);
  it2 = it;
  EXPECT_TRUE(*it == *it2) << "Assigned iterators must point to the "
                           << "element same as its source points to";

  
  EXPECT_EQ(&it, &(++it)) << "Result of the prefix operator++ must be "
                          << "refer to the original object";

  
  
  int original_value = *it;  
                             
  EXPECT_EQ(original_value, *(it++));

  
  
  it2 = it;
  it++;
  ++it2;
  EXPECT_TRUE(*it == *it2);
}


TEST(RangeTest, IntRangeWithDefaultStep) {
  const ParamGenerator<int> gen = Range(0, 3);
  const int expected_values[] = {0, 1, 2};
  VerifyGenerator(gen, expected_values);
}



TEST(RangeTest, IntRangeSingleValue) {
  const ParamGenerator<int> gen = Range(0, 1);
  const int expected_values[] = {0};
  VerifyGenerator(gen, expected_values);
}



TEST(RangeTest, IntRangeEmpty) {
  const ParamGenerator<int> gen = Range(0, 0);
  VerifyGeneratorIsEmpty(gen);
}



TEST(RangeTest, IntRangeWithCustomStep) {
  const ParamGenerator<int> gen = Range(0, 9, 3);
  const int expected_values[] = {0, 3, 6};
  VerifyGenerator(gen, expected_values);
}





TEST(RangeTest, IntRangeWithCustomStepOverUpperBound) {
  const ParamGenerator<int> gen = Range(0, 4, 3);
  const int expected_values[] = {0, 3};
  VerifyGenerator(gen, expected_values);
}



class DogAdder {
 public:
  explicit DogAdder(const char* a_value) : value_(a_value) {}
  DogAdder(const DogAdder& other) : value_(other.value_.c_str()) {}

  DogAdder operator=(const DogAdder& other) {
    if (this != &other)
      value_ = other.value_;
    return *this;
  }
  DogAdder operator+(const DogAdder& other) const {
    Message msg;
    msg << value_.c_str() << other.value_.c_str();
    return DogAdder(msg.GetString().c_str());
  }
  bool operator<(const DogAdder& other) const {
    return value_ < other.value_;
  }
  const std::string& value() const { return value_; }

 private:
  std::string value_;
};

TEST(RangeTest, WorksWithACustomType) {
  const ParamGenerator<DogAdder> gen =
      Range(DogAdder("cat"), DogAdder("catdogdog"), DogAdder("dog"));
  ParamGenerator<DogAdder>::iterator it = gen.begin();

  ASSERT_FALSE(it == gen.end());
  EXPECT_STREQ("cat", it->value().c_str());

  ASSERT_FALSE(++it == gen.end());
  EXPECT_STREQ("catdog", it->value().c_str());

  EXPECT_TRUE(++it == gen.end());
}

class IntWrapper {
 public:
  explicit IntWrapper(int a_value) : value_(a_value) {}
  IntWrapper(const IntWrapper& other) : value_(other.value_) {}

  IntWrapper operator=(const IntWrapper& other) {
    value_ = other.value_;
    return *this;
  }
  
  IntWrapper operator+(int other) const { return IntWrapper(value_ + other); }
  bool operator<(const IntWrapper& other) const {
    return value_ < other.value_;
  }
  int value() const { return value_; }

 private:
  int value_;
};

TEST(RangeTest, WorksWithACustomTypeWithDifferentIncrementType) {
  const ParamGenerator<IntWrapper> gen = Range(IntWrapper(0), IntWrapper(2));
  ParamGenerator<IntWrapper>::iterator it = gen.begin();

  ASSERT_FALSE(it == gen.end());
  EXPECT_EQ(0, it->value());

  ASSERT_FALSE(++it == gen.end());
  EXPECT_EQ(1, it->value());

  EXPECT_TRUE(++it == gen.end());
}



TEST(ValuesInTest, ValuesInArray) {
  int array[] = {3, 5, 8};
  const ParamGenerator<int> gen = ValuesIn(array);
  VerifyGenerator(gen, array);
}



TEST(ValuesInTest, ValuesInConstArray) {
  const int array[] = {3, 5, 8};
  const ParamGenerator<int> gen = ValuesIn(array);
  VerifyGenerator(gen, array);
}



TEST(ValuesInTest, ValuesInSingleElementArray) {
  int array[] = {42};
  const ParamGenerator<int> gen = ValuesIn(array);
  VerifyGenerator(gen, array);
}



TEST(ValuesInTest, ValuesInVector) {
  typedef ::std::vector<int> ContainerType;
  ContainerType values;
  values.push_back(3);
  values.push_back(5);
  values.push_back(8);
  const ParamGenerator<int> gen = ValuesIn(values);

  const int expected_values[] = {3, 5, 8};
  VerifyGenerator(gen, expected_values);
}


TEST(ValuesInTest, ValuesInIteratorRange) {
  typedef ::std::vector<int> ContainerType;
  ContainerType values;
  values.push_back(3);
  values.push_back(5);
  values.push_back(8);
  const ParamGenerator<int> gen = ValuesIn(values.begin(), values.end());

  const int expected_values[] = {3, 5, 8};
  VerifyGenerator(gen, expected_values);
}



TEST(ValuesInTest, ValuesInSingleElementIteratorRange) {
  typedef ::std::vector<int> ContainerType;
  ContainerType values;
  values.push_back(42);
  const ParamGenerator<int> gen = ValuesIn(values.begin(), values.end());

  const int expected_values[] = {42};
  VerifyGenerator(gen, expected_values);
}



TEST(ValuesInTest, ValuesInEmptyIteratorRange) {
  typedef ::std::vector<int> ContainerType;
  ContainerType values;
  const ParamGenerator<int> gen = ValuesIn(values.begin(), values.end());

  VerifyGeneratorIsEmpty(gen);
}


TEST(ValuesTest, ValuesWorks) {
  const ParamGenerator<int> gen = Values(3, 5, 8);

  const int expected_values[] = {3, 5, 8};
  VerifyGenerator(gen, expected_values);
}



TEST(ValuesTest, ValuesWorksForValuesOfCompatibleTypes) {
  const ParamGenerator<double> gen = Values(3, 5.0f, 8.0);

  const double expected_values[] = {3.0, 5.0, 8.0};
  VerifyGenerator(gen, expected_values);
}

TEST(ValuesTest, ValuesWorksForMaxLengthList) {
  const ParamGenerator<int> gen = Values(
      10, 20, 30, 40, 50, 60, 70, 80, 90, 100,
      110, 120, 130, 140, 150, 160, 170, 180, 190, 200,
      210, 220, 230, 240, 250, 260, 270, 280, 290, 300,
      310, 320, 330, 340, 350, 360, 370, 380, 390, 400,
      410, 420, 430, 440, 450, 460, 470, 480, 490, 500);

  const int expected_values[] = {
      10, 20, 30, 40, 50, 60, 70, 80, 90, 100,
      110, 120, 130, 140, 150, 160, 170, 180, 190, 200,
      210, 220, 230, 240, 250, 260, 270, 280, 290, 300,
      310, 320, 330, 340, 350, 360, 370, 380, 390, 400,
      410, 420, 430, 440, 450, 460, 470, 480, 490, 500};
  VerifyGenerator(gen, expected_values);
}



TEST(ValuesTest, ValuesWithSingleParameter) {
  const ParamGenerator<int> gen = Values(42);

  const int expected_values[] = {42};
  VerifyGenerator(gen, expected_values);
}


TEST(BoolTest, BoolWorks) {
  const ParamGenerator<bool> gen = Bool();

  const bool expected_values[] = {false, true};
  VerifyGenerator(gen, expected_values);
}

# if GTEST_HAS_COMBINE


TEST(CombineTest, CombineWithTwoParameters) {
  const char* foo = "foo";
  const char* bar = "bar";
  const ParamGenerator<tuple<const char*, int> > gen =
      Combine(Values(foo, bar), Values(3, 4));

  tuple<const char*, int> expected_values[] = {
    make_tuple(foo, 3), make_tuple(foo, 4),
    make_tuple(bar, 3), make_tuple(bar, 4)};
  VerifyGenerator(gen, expected_values);
}


TEST(CombineTest, CombineWithThreeParameters) {
  const ParamGenerator<tuple<int, int, int> > gen = Combine(Values(0, 1),
                                                            Values(3, 4),
                                                            Values(5, 6));
  tuple<int, int, int> expected_values[] = {
    make_tuple(0, 3, 5), make_tuple(0, 3, 6),
    make_tuple(0, 4, 5), make_tuple(0, 4, 6),
    make_tuple(1, 3, 5), make_tuple(1, 3, 6),
    make_tuple(1, 4, 5), make_tuple(1, 4, 6)};
  VerifyGenerator(gen, expected_values);
}




TEST(CombineTest, CombineWithFirstParameterSingleValue) {
  const ParamGenerator<tuple<int, int> > gen = Combine(Values(42),
                                                       Values(0, 1));

  tuple<int, int> expected_values[] = {make_tuple(42, 0), make_tuple(42, 1)};
  VerifyGenerator(gen, expected_values);
}




TEST(CombineTest, CombineWithSecondParameterSingleValue) {
  const ParamGenerator<tuple<int, int> > gen = Combine(Values(0, 1),
                                                       Values(42));

  tuple<int, int> expected_values[] = {make_tuple(0, 42), make_tuple(1, 42)};
  VerifyGenerator(gen, expected_values);
}



TEST(CombineTest, CombineWithFirstParameterEmptyRange) {
  const ParamGenerator<tuple<int, int> > gen = Combine(Range(0, 0),
                                                       Values(0, 1));
  VerifyGeneratorIsEmpty(gen);
}



TEST(CombineTest, CombineWithSecondParameterEmptyRange) {
  const ParamGenerator<tuple<int, int> > gen = Combine(Values(0, 1),
                                                       Range(1, 1));
  VerifyGeneratorIsEmpty(gen);
}



TEST(CombineTest, CombineWithMaxNumberOfParameters) {
  const char* foo = "foo";
  const char* bar = "bar";
  const ParamGenerator<tuple<const char*, int, int, int, int, int, int, int,
                             int, int> > gen = Combine(Values(foo, bar),
                                                       Values(1), Values(2),
                                                       Values(3), Values(4),
                                                       Values(5), Values(6),
                                                       Values(7), Values(8),
                                                       Values(9));

  tuple<const char*, int, int, int, int, int, int, int, int, int>
      expected_values[] = {make_tuple(foo, 1, 2, 3, 4, 5, 6, 7, 8, 9),
                           make_tuple(bar, 1, 2, 3, 4, 5, 6, 7, 8, 9)};
  VerifyGenerator(gen, expected_values);
}

# endif  



TEST(ParamGeneratorTest, AssignmentWorks) {
  ParamGenerator<int> gen = Values(1, 2);
  const ParamGenerator<int> gen2 = Values(3, 4);
  gen = gen2;

  const int expected_values[] = {3, 4};
  VerifyGenerator(gen, expected_values);
}










template <int kExpectedCalls>
class TestGenerationEnvironment : public ::testing::Environment {
 public:
  static TestGenerationEnvironment* Instance() {
    static TestGenerationEnvironment* instance = new TestGenerationEnvironment;
    return instance;
  }

  void FixtureConstructorExecuted() { fixture_constructor_count_++; }
  void SetUpExecuted() { set_up_count_++; }
  void TearDownExecuted() { tear_down_count_++; }
  void TestBodyExecuted() { test_body_count_++; }

  virtual void TearDown() {
    
    
    bool perform_check = false;

    for (int i = 0; i < kExpectedCalls; ++i) {
      Message msg;
      msg << "TestsExpandedAndRun/" << i;
      if (UnitTestOptions::FilterMatchesTest(
             "TestExpansionModule/MultipleTestGenerationTest",
              msg.GetString().c_str())) {
        perform_check = true;
      }
    }
    if (perform_check) {
      EXPECT_EQ(kExpectedCalls, fixture_constructor_count_)
          << "Fixture constructor of ParamTestGenerationTest test case "
          << "has not been run as expected.";
      EXPECT_EQ(kExpectedCalls, set_up_count_)
          << "Fixture SetUp method of ParamTestGenerationTest test case "
          << "has not been run as expected.";
      EXPECT_EQ(kExpectedCalls, tear_down_count_)
          << "Fixture TearDown method of ParamTestGenerationTest test case "
          << "has not been run as expected.";
      EXPECT_EQ(kExpectedCalls, test_body_count_)
          << "Test in ParamTestGenerationTest test case "
          << "has not been run as expected.";
    }
  }

 private:
  TestGenerationEnvironment() : fixture_constructor_count_(0), set_up_count_(0),
                                tear_down_count_(0), test_body_count_(0) {}

  int fixture_constructor_count_;
  int set_up_count_;
  int tear_down_count_;
  int test_body_count_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(TestGenerationEnvironment);
};

const int test_generation_params[] = {36, 42, 72};

class TestGenerationTest : public TestWithParam<int> {
 public:
  enum {
    PARAMETER_COUNT =
        sizeof(test_generation_params)/sizeof(test_generation_params[0])
  };

  typedef TestGenerationEnvironment<PARAMETER_COUNT> Environment;

  TestGenerationTest() {
    Environment::Instance()->FixtureConstructorExecuted();
    current_parameter_ = GetParam();
  }
  virtual void SetUp() {
    Environment::Instance()->SetUpExecuted();
    EXPECT_EQ(current_parameter_, GetParam());
  }
  virtual void TearDown() {
    Environment::Instance()->TearDownExecuted();
    EXPECT_EQ(current_parameter_, GetParam());
  }

  static void SetUpTestCase() {
    bool all_tests_in_test_case_selected = true;

    for (int i = 0; i < PARAMETER_COUNT; ++i) {
      Message test_name;
      test_name << "TestsExpandedAndRun/" << i;
      if ( !UnitTestOptions::FilterMatchesTest(
                "TestExpansionModule/MultipleTestGenerationTest",
                test_name.GetString())) {
        all_tests_in_test_case_selected = false;
      }
    }
    EXPECT_TRUE(all_tests_in_test_case_selected)
        << "When running the TestGenerationTest test case all of its tests\n"
        << "must be selected by the filter flag for the test case to pass.\n"
        << "If not all of them are enabled, we can't reliably conclude\n"
        << "that the correct number of tests have been generated.";

    collected_parameters_.clear();
  }

  static void TearDownTestCase() {
    vector<int> expected_values(test_generation_params,
                                test_generation_params + PARAMETER_COUNT);
    
    
    
    sort(expected_values.begin(), expected_values.end());
    sort(collected_parameters_.begin(), collected_parameters_.end());

    EXPECT_TRUE(collected_parameters_ == expected_values);
  }

 protected:
  int current_parameter_;
  static vector<int> collected_parameters_;

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(TestGenerationTest);
};
vector<int> TestGenerationTest::collected_parameters_;

TEST_P(TestGenerationTest, TestsExpandedAndRun) {
  Environment::Instance()->TestBodyExecuted();
  EXPECT_EQ(current_parameter_, GetParam());
  collected_parameters_.push_back(GetParam());
}
INSTANTIATE_TEST_CASE_P(TestExpansionModule, TestGenerationTest,
                        ValuesIn(test_generation_params));











class GeneratorEvaluationTest : public TestWithParam<int> {
 public:
  static int param_value() { return param_value_; }
  static void set_param_value(int param_value) { param_value_ = param_value; }

 private:
  static int param_value_;
};
int GeneratorEvaluationTest::param_value_ = 0;

TEST_P(GeneratorEvaluationTest, GeneratorsEvaluatedInMain) {
  EXPECT_EQ(1, GetParam());
}
INSTANTIATE_TEST_CASE_P(GenEvalModule,
                        GeneratorEvaluationTest,
                        Values(GeneratorEvaluationTest::param_value()));



extern ParamGenerator<int> extern_gen;
class ExternalGeneratorTest : public TestWithParam<int> {};
TEST_P(ExternalGeneratorTest, ExternalGenerator) {
  
  
  EXPECT_EQ(GetParam(), 33);
}
INSTANTIATE_TEST_CASE_P(ExternalGeneratorModule,
                        ExternalGeneratorTest,
                        extern_gen);





TEST_P(ExternalInstantiationTest, IsMultipleOf33) {
  EXPECT_EQ(0, GetParam() % 33);
}



class MultipleInstantiationTest : public TestWithParam<int> {};
TEST_P(MultipleInstantiationTest, AllowsMultipleInstances) {
}
INSTANTIATE_TEST_CASE_P(Sequence1, MultipleInstantiationTest, Values(1, 2));
INSTANTIATE_TEST_CASE_P(Sequence2, MultipleInstantiationTest, Range(3, 5));






TEST_P(InstantiationInMultipleTranslaionUnitsTest, IsMultipleOf42) {
  EXPECT_EQ(0, GetParam() % 42);
}
INSTANTIATE_TEST_CASE_P(Sequence1,
                        InstantiationInMultipleTranslaionUnitsTest,
                        Values(42, 42*2));



class SeparateInstanceTest : public TestWithParam<int> {
 public:
  SeparateInstanceTest() : count_(0) {}

  static void TearDownTestCase() {
    EXPECT_GE(global_count_, 2)
        << "If some (but not all) SeparateInstanceTest tests have been "
        << "filtered out this test will fail. Make sure that all "
        << "GeneratorEvaluationTest are selected or de-selected together "
        << "by the test filter.";
  }

 protected:
  int count_;
  static int global_count_;
};
int SeparateInstanceTest::global_count_ = 0;

TEST_P(SeparateInstanceTest, TestsRunInSeparateInstances) {
  EXPECT_EQ(0, count_++);
  global_count_++;
}
INSTANTIATE_TEST_CASE_P(FourElemSequence, SeparateInstanceTest, Range(1, 4));






class NamingTest : public TestWithParam<int> {};

TEST_P(NamingTest, TestsReportCorrectNamesAndParameters) {
  const ::testing::TestInfo* const test_info =
     ::testing::UnitTest::GetInstance()->current_test_info();

  EXPECT_STREQ("ZeroToFiveSequence/NamingTest", test_info->test_case_name());

  Message index_stream;
  index_stream << "TestsReportCorrectNamesAndParameters/" << GetParam();
  EXPECT_STREQ(index_stream.GetString().c_str(), test_info->name());

  EXPECT_EQ(::testing::PrintToString(GetParam()), test_info->value_param());
}

INSTANTIATE_TEST_CASE_P(ZeroToFiveSequence, NamingTest, Range(0, 5));





class Unstreamable {
 public:
  explicit Unstreamable(int value) : value_(value) {}

 private:
  int value_;
};

class CommentTest : public TestWithParam<Unstreamable> {};

TEST_P(CommentTest, TestsCorrectlyReportUnstreamableParams) {
  const ::testing::TestInfo* const test_info =
     ::testing::UnitTest::GetInstance()->current_test_info();

  EXPECT_EQ(::testing::PrintToString(GetParam()), test_info->value_param());
}

INSTANTIATE_TEST_CASE_P(InstantiationWithComments,
                        CommentTest,
                        Values(Unstreamable(1)));





class NonParameterizedBaseTest : public ::testing::Test {
 public:
  NonParameterizedBaseTest() : n_(17) { }
 protected:
  int n_;
};

class ParameterizedDerivedTest : public NonParameterizedBaseTest,
                                 public ::testing::WithParamInterface<int> {
 protected:
  ParameterizedDerivedTest() : count_(0) { }
  int count_;
  static int global_count_;
};

int ParameterizedDerivedTest::global_count_ = 0;

TEST_F(NonParameterizedBaseTest, FixtureIsInitialized) {
  EXPECT_EQ(17, n_);
}

TEST_P(ParameterizedDerivedTest, SeesSequence) {
  EXPECT_EQ(17, n_);
  EXPECT_EQ(0, count_++);
  EXPECT_EQ(GetParam(), global_count_++);
}

class ParameterizedDeathTest : public ::testing::TestWithParam<int> { };

TEST_F(ParameterizedDeathTest, GetParamDiesFromTestF) {
  EXPECT_DEATH_IF_SUPPORTED(GetParam(),
                            ".* value-parameterized test .*");
}

INSTANTIATE_TEST_CASE_P(RangeZeroToFive, ParameterizedDerivedTest, Range(0, 5));

#endif  

TEST(CompileTest, CombineIsDefinedOnlyWhenGtestHasParamTestIsDefined) {
#if GTEST_HAS_COMBINE && !GTEST_HAS_PARAM_TEST
  FAIL() << "GTEST_HAS_COMBINE is defined while GTEST_HAS_PARAM_TEST is not\n"
#endif
}

int main(int argc, char **argv) {
#if GTEST_HAS_PARAM_TEST
  
  AddGlobalTestEnvironment(TestGenerationTest::Environment::Instance());
  
  
  GeneratorEvaluationTest::set_param_value(1);
#endif  

  ::testing::InitGoogleTest(&argc, argv);

#if GTEST_HAS_PARAM_TEST
  
  
  
  GeneratorEvaluationTest::set_param_value(2);
#endif  

  return RUN_ALL_TESTS();
}

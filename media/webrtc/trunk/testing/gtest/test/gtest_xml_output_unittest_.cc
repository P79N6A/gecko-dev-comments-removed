







































#include "gtest/gtest.h"

using ::testing::InitGoogleTest;
using ::testing::TestEventListeners;
using ::testing::TestWithParam;
using ::testing::UnitTest;
using ::testing::Test;
using ::testing::Types;
using ::testing::Values;

class SuccessfulTest : public Test {
};

TEST_F(SuccessfulTest, Succeeds) {
  SUCCEED() << "This is a success.";
  ASSERT_EQ(1, 1);
}

class FailedTest : public Test {
};

TEST_F(FailedTest, Fails) {
  ASSERT_EQ(1, 2);
}

class DisabledTest : public Test {
};

TEST_F(DisabledTest, DISABLED_test_not_run) {
  FAIL() << "Unexpected failure: Disabled test should not be run";
}

TEST(MixedResultTest, Succeeds) {
  EXPECT_EQ(1, 1);
  ASSERT_EQ(1, 1);
}

TEST(MixedResultTest, Fails) {
  EXPECT_EQ(1, 2);
  ASSERT_EQ(2, 3);
}

TEST(MixedResultTest, DISABLED_test) {
  FAIL() << "Unexpected failure: Disabled test should not be run";
}

TEST(XmlQuotingTest, OutputsCData) {
  FAIL() << "XML output: "
            "<?xml encoding=\"utf-8\"><top><![CDATA[cdata text]]></top>";
}



TEST(InvalidCharactersTest, InvalidCharactersInMessage) {
  FAIL() << "Invalid characters in brackets [\x1\x2]";
}

class PropertyRecordingTest : public Test {
};

TEST_F(PropertyRecordingTest, OneProperty) {
  RecordProperty("key_1", "1");
}

TEST_F(PropertyRecordingTest, IntValuedProperty) {
  RecordProperty("key_int", 1);
}

TEST_F(PropertyRecordingTest, ThreeProperties) {
  RecordProperty("key_1", "1");
  RecordProperty("key_2", "2");
  RecordProperty("key_3", "3");
}

TEST_F(PropertyRecordingTest, TwoValuesForOneKeyUsesLastValue) {
  RecordProperty("key_1", "1");
  RecordProperty("key_1", "2");
}

TEST(NoFixtureTest, RecordProperty) {
  RecordProperty("key", "1");
}

void ExternalUtilityThatCallsRecordProperty(const char* key, int value) {
  testing::Test::RecordProperty(key, value);
}

void ExternalUtilityThatCallsRecordProperty(const char* key,
                                            const char* value) {
  testing::Test::RecordProperty(key, value);
}

TEST(NoFixtureTest, ExternalUtilityThatCallsRecordIntValuedProperty) {
  ExternalUtilityThatCallsRecordProperty("key_for_utility_int", 1);
}

TEST(NoFixtureTest, ExternalUtilityThatCallsRecordStringValuedProperty) {
  ExternalUtilityThatCallsRecordProperty("key_for_utility_string", "1");
}



class ValueParamTest : public TestWithParam<int> {};
TEST_P(ValueParamTest, HasValueParamAttribute) {}
TEST_P(ValueParamTest, AnotherTestThatHasValueParamAttribute) {}
INSTANTIATE_TEST_CASE_P(Single, ValueParamTest, Values(33, 42));



template <typename T> class TypedTest : public Test {};
typedef Types<int, long> TypedTestTypes;
TYPED_TEST_CASE(TypedTest, TypedTestTypes);
TYPED_TEST(TypedTest, HasTypeParamAttribute) {}



template <typename T> class TypeParameterizedTestCase : public Test {};
TYPED_TEST_CASE_P(TypeParameterizedTestCase);
TYPED_TEST_P(TypeParameterizedTestCase, HasTypeParamAttribute) {}
REGISTER_TYPED_TEST_CASE_P(TypeParameterizedTestCase, HasTypeParamAttribute);
typedef Types<int, long> TypeParameterizedTestCaseTypes;
INSTANTIATE_TYPED_TEST_CASE_P(Single,
                              TypeParameterizedTestCase,
                              TypeParameterizedTestCaseTypes);

int main(int argc, char** argv) {
  InitGoogleTest(&argc, argv);

  if (argc > 1 && strcmp(argv[1], "--shut_down_xml") == 0) {
    TestEventListeners& listeners = UnitTest::GetInstance()->listeners();
    delete listeners.Release(listeners.default_xml_generator());
  }
  return RUN_ALL_TESTS();
}

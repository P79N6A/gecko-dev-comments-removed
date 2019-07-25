






























"""Tests that leaked mock objects can be caught be Google Mock."""

__author__ = 'wan@google.com (Zhanyong Wan)'


import gmock_test_utils


PROGRAM_PATH = gmock_test_utils.GetTestExecutablePath('gmock_leak_test_')
TEST_WITH_EXPECT_CALL = [PROGRAM_PATH, '--gtest_filter=*ExpectCall*']
TEST_WITH_ON_CALL = [PROGRAM_PATH, '--gtest_filter=*OnCall*']
TEST_MULTIPLE_LEAKS = [PROGRAM_PATH, '--gtest_filter=*MultipleLeaked*']


class GMockLeakTest(gmock_test_utils.TestCase):

  def testCatchesLeakedMockByDefault(self):
    self.assertNotEqual(
        0,
        gmock_test_utils.Subprocess(TEST_WITH_EXPECT_CALL).exit_code)
    self.assertNotEqual(
        0,
        gmock_test_utils.Subprocess(TEST_WITH_ON_CALL).exit_code)

  def testDoesNotCatchLeakedMockWhenDisabled(self):
    self.assertEquals(
        0,
        gmock_test_utils.Subprocess(TEST_WITH_EXPECT_CALL +
                                    ['--gmock_catch_leaked_mocks=0']).exit_code)
    self.assertEquals(
        0,
        gmock_test_utils.Subprocess(TEST_WITH_ON_CALL +
                                    ['--gmock_catch_leaked_mocks=0']).exit_code)

  def testCatchesLeakedMockWhenEnabled(self):
    self.assertNotEqual(
        0,
        gmock_test_utils.Subprocess(TEST_WITH_EXPECT_CALL +
                                    ['--gmock_catch_leaked_mocks']).exit_code)
    self.assertNotEqual(
        0,
        gmock_test_utils.Subprocess(TEST_WITH_ON_CALL +
                                    ['--gmock_catch_leaked_mocks']).exit_code)

  def testCatchesLeakedMockWhenEnabledWithExplictFlagValue(self):
    self.assertNotEqual(
        0,
        gmock_test_utils.Subprocess(TEST_WITH_EXPECT_CALL +
                                    ['--gmock_catch_leaked_mocks=1']).exit_code)

  def testCatchesMultipleLeakedMocks(self):
    self.assertNotEqual(
        0,
        gmock_test_utils.Subprocess(TEST_MULTIPLE_LEAKS +
                                    ['--gmock_catch_leaked_mocks']).exit_code)


if __name__ == '__main__':
  gmock_test_utils.Main()































"""Verifies that test shuffling works."""

__author__ = 'wan@google.com (Zhanyong Wan)'

import os
import gtest_test_utils


COMMAND = gtest_test_utils.GetTestExecutablePath('gtest_shuffle_test_')


TOTAL_SHARDS_ENV_VAR = 'GTEST_TOTAL_SHARDS'
SHARD_INDEX_ENV_VAR = 'GTEST_SHARD_INDEX'

TEST_FILTER = 'A*.A:A*.B:C*'

ALL_TESTS = []
ACTIVE_TESTS = []
FILTERED_TESTS = []
SHARDED_TESTS = []

SHUFFLED_ALL_TESTS = []
SHUFFLED_ACTIVE_TESTS = []
SHUFFLED_FILTERED_TESTS = []
SHUFFLED_SHARDED_TESTS = []


def AlsoRunDisabledTestsFlag():
  return '--gtest_also_run_disabled_tests'


def FilterFlag(test_filter):
  return '--gtest_filter=%s' % (test_filter,)


def RepeatFlag(n):
  return '--gtest_repeat=%s' % (n,)


def ShuffleFlag():
  return '--gtest_shuffle'


def RandomSeedFlag(n):
  return '--gtest_random_seed=%s' % (n,)


def RunAndReturnOutput(extra_env, args):
  """Runs the test program and returns its output."""

  environ_copy = os.environ.copy()
  environ_copy.update(extra_env)

  return gtest_test_utils.Subprocess([COMMAND] + args, env=environ_copy,
                                     capture_stderr=False).output


def GetTestsForAllIterations(extra_env, args):
  """Runs the test program and returns a list of test lists.

  Args:
    extra_env: a map from environment variables to their values
    args: command line flags to pass to gtest_shuffle_test_

  Returns:
    A list where the i-th element is the list of tests run in the i-th
    test iteration.
  """

  test_iterations = []
  for line in RunAndReturnOutput(extra_env, args).split('\n'):
    if line.startswith('----'):
      tests = []
      test_iterations.append(tests)
    elif line.strip():
      tests.append(line.strip())  

  return test_iterations


def GetTestCases(tests):
  """Returns a list of test cases in the given full test names.

  Args:
    tests: a list of full test names

  Returns:
    A list of test cases from 'tests', in their original order.
    Consecutive duplicates are removed.
  """

  test_cases = []
  for test in tests:
    test_case = test.split('.')[0]
    if not test_case in test_cases:
      test_cases.append(test_case)

  return test_cases


def CalculateTestLists():
  """Calculates the list of tests run under different flags."""

  if not ALL_TESTS:
    ALL_TESTS.extend(
        GetTestsForAllIterations({}, [AlsoRunDisabledTestsFlag()])[0])

  if not ACTIVE_TESTS:
    ACTIVE_TESTS.extend(GetTestsForAllIterations({}, [])[0])

  if not FILTERED_TESTS:
    FILTERED_TESTS.extend(
        GetTestsForAllIterations({}, [FilterFlag(TEST_FILTER)])[0])

  if not SHARDED_TESTS:
    SHARDED_TESTS.extend(
        GetTestsForAllIterations({TOTAL_SHARDS_ENV_VAR: '3',
                                  SHARD_INDEX_ENV_VAR: '1'},
                                 [])[0])

  if not SHUFFLED_ALL_TESTS:
    SHUFFLED_ALL_TESTS.extend(GetTestsForAllIterations(
        {}, [AlsoRunDisabledTestsFlag(), ShuffleFlag(), RandomSeedFlag(1)])[0])

  if not SHUFFLED_ACTIVE_TESTS:
    SHUFFLED_ACTIVE_TESTS.extend(GetTestsForAllIterations(
        {}, [ShuffleFlag(), RandomSeedFlag(1)])[0])

  if not SHUFFLED_FILTERED_TESTS:
    SHUFFLED_FILTERED_TESTS.extend(GetTestsForAllIterations(
        {}, [ShuffleFlag(), RandomSeedFlag(1), FilterFlag(TEST_FILTER)])[0])

  if not SHUFFLED_SHARDED_TESTS:
    SHUFFLED_SHARDED_TESTS.extend(
        GetTestsForAllIterations({TOTAL_SHARDS_ENV_VAR: '3',
                                  SHARD_INDEX_ENV_VAR: '1'},
                                 [ShuffleFlag(), RandomSeedFlag(1)])[0])


class GTestShuffleUnitTest(gtest_test_utils.TestCase):
  """Tests test shuffling."""

  def setUp(self):
    CalculateTestLists()

  def testShufflePreservesNumberOfTests(self):
    self.assertEqual(len(ALL_TESTS), len(SHUFFLED_ALL_TESTS))
    self.assertEqual(len(ACTIVE_TESTS), len(SHUFFLED_ACTIVE_TESTS))
    self.assertEqual(len(FILTERED_TESTS), len(SHUFFLED_FILTERED_TESTS))
    self.assertEqual(len(SHARDED_TESTS), len(SHUFFLED_SHARDED_TESTS))

  def testShuffleChangesTestOrder(self):
    self.assert_(SHUFFLED_ALL_TESTS != ALL_TESTS, SHUFFLED_ALL_TESTS)
    self.assert_(SHUFFLED_ACTIVE_TESTS != ACTIVE_TESTS, SHUFFLED_ACTIVE_TESTS)
    self.assert_(SHUFFLED_FILTERED_TESTS != FILTERED_TESTS,
                 SHUFFLED_FILTERED_TESTS)
    self.assert_(SHUFFLED_SHARDED_TESTS != SHARDED_TESTS,
                 SHUFFLED_SHARDED_TESTS)

  def testShuffleChangesTestCaseOrder(self):
    self.assert_(GetTestCases(SHUFFLED_ALL_TESTS) != GetTestCases(ALL_TESTS),
                 GetTestCases(SHUFFLED_ALL_TESTS))
    self.assert_(
        GetTestCases(SHUFFLED_ACTIVE_TESTS) != GetTestCases(ACTIVE_TESTS),
        GetTestCases(SHUFFLED_ACTIVE_TESTS))
    self.assert_(
        GetTestCases(SHUFFLED_FILTERED_TESTS) != GetTestCases(FILTERED_TESTS),
        GetTestCases(SHUFFLED_FILTERED_TESTS))
    self.assert_(
        GetTestCases(SHUFFLED_SHARDED_TESTS) != GetTestCases(SHARDED_TESTS),
        GetTestCases(SHUFFLED_SHARDED_TESTS))

  def testShuffleDoesNotRepeatTest(self):
    for test in SHUFFLED_ALL_TESTS:
      self.assertEqual(1, SHUFFLED_ALL_TESTS.count(test),
                       '%s appears more than once' % (test,))
    for test in SHUFFLED_ACTIVE_TESTS:
      self.assertEqual(1, SHUFFLED_ACTIVE_TESTS.count(test),
                       '%s appears more than once' % (test,))
    for test in SHUFFLED_FILTERED_TESTS:
      self.assertEqual(1, SHUFFLED_FILTERED_TESTS.count(test),
                       '%s appears more than once' % (test,))
    for test in SHUFFLED_SHARDED_TESTS:
      self.assertEqual(1, SHUFFLED_SHARDED_TESTS.count(test),
                       '%s appears more than once' % (test,))

  def testShuffleDoesNotCreateNewTest(self):
    for test in SHUFFLED_ALL_TESTS:
      self.assert_(test in ALL_TESTS, '%s is an invalid test' % (test,))
    for test in SHUFFLED_ACTIVE_TESTS:
      self.assert_(test in ACTIVE_TESTS, '%s is an invalid test' % (test,))
    for test in SHUFFLED_FILTERED_TESTS:
      self.assert_(test in FILTERED_TESTS, '%s is an invalid test' % (test,))
    for test in SHUFFLED_SHARDED_TESTS:
      self.assert_(test in SHARDED_TESTS, '%s is an invalid test' % (test,))

  def testShuffleIncludesAllTests(self):
    for test in ALL_TESTS:
      self.assert_(test in SHUFFLED_ALL_TESTS, '%s is missing' % (test,))
    for test in ACTIVE_TESTS:
      self.assert_(test in SHUFFLED_ACTIVE_TESTS, '%s is missing' % (test,))
    for test in FILTERED_TESTS:
      self.assert_(test in SHUFFLED_FILTERED_TESTS, '%s is missing' % (test,))
    for test in SHARDED_TESTS:
      self.assert_(test in SHUFFLED_SHARDED_TESTS, '%s is missing' % (test,))

  def testShuffleLeavesDeathTestsAtFront(self):
    non_death_test_found = False
    for test in SHUFFLED_ACTIVE_TESTS:
      if 'DeathTest.' in test:
        self.assert_(not non_death_test_found,
                     '%s appears after a non-death test' % (test,))
      else:
        non_death_test_found = True

  def _VerifyTestCasesDoNotInterleave(self, tests):
    test_cases = []
    for test in tests:
      [test_case, _] = test.split('.')
      if test_cases and test_cases[-1] != test_case:
        test_cases.append(test_case)
        self.assertEqual(1, test_cases.count(test_case),
                         'Test case %s is not grouped together in %s' %
                         (test_case, tests))

  def testShuffleDoesNotInterleaveTestCases(self):
    self._VerifyTestCasesDoNotInterleave(SHUFFLED_ALL_TESTS)
    self._VerifyTestCasesDoNotInterleave(SHUFFLED_ACTIVE_TESTS)
    self._VerifyTestCasesDoNotInterleave(SHUFFLED_FILTERED_TESTS)
    self._VerifyTestCasesDoNotInterleave(SHUFFLED_SHARDED_TESTS)

  def testShuffleRestoresOrderAfterEachIteration(self):
    
    
    
    
    
    [tests_in_iteration1, tests_in_iteration2, tests_in_iteration3] = (
        GetTestsForAllIterations(
            {}, [ShuffleFlag(), RandomSeedFlag(1), RepeatFlag(3)]))

    
    
    [tests_with_seed1] = GetTestsForAllIterations(
        {}, [ShuffleFlag(), RandomSeedFlag(1)])
    self.assertEqual(tests_in_iteration1, tests_with_seed1)

    
    
    
    
    [tests_with_seed2] = GetTestsForAllIterations(
        {}, [ShuffleFlag(), RandomSeedFlag(2)])
    self.assertEqual(tests_in_iteration2, tests_with_seed2)

    
    
    
    
    [tests_with_seed3] = GetTestsForAllIterations(
        {}, [ShuffleFlag(), RandomSeedFlag(3)])
    self.assertEqual(tests_in_iteration3, tests_with_seed3)

  def testShuffleGeneratesNewOrderInEachIteration(self):
    [tests_in_iteration1, tests_in_iteration2, tests_in_iteration3] = (
        GetTestsForAllIterations(
            {}, [ShuffleFlag(), RandomSeedFlag(1), RepeatFlag(3)]))

    self.assert_(tests_in_iteration1 != tests_in_iteration2,
                 tests_in_iteration1)
    self.assert_(tests_in_iteration1 != tests_in_iteration3,
                 tests_in_iteration1)
    self.assert_(tests_in_iteration2 != tests_in_iteration3,
                 tests_in_iteration2)

  def testShuffleShardedTestsPreservesPartition(self):
    
    
    [tests1] = GetTestsForAllIterations({TOTAL_SHARDS_ENV_VAR: '3',
                                         SHARD_INDEX_ENV_VAR: '0'},
                                        [ShuffleFlag(), RandomSeedFlag(1)])
    [tests2] = GetTestsForAllIterations({TOTAL_SHARDS_ENV_VAR: '3',
                                         SHARD_INDEX_ENV_VAR: '1'},
                                        [ShuffleFlag(), RandomSeedFlag(20)])
    [tests3] = GetTestsForAllIterations({TOTAL_SHARDS_ENV_VAR: '3',
                                         SHARD_INDEX_ENV_VAR: '2'},
                                        [ShuffleFlag(), RandomSeedFlag(25)])
    sorted_sharded_tests = tests1 + tests2 + tests3
    sorted_sharded_tests.sort()
    sorted_active_tests = []
    sorted_active_tests.extend(ACTIVE_TESTS)
    sorted_active_tests.sort()
    self.assertEqual(sorted_active_tests, sorted_sharded_tests)

if __name__ == '__main__':
  gtest_test_utils.Main()

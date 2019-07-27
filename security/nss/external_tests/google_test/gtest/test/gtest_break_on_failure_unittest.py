






























"""Unit test for Google Test's break-on-failure mode.

A user can ask Google Test to seg-fault when an assertion fails, using
either the GTEST_BREAK_ON_FAILURE environment variable or the
--gtest_break_on_failure flag.  This script tests such functionality
by invoking gtest_break_on_failure_unittest_ (a program written with
Google Test) with different environments and command line flags.
"""

__author__ = 'wan@google.com (Zhanyong Wan)'

import gtest_test_utils
import os
import sys




IS_WINDOWS = os.name == 'nt'


BREAK_ON_FAILURE_ENV_VAR = 'GTEST_BREAK_ON_FAILURE'


BREAK_ON_FAILURE_FLAG = 'gtest_break_on_failure'


THROW_ON_FAILURE_ENV_VAR = 'GTEST_THROW_ON_FAILURE'


CATCH_EXCEPTIONS_ENV_VAR = 'GTEST_CATCH_EXCEPTIONS'


EXE_PATH = gtest_test_utils.GetTestExecutablePath(
    'gtest_break_on_failure_unittest_')


environ = gtest_test_utils.environ
SetEnvVar = gtest_test_utils.SetEnvVar






SetEnvVar(gtest_test_utils.PREMATURE_EXIT_FILE_ENV_VAR, None)


def Run(command):
  """Runs a command; returns 1 if it was killed by a signal, or 0 otherwise."""

  p = gtest_test_utils.Subprocess(command, env=environ)
  if p.terminated_by_signal:
    return 1
  else:
    return 0





class GTestBreakOnFailureUnitTest(gtest_test_utils.TestCase):
  """Tests using the GTEST_BREAK_ON_FAILURE environment variable or
  the --gtest_break_on_failure flag to turn assertion failures into
  segmentation faults.
  """

  def RunAndVerify(self, env_var_value, flag_value, expect_seg_fault):
    """Runs gtest_break_on_failure_unittest_ and verifies that it does
    (or does not) have a seg-fault.

    Args:
      env_var_value:    value of the GTEST_BREAK_ON_FAILURE environment
                        variable; None if the variable should be unset.
      flag_value:       value of the --gtest_break_on_failure flag;
                        None if the flag should not be present.
      expect_seg_fault: 1 if the program is expected to generate a seg-fault;
                        0 otherwise.
    """

    SetEnvVar(BREAK_ON_FAILURE_ENV_VAR, env_var_value)

    if env_var_value is None:
      env_var_value_msg = ' is not set'
    else:
      env_var_value_msg = '=' + env_var_value

    if flag_value is None:
      flag = ''
    elif flag_value == '0':
      flag = '--%s=0' % BREAK_ON_FAILURE_FLAG
    else:
      flag = '--%s' % BREAK_ON_FAILURE_FLAG

    command = [EXE_PATH]
    if flag:
      command.append(flag)

    if expect_seg_fault:
      should_or_not = 'should'
    else:
      should_or_not = 'should not'

    has_seg_fault = Run(command)

    SetEnvVar(BREAK_ON_FAILURE_ENV_VAR, None)

    msg = ('when %s%s, an assertion failure in "%s" %s cause a seg-fault.' %
           (BREAK_ON_FAILURE_ENV_VAR, env_var_value_msg, ' '.join(command),
            should_or_not))
    self.assert_(has_seg_fault == expect_seg_fault, msg)

  def testDefaultBehavior(self):
    """Tests the behavior of the default mode."""

    self.RunAndVerify(env_var_value=None,
                      flag_value=None,
                      expect_seg_fault=0)

  def testEnvVar(self):
    """Tests using the GTEST_BREAK_ON_FAILURE environment variable."""

    self.RunAndVerify(env_var_value='0',
                      flag_value=None,
                      expect_seg_fault=0)
    self.RunAndVerify(env_var_value='1',
                      flag_value=None,
                      expect_seg_fault=1)

  def testFlag(self):
    """Tests using the --gtest_break_on_failure flag."""

    self.RunAndVerify(env_var_value=None,
                      flag_value='0',
                      expect_seg_fault=0)
    self.RunAndVerify(env_var_value=None,
                      flag_value='1',
                      expect_seg_fault=1)

  def testFlagOverridesEnvVar(self):
    """Tests that the flag overrides the environment variable."""

    self.RunAndVerify(env_var_value='0',
                      flag_value='0',
                      expect_seg_fault=0)
    self.RunAndVerify(env_var_value='0',
                      flag_value='1',
                      expect_seg_fault=1)
    self.RunAndVerify(env_var_value='1',
                      flag_value='0',
                      expect_seg_fault=0)
    self.RunAndVerify(env_var_value='1',
                      flag_value='1',
                      expect_seg_fault=1)

  def testBreakOnFailureOverridesThrowOnFailure(self):
    """Tests that gtest_break_on_failure overrides gtest_throw_on_failure."""

    SetEnvVar(THROW_ON_FAILURE_ENV_VAR, '1')
    try:
      self.RunAndVerify(env_var_value=None,
                        flag_value='1',
                        expect_seg_fault=1)
    finally:
      SetEnvVar(THROW_ON_FAILURE_ENV_VAR, None)

  if IS_WINDOWS:
    def testCatchExceptionsDoesNotInterfere(self):
      """Tests that gtest_catch_exceptions doesn't interfere."""

      SetEnvVar(CATCH_EXCEPTIONS_ENV_VAR, '1')
      try:
        self.RunAndVerify(env_var_value='1',
                          flag_value='1',
                          expect_seg_fault=1)
      finally:
        SetEnvVar(CATCH_EXCEPTIONS_ENV_VAR, None)


if __name__ == '__main__':
  gtest_test_utils.Main()

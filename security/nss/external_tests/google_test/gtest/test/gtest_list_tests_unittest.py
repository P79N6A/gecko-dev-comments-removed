






























"""Unit test for Google Test's --gtest_list_tests flag.

A user can ask Google Test to list all tests by specifying the
--gtest_list_tests flag.  This script tests such functionality
by invoking gtest_list_tests_unittest_ (a program written with
Google Test) the command line flags.
"""

__author__ = 'phanna@google.com (Patrick Hanna)'

import gtest_test_utils
import re





LIST_TESTS_FLAG = 'gtest_list_tests'


EXE_PATH = gtest_test_utils.GetTestExecutablePath('gtest_list_tests_unittest_')



EXPECTED_OUTPUT_NO_FILTER_RE = re.compile(r"""FooDeathTest\.
  Test1
Foo\.
  Bar1
  Bar2
  DISABLED_Bar3
Abc\.
  Xyz
  Def
FooBar\.
  Baz
FooTest\.
  Test1
  DISABLED_Test2
  Test3
TypedTest/0\.  # TypeParam = (VeryLo{245}|class VeryLo{239})\.\.\.
  TestA
  TestB
TypedTest/1\.  # TypeParam = int\s*\*
  TestA
  TestB
TypedTest/2\.  # TypeParam = .*MyArray<bool,\s*42>
  TestA
  TestB
My/TypeParamTest/0\.  # TypeParam = (VeryLo{245}|class VeryLo{239})\.\.\.
  TestA
  TestB
My/TypeParamTest/1\.  # TypeParam = int\s*\*
  TestA
  TestB
My/TypeParamTest/2\.  # TypeParam = .*MyArray<bool,\s*42>
  TestA
  TestB
MyInstantiation/ValueParamTest\.
  TestA/0  # GetParam\(\) = one line
  TestA/1  # GetParam\(\) = two\\nlines
  TestA/2  # GetParam\(\) = a very\\nlo{241}\.\.\.
  TestB/0  # GetParam\(\) = one line
  TestB/1  # GetParam\(\) = two\\nlines
  TestB/2  # GetParam\(\) = a very\\nlo{241}\.\.\.
""")



EXPECTED_OUTPUT_FILTER_FOO_RE = re.compile(r"""FooDeathTest\.
  Test1
Foo\.
  Bar1
  Bar2
  DISABLED_Bar3
FooBar\.
  Baz
FooTest\.
  Test1
  DISABLED_Test2
  Test3
""")




def Run(args):
  """Runs gtest_list_tests_unittest_ and returns the list of tests printed."""

  return gtest_test_utils.Subprocess([EXE_PATH] + args,
                                     capture_stderr=False).output




class GTestListTestsUnitTest(gtest_test_utils.TestCase):
  """Tests using the --gtest_list_tests flag to list all tests."""

  def RunAndVerify(self, flag_value, expected_output_re, other_flag):
    """Runs gtest_list_tests_unittest_ and verifies that it prints
    the correct tests.

    Args:
      flag_value:         value of the --gtest_list_tests flag;
                          None if the flag should not be present.
      expected_output_re: regular expression that matches the expected
                          output after running command;
      other_flag:         a different flag to be passed to command
                          along with gtest_list_tests;
                          None if the flag should not be present.
    """

    if flag_value is None:
      flag = ''
      flag_expression = 'not set'
    elif flag_value == '0':
      flag = '--%s=0' % LIST_TESTS_FLAG
      flag_expression = '0'
    else:
      flag = '--%s' % LIST_TESTS_FLAG
      flag_expression = '1'

    args = [flag]

    if other_flag is not None:
      args += [other_flag]

    output = Run(args)

    if expected_output_re:
      self.assert_(
          expected_output_re.match(output),
          ('when %s is %s, the output of "%s" is "%s",\n'
           'which does not match regex "%s"' %
           (LIST_TESTS_FLAG, flag_expression, ' '.join(args), output,
            expected_output_re.pattern)))
    else:
      self.assert_(
          not EXPECTED_OUTPUT_NO_FILTER_RE.match(output),
          ('when %s is %s, the output of "%s" is "%s"'%
           (LIST_TESTS_FLAG, flag_expression, ' '.join(args), output)))

  def testDefaultBehavior(self):
    """Tests the behavior of the default mode."""

    self.RunAndVerify(flag_value=None,
                      expected_output_re=None,
                      other_flag=None)

  def testFlag(self):
    """Tests using the --gtest_list_tests flag."""

    self.RunAndVerify(flag_value='0',
                      expected_output_re=None,
                      other_flag=None)
    self.RunAndVerify(flag_value='1',
                      expected_output_re=EXPECTED_OUTPUT_NO_FILTER_RE,
                      other_flag=None)

  def testOverrideNonFilterFlags(self):
    """Tests that --gtest_list_tests overrides the non-filter flags."""

    self.RunAndVerify(flag_value='1',
                      expected_output_re=EXPECTED_OUTPUT_NO_FILTER_RE,
                      other_flag='--gtest_break_on_failure')

  def testWithFilterFlags(self):
    """Tests that --gtest_list_tests takes into account the
    --gtest_filter flag."""

    self.RunAndVerify(flag_value='1',
                      expected_output_re=EXPECTED_OUTPUT_FILTER_FOO_RE,
                      other_flag='--gtest_filter=Foo*')


if __name__ == '__main__':
  gtest_test_utils.Main()

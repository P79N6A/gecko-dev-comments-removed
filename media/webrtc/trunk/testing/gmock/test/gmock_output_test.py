






























"""Tests the text output of Google C++ Mocking Framework.

SYNOPSIS
       gmock_output_test.py --build_dir=BUILD/DIR --gengolden
         # where BUILD/DIR contains the built gmock_output_test_ file.
       gmock_output_test.py --gengolden
       gmock_output_test.py
"""

__author__ = 'wan@google.com (Zhanyong Wan)'

import os
import re
import sys

import gmock_test_utils



GENGOLDEN_FLAG = '--gengolden'

PROGRAM_PATH = gmock_test_utils.GetTestExecutablePath('gmock_output_test_')
COMMAND = [PROGRAM_PATH, '--gtest_stack_trace_depth=0', '--gtest_print_time=0']
GOLDEN_NAME = 'gmock_output_test_golden.txt'
GOLDEN_PATH = os.path.join(gmock_test_utils.GetSourceDir(), GOLDEN_NAME)


def ToUnixLineEnding(s):
  """Changes all Windows/Mac line endings in s to UNIX line endings."""

  return s.replace('\r\n', '\n').replace('\r', '\n')


def RemoveReportHeaderAndFooter(output):
  """Removes Google Test result report's header and footer from the output."""

  output = re.sub(r'.*gtest_main.*\n', '', output)
  output = re.sub(r'\[.*\d+ tests.*\n', '', output)
  output = re.sub(r'\[.* test environment .*\n', '', output)
  output = re.sub(r'\[=+\] \d+ tests .* ran.*', '', output)
  output = re.sub(r'.* FAILED TESTS\n', '', output)
  return output


def RemoveLocations(output):
  """Removes all file location info from a Google Test program's output.

  Args:
       output:  the output of a Google Test program.

  Returns:
       output with all file location info (in the form of
       'DIRECTORY/FILE_NAME:LINE_NUMBER: 'or
       'DIRECTORY\\FILE_NAME(LINE_NUMBER): ') replaced by
       'FILE:#: '.
  """

  return re.sub(r'.*[/\\](.+)(\:\d+|\(\d+\))\:', 'FILE:#:', output)


def NormalizeErrorMarker(output):
  """Normalizes the error marker, which is different on Windows vs on Linux."""

  return re.sub(r' error: ', ' Failure\n', output)


def RemoveMemoryAddresses(output):
  """Removes memory addresses from the test output."""

  return re.sub(r'@\w+', '@0x#', output)


def RemoveTestNamesOfLeakedMocks(output):
  """Removes the test names of leaked mock objects from the test output."""

  return re.sub(r'\(used in test .+\) ', '', output)


def GetLeakyTests(output):
  """Returns a list of test names that leak mock objects."""

  
  
  
  return re.findall(r'\(used in test (.+)\)', output)


def GetNormalizedOutputAndLeakyTests(output):
  """Normalizes the output of gmock_output_test_.

  Args:
    output: The test output.

  Returns:
    A tuple (the normalized test output, the list of test names that have
    leaked mocks).
  """

  output = ToUnixLineEnding(output)
  output = RemoveReportHeaderAndFooter(output)
  output = NormalizeErrorMarker(output)
  output = RemoveLocations(output)
  output = RemoveMemoryAddresses(output)
  return (RemoveTestNamesOfLeakedMocks(output), GetLeakyTests(output))


def GetShellCommandOutput(cmd):
  """Runs a command in a sub-process, and returns its STDOUT in a string."""

  return gmock_test_utils.Subprocess(cmd, capture_stderr=False).output


def GetNormalizedCommandOutputAndLeakyTests(cmd):
  """Runs a command and returns its normalized output and a list of leaky tests.

  Args:
    cmd:  the shell command.
  """

  
  os.environ['GTEST_CATCH_EXCEPTIONS'] = '1'
  return GetNormalizedOutputAndLeakyTests(GetShellCommandOutput(cmd))


class GMockOutputTest(gmock_test_utils.TestCase):
  def testOutput(self):
    (output, leaky_tests) = GetNormalizedCommandOutputAndLeakyTests(COMMAND)
    golden_file = open(GOLDEN_PATH, 'rb')
    golden = golden_file.read()
    golden_file.close()

    
    self.assertEquals(golden, output)

    
    
    self.assertEquals(['GMockOutputTest.CatchesLeakedMocks',
                       'GMockOutputTest.CatchesLeakedMocks'],
                      leaky_tests)


if __name__ == '__main__':
  if sys.argv[1:] == [GENGOLDEN_FLAG]:
    (output, _) = GetNormalizedCommandOutputAndLeakyTests(COMMAND)
    golden_file = open(GOLDEN_PATH, 'wb')
    golden_file.write(output)
    golden_file.close()
  else:
    gmock_test_utils.Main()

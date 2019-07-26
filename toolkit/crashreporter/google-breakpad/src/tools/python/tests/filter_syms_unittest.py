





























"""Unit tests for filter_syms.py"""

import cStringIO
import ntpath
import os
import StringIO
import sys
import unittest

ROOT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ROOT_DIR, '..'))


import filter_syms

class FilterSysmsTest(unittest.TestCase):
  def assertParsed(self, input_data, ignored_prefixes, expected):
    input_io = cStringIO.StringIO(input_data)
    output_io = cStringIO.StringIO()
    parser = filter_syms.SymbolFileParser(input_io, output_io,
                                          ignored_prefixes, ntpath)
    parser.Process()
    self.assertEqual(output_io.getvalue(), expected)
    
  def testDuplicateFiles(self):
    """Tests that duplicate files in FILE records are correctly removed and
    that Line records are updated."""

    INPUT = \
"""MODULE windows x86 111111111111111111111111111111111 module1.pdb
INFO CODE_ID FFFFFFFF module1.exe
FILE 1 foo/../file1_1.cc
FILE 2 bar/../file1_1.cc
FILE 3 baz/../file1_1.cc
FUNC 1000 c 0 Function1_1
1000 8 45 2
1008 4 46 3
100c 4 44 1
"""
    EXPECTED_OUTPUT = \
"""MODULE windows x86 111111111111111111111111111111111 module1.pdb
INFO CODE_ID FFFFFFFF module1.exe
FILE 1 file1_1.cc
FUNC 1000 c 0 Function1_1
1000 8 45 1
1008 4 46 1
100c 4 44 1
"""
    self.assertParsed(INPUT, [], EXPECTED_OUTPUT)

  def testIgnoredPrefix(self):
    """Tests that prefixes in FILE records are correctly removed."""

    INPUT = \
"""MODULE windows x86 111111111111111111111111111111111 module1.pdb
INFO CODE_ID FFFFFFFF module1.exe
FILE 1 /src/build/foo/../file1_1.cc
FILE 2 /src/build/bar/../file1_2.cc
FILE 3 /src/build/baz/../file1_2.cc
FUNC 1000 c 0 Function1_1
1000 8 45 2
1008 4 46 3
100c 4 44 1
"""
    EXPECTED_OUTPUT = \
"""MODULE windows x86 111111111111111111111111111111111 module1.pdb
INFO CODE_ID FFFFFFFF module1.exe
FILE 1 file1_1.cc
FILE 2 file1_2.cc
FUNC 1000 c 0 Function1_1
1000 8 45 2
1008 4 46 2
100c 4 44 1
"""
    IGNORED_PREFIXES = ['\\src\\build\\']
    self.assertParsed(INPUT, IGNORED_PREFIXES, EXPECTED_OUTPUT)

  def testIgnoredPrefixesDuplicateFiles(self):
    """Tests that de-duplication of FILE records happens BEFORE prefixes
    are removed."""

    INPUT = \
"""MODULE windows x86 111111111111111111111111111111111 module1.pdb
INFO CODE_ID FFFFFFFF module1.exe
FILE 1 /src/build/foo/../file1_1.cc
FILE 2 /src/build/bar/../file1_2.cc
FILE 3 D:/src/build2/baz/../file1_2.cc
FUNC 1000 c 0 Function1_1
1000 8 45 2
1008 4 46 3
100c 4 44 1
"""
    EXPECTED_OUTPUT = \
"""MODULE windows x86 111111111111111111111111111111111 module1.pdb
INFO CODE_ID FFFFFFFF module1.exe
FILE 1 file1_1.cc
FILE 2 file1_2.cc
FILE 3 file1_2.cc
FUNC 1000 c 0 Function1_1
1000 8 45 2
1008 4 46 3
100c 4 44 1
"""
    IGNORED_PREFIXES = ['\\src\\build\\', 'D:\\src\\build2\\']
    self.assertParsed(INPUT, IGNORED_PREFIXES, EXPECTED_OUTPUT)

if __name__ == '__main__':
  unittest.main()
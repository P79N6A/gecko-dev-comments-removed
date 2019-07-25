
















"""Tests for gmock.scripts.generator.cpp.gmock_class."""

__author__ = 'nnorwitz@google.com (Neal Norwitz)'


import os
import sys
import unittest


sys.path.append(os.path.join(os.path.dirname(__file__), '..'))

from cpp import ast
from cpp import gmock_class


class TestCase(unittest.TestCase):
  """Helper class that adds assert methods."""

  def StripLeadingWhitespace(self, lines):
    """Strip leading whitespace in each line in 'lines'."""
    return '\n'.join([s.lstrip() for s in lines.split('\n')])

  def assertEqualIgnoreLeadingWhitespace(self, expected_lines, lines):
    """Specialized assert that ignores the indent level."""
    self.assertEqual(expected_lines, self.StripLeadingWhitespace(lines))


class GenerateMethodsTest(TestCase):

  def GenerateMethodSource(self, cpp_source):
    """Convert C++ source to Google Mock output source lines."""
    method_source_lines = []
    
    builder = ast.BuilderFromSource(cpp_source, '<test>')
    ast_list = list(builder.Generate())
    gmock_class._GenerateMethods(method_source_lines, cpp_source, ast_list[0])
    return '\n'.join(method_source_lines)

  def testSimpleMethod(self):
    source = """
class Foo {
 public:
  virtual int Bar();
};
"""
    self.assertEqualIgnoreLeadingWhitespace(
        'MOCK_METHOD0(Bar,\nint());',
        self.GenerateMethodSource(source))

  def testSimpleConstMethod(self):
    source = """
class Foo {
 public:
  virtual void Bar(bool flag) const;
};
"""
    self.assertEqualIgnoreLeadingWhitespace(
        'MOCK_CONST_METHOD1(Bar,\nvoid(bool flag));',
        self.GenerateMethodSource(source))

  def testStrangeNewlineInParameter(self):
    source = """
class Foo {
 public:
  virtual void Bar(int
a) = 0;
};
"""
    self.assertEqualIgnoreLeadingWhitespace(
        'MOCK_METHOD1(Bar,\nvoid(int a));',
        self.GenerateMethodSource(source))

  def testDoubleSlashCommentsInParameterListAreRemoved(self):
    source = """
class Foo {
 public:
  virtual void Bar(int a,  // inline comments should be elided.
                   int b   // inline comments should be elided.
                   ) const = 0;
};
"""
    self.assertEqualIgnoreLeadingWhitespace(
        'MOCK_CONST_METHOD2(Bar,\nvoid(int a, int b));',
        self.GenerateMethodSource(source))

  def testCStyleCommentsInParameterListAreNotRemoved(self):
    
    
    
    source = """
class Foo {
 public:
  virtual const string& Bar(int /* keeper */, int b);
};
"""
    self.assertEqualIgnoreLeadingWhitespace(
        'MOCK_METHOD2(Bar,\nconst string&(int /* keeper */, int b));',
        self.GenerateMethodSource(source))

  def testArgsOfTemplateTypes(self):
    source = """
class Foo {
 public:
  virtual int Bar(const vector<int>& v, map<int, string>* output);
};"""
    self.assertEqualIgnoreLeadingWhitespace(
        'MOCK_METHOD2(Bar,\n'
        'int(const vector<int>& v, map<int, string>* output));',
        self.GenerateMethodSource(source))

  def testReturnTypeWithOneTemplateArg(self):
    source = """
class Foo {
 public:
  virtual vector<int>* Bar(int n);
};"""
    self.assertEqualIgnoreLeadingWhitespace(
        'MOCK_METHOD1(Bar,\nvector<int>*(int n));',
        self.GenerateMethodSource(source))

  def testReturnTypeWithManyTemplateArgs(self):
    source = """
class Foo {
 public:
  virtual map<int, string> Bar();
};"""
    
    
    self.assertEqualIgnoreLeadingWhitespace(
        '// The following line won\'t really compile, as the return\n'
        '// type has multiple template arguments.  To fix it, use a\n'
        '// typedef for the return type.\n'
        'MOCK_METHOD0(Bar,\nmap<int, string>());',
        self.GenerateMethodSource(source))


class GenerateMocksTest(TestCase):

  def GenerateMocks(self, cpp_source):
    """Convert C++ source to complete Google Mock output source."""
    
    filename = '<test>'
    builder = ast.BuilderFromSource(cpp_source, filename)
    ast_list = list(builder.Generate())
    lines = gmock_class._GenerateMocks(filename, cpp_source, ast_list, None)
    return '\n'.join(lines)

  def testNamespaces(self):
    source = """
namespace Foo {
namespace Bar { class Forward; }
namespace Baz {

class Test {
 public:
  virtual void Foo();
};

}  // namespace Baz
}  // namespace Foo
"""
    expected = """\
namespace Foo {
namespace Baz {

class MockTest : public Test {
public:
MOCK_METHOD0(Foo,
void());
};

}  // namespace Baz
}  // namespace Foo
"""
    self.assertEqualIgnoreLeadingWhitespace(
        expected, self.GenerateMocks(source))

  def testClassWithStorageSpecifierMacro(self):
    source = """
class STORAGE_SPECIFIER Test {
 public:
  virtual void Foo();
};
"""
    expected = """\
class MockTest : public Test {
public:
MOCK_METHOD0(Foo,
void());
};
"""
    self.assertEqualIgnoreLeadingWhitespace(
        expected, self.GenerateMocks(source))

if __name__ == '__main__':
  unittest.main()

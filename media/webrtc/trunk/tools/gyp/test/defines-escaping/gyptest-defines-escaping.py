





"""
Verifies build of an executable with C++ define specified by a gyp define using
various special characters such as quotes, commas, etc.
"""

import os
import TestGyp

test = TestGyp.TestGyp()


try:
  os.environ['GYP_DEFINES'] = (
      r"""test_format='\n%s\n' """
      r"""test_args='"Simple test of %s with a literal"'""")
  test.run_gyp('defines-escaping.gyp')
finally:
  del os.environ['GYP_DEFINES']

test.build('defines-escaping.gyp')

expect = """
Simple test of %s with a literal
"""
test.run_built_executable('defines_escaping', stdout=expect)



try:
  os.environ['GYP_DEFINES'] = \
      r"""test_format='\n%s and %s\n' test_args='"foo", "bar"'"""
  test.run_gyp('defines-escaping.gyp')
finally:
  del os.environ['GYP_DEFINES']

test.sleep()
test.touch('defines-escaping.c')
test.build('defines-escaping.gyp')

expect = """
foo and bar
"""
test.run_built_executable('defines_escaping', stdout=expect)



try:
  os.environ['GYP_DEFINES'] = (
      r"""test_format='\n%s %s %s %s %s\n' """
      r"""test_args='"\"These,\"","""
                r""" "\"words,\"","""
                r""" "\"are,\"","""
                r""" "\"in,\"","""
                r""" "\"quotes.\""'""")
  test.run_gyp('defines-escaping.gyp')
finally:
  del os.environ['GYP_DEFINES']

test.sleep()
test.touch('defines-escaping.c')
test.build('defines-escaping.gyp')

expect = """
"These," "words," "are," "in," "quotes."
"""
test.run_built_executable('defines_escaping', stdout=expect)



try:
  os.environ['GYP_DEFINES'] = (
      r"""test_format='\n%s %s %s %s %s\n' """
      r"""test_args="\"'These,'\","""
                r""" \"'words,'\","""
                r""" \"'are,'\","""
                r""" \"'in,'\","""
                r""" \"'quotes.'\"" """)
  test.run_gyp('defines-escaping.gyp')
finally:
  del os.environ['GYP_DEFINES']

test.sleep()
test.touch('defines-escaping.c')
test.build('defines-escaping.gyp')

expect = """
'These,' 'words,' 'are,' 'in,' 'quotes.'
"""
test.run_built_executable('defines_escaping', stdout=expect)




try:
  os.environ['GYP_DEFINES'] = (
      r"""test_format='\n%s\n%s\n%s\n' """
      r"""test_args='"\\\"1 visible slash\\\"","""
                r""" "\\\\\"2 visible slashes\\\\\"","""
                r""" "\\\\\\\"3 visible slashes\\\\\\\""'""")
  test.run_gyp('defines-escaping.gyp')
finally:
  del os.environ['GYP_DEFINES']

test.sleep()
test.touch('defines-escaping.c')
test.build('defines-escaping.gyp')

expect = r"""
\"1 visible slash\"
\\"2 visible slashes\\"
\\\"3 visible slashes\\\"
"""
test.run_built_executable('defines_escaping', stdout=expect)



try:
  os.environ['GYP_DEFINES'] = (
      r"""test_format='\n%s\n' """
      r"""test_args='"$foo, &quot; `foo`;"'""")
  test.run_gyp('defines-escaping.gyp')
finally:
  del os.environ['GYP_DEFINES']

test.sleep()
test.touch('defines-escaping.c')
test.build('defines-escaping.gyp')

expect = """
$foo, &quot; `foo`;
"""
test.run_built_executable('defines_escaping', stdout=expect)



if not (test.format == 'msvs' and test.uses_msbuild):
  try:
    os.environ['GYP_DEFINES'] = (
        """test_format='%s' """
        """test_args='"%PATH%"'""")
    test.run_gyp('defines-escaping.gyp')
  finally:
    del os.environ['GYP_DEFINES']

  test.sleep()
  test.touch('defines-escaping.c')
  test.build('defines-escaping.gyp')

  expect = "%PATH%"
  test.run_built_executable('defines_escaping', stdout=expect)




try:
  os.environ['GYP_DEFINES'] = (
      r"""test_format='\n%s\n%s\n' """
      r"""test_args='"\\, \\\\;","""
                
                r""" "\"\\, \\\\;\""'""")
  test.run_gyp('defines-escaping.gyp')
finally:
  del os.environ['GYP_DEFINES']

test.sleep()
test.touch('defines-escaping.c')
test.build('defines-escaping.gyp')

expect = r"""
\, \\;
"\, \\;"
"""
test.run_built_executable('defines_escaping', stdout=expect)




test.pass_test()

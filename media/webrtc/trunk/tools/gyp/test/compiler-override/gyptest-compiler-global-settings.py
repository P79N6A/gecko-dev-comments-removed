



"""
Verifies that make_global_settings can be used to override the
compiler settings.
"""

import TestGyp
import os
import copy
import sys
from string import Template


if sys.platform == 'win32':
  
  
  sys.exit(0)

test = TestGyp.TestGyp(formats=['ninja', 'make'])

gypfile = 'compiler-global-settings.gyp'

replacements = { 'PYTHON': '/usr/bin/python', 'PWD': os.getcwd()}




replacements['TOOLSET'] = 'target'
s = Template(open(gypfile + '.in').read())
output = open(gypfile, 'w')
output.write(s.substitute(replacements))
output.close()

test.run_gyp(gypfile)
test.build(gypfile)
test.must_contain_all_lines(test.stdout(), ['my_cc.py', 'my_cxx.py', 'FOO'])


replacements['TOOLSET'] = 'host'
s = Template(open(gypfile + '.in').read())
output = open(gypfile, 'w')
output.write(s.substitute(replacements))
output.close()

test.run_gyp(gypfile)
test.build(gypfile)
test.must_contain_all_lines(test.stdout(), ['my_cc.py', 'my_cxx.py', 'BAR'])

test.pass_test()

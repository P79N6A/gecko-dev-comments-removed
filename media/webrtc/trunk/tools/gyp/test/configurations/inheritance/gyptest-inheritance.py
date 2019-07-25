





"""
Verifies build of an executable in three different configurations.
"""

import TestGyp

test = TestGyp.TestGyp()

test.run_gyp('configurations.gyp')

test.set_configuration('Release')
test.build('configurations.gyp')
test.run_built_executable('configurations',
                          stdout=('Base configuration\n'
                                  'Common configuration\n'
                                  'Common2 configuration\n'
                                  'Release configuration\n'))

test.set_configuration('Debug')
test.build('configurations.gyp')
test.run_built_executable('configurations',
                          stdout=('Base configuration\n'
                                  'Common configuration\n'
                                  'Common2 configuration\n'
                                  'Debug configuration\n'))

test.pass_test()







"""
Verify that relinking a solib doesn't relink a dependent executable if the
solib's public API hasn't changed.
"""

import os
import sys
import TestCommon
import TestGyp







test = TestGyp.TestGyp(formats=['ninja'])

test.run_gyp('solibs_avoid_relinking.gyp')



test.build('solibs_avoid_relinking.gyp', 'b')
test.built_file_must_exist('b' + TestCommon.exe_suffix)
pre_stat = os.stat(test.built_file_path('b' + TestCommon.exe_suffix))
os.utime(os.path.join(test.workdir, 'solib.cc'),
         (pre_stat.st_atime, pre_stat.st_mtime + 100))
test.sleep()
test.build('solibs_avoid_relinking.gyp', 'b')
post_stat = os.stat(test.built_file_path('b' + TestCommon.exe_suffix))

if pre_stat.st_mtime != post_stat.st_mtime:
  test.fail_test()
else:
  test.pass_test()





import glob
import os
import re
import sys
import subprocess





testcase_name = None
for arg in sys.argv:
  m = re.match("\-\-test\-name=(.*)", arg)
  if m:
    assert testcase_name is None
    testcase_name = m.groups()[0]


cmd_to_run = sys.argv[1:]






logdir_idx = cmd_to_run.index("-logdir")
old_logdir = cmd_to_run[logdir_idx + 1]

wrapper_pid = str(os.getpid())





wrapper_pid += "_%d" % len(glob.glob(old_logdir + "\\*"))

cmd_to_run[logdir_idx + 1] += "\\testcase.%s.logs" % wrapper_pid
os.makedirs(cmd_to_run[logdir_idx + 1])

if testcase_name:
  f = open(old_logdir + "\\testcase.%s.name" % wrapper_pid, "w")
  print >>f, testcase_name
  f.close()

exit(subprocess.call(cmd_to_run))

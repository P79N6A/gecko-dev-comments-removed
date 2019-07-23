





































"""Runs extension performance tests.

   This file runs Ts (startup time) and Tp (page load time) tests
   for an extension with different profiles.  It was originally
   written for the Google Toolbar for Firefox; to make it work for
   another extension modify get_xpi.py.  To change the preferences
   that are set on the profiles that are tested, edit the arrays in
   the main function below.
"""

__author__ = 'annie.sullivan@gmail.com (Annie Sullivan)'


import time
import syck
import sys

import report
import paths
import tp
import ts



TS_NUM_RUNS = 5


TP_NUM_CYCLES = 7



TP_RESOLUTION = 1


def test_file(filename):
  """Runs the Ts and Tp tests on the given config file and generates a report.
  
  Args:
    filename: the name of the file to run the tests on
  """
  
  test_configs = []
  test_names = []
  title = ''
  filename_prefix = ''
  
  
  config_file = open(filename, 'r')
  yaml = syck.load(config_file)
  config_file.close()
  for item in yaml:
    if item == 'title':
      title = yaml[item]
    elif item == 'filename':
      filename_prefix = yaml[item]
    else:
      new_config = [yaml[item]['preferences'],
                    yaml[item]['extensions'],
                    yaml[item]['firefox']]
      test_configs.append(new_config)
      test_names.append(item)
  config_file.close()
  
  
  ts_times = ts.RunStartupTests(paths.BASE_PROFILE_DIR,
                                test_configs,
                                TS_NUM_RUNS)
  
  
  
  (tp_times, tp_counters) = tp.RunPltTests(paths.BASE_PROFILE_DIR,
                                           test_configs,
                                           TP_NUM_CYCLES,
                                           ['Private Bytes', 'Working Set', '% Processor Time'],
                                           TP_RESOLUTION)
  
  
  report.GenerateReport(title,
                        filename_prefix,
                        test_names,
                        ts_times,
                        tp_times,
                        tp_counters,
                        TP_RESOLUTION)


if __name__=='__main__':

  
  for i in range(1, len(sys.argv)):
    test_file(sys.argv[i])


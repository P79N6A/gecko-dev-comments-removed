




































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
import urllib 
import tempfile
import os
import string
import socket
socket.setdefaulttimeout(480)

import config
import post_file
import tp
import ts

def shortNames(name):
  if name == "tp_loadtime":
    return "tp_l"
  elif name == "tp_Percent Processor Time":
    return "tp_%cpu"
  elif name == "tp_Working Set":
    return "tp_memset"
  elif name == "tp_Private Bytes":
    return "tp_pbytes"
  else:
    return name

def process_Request(post):
  str = ""
  lines = post.split('\n')
  for line in lines:
    if line.find("RETURN:") > -1:
        str += line.rsplit(":")[3] + ":" + shortNames(line.rsplit(":")[1]) + ":" + line.rsplit(":")[2] + '\n'
  return str

def test_file(filename):
  """Runs the Ts and Tp tests on the given config file and generates a report.
  
  Args:
    filename: the name of the file to run the tests on
  """
  
  test_configs = []
  test_names = []
  title = ''
  filename_prefix = ''
  testdate = ''
  
  
  config_file = open(filename, 'r')
  
  try:
    yaml = syck.load(config_file)
  except:
    yaml = syck.load("".join(config_file.readlines()))
  config_file.close()
  for item in yaml:
    if item == 'title':
      title = yaml[item]
    elif item == 'filename':
      filename_prefix = yaml[item]
    elif item == 'testdate':
      testdate = yaml[item]
    else:
      new_config = [yaml[item]['preferences'],
                    yaml[item]['extensions'],
                    yaml[item]['firefox'],
                    yaml[item]['branch'],
                    yaml[item]['branchid'],
                    yaml[item]['profile_path']]
      test_configs.append(new_config)
      test_names.append(item)
  config_file.close()

  print test_configs
  sys.stdout.flush()
  if (testdate != ''):
    date = int(time.mktime(time.strptime(testdate, '%a, %d %b %Y %H:%M:%S GMT')))
  else:
    date = int(time.time()) 
  print "using testdate: %d" % date
  print "actual date: %d" % int(time.time())
 

  
  ts_times = ts.RunStartupTests(test_configs,
                                config.TS_NUM_RUNS)

  print "finished ts"
  sys.stdout.flush()
  for ts_set in ts_times:
    if len(ts_set) == 0:
	print "FAIL:no ts results, build failed to run:BAD BUILD"
	sys.exit(0)

  (res, r_strings, tp_times, tp_counters) = tp.RunPltTests(test_configs,
                                           config.TP_NUM_CYCLES,
                                           config.COUNTERS,
                                           config.TP_RESOLUTION)

  print "finished tp"
  sys.stdout.flush()

  if not res:
    print "FAIL:tp did not run to completion"
    print "FAIL:" + r_strings[0]
    sys.exit(0)

  
  
  
  tbox = title
  url_format = "http://%s/%s"
  link_format= "<a href = \"%s\">%s</a>"
  
  result_format = "%.2f,%s,%s,%d,%d,%s,%s,%s,%s,\n"
  result_format2 = "%.2f,%s,%s,%d,%d,%s,%s,%s,\n"
  filename = tempfile.mktemp()
  tmpf = open(filename, "w")

  testname = "ts"
  print "formating results for: ts"
  print "# of values: %d" % len(ts_times)
  for index in range(len(ts_times)):
    i = 0
    for tstime in ts_times[index]:
      tmpf.write(result_format % (float(tstime), testname, tbox, i, date, test_configs[index][3], test_configs[index][4], "discrete", "ms"))
      i = i+1

  testname = "tp"
  for index in range(len(r_strings)):
    r_strings[index].strip('\n')
    page_results = r_strings[index].splitlines()
    i = 0
    print "formating results for: loadtime"
    print "# of values: %d" % len(page_results)
    for mypage in page_results[3:]:
      r = mypage.split(';')
      tmpf.write(result_format % (float(r[2]), testname + "_loadtime", tbox, i, date, test_configs[index][3], test_configs[index][4], "discrete", r[1]))
      i = i+1

  for index in range(len(tp_counters)):
    for count_type in config.COUNTERS:
      i = 0
      print "formating results for: " + count_type
      print "# of values: %d" % len(tp_counters[index][count_type])
      for value in tp_counters[index][count_type]:
        tmpf.write(result_format2 % (float(value), testname + "_" + count_type.replace("%", "Percent"), tbox, i, date, test_configs[index][3], test_configs[index][4], "discrete"))
        i = i+1


  print "finished formating results"
  tmpf.flush()
  tmpf.close()
  tmpf = open(filename, "r")
  file_data = tmpf.read()
  while True:
    try:
      ret = post_file.post_multipart(config.RESULTS_SERVER, config.RESULTS_LINK, [("key", "value")], [("filename", filename, file_data)
   ])
    except IOError:
      print "IOError"
    else:
      break
  print "completed sending results"
  links = process_Request(ret)
  tmpf.close()
  os.remove(filename)

  lines = links.split('\n')
  for line in lines:
    if line == "":
      continue
    values = line.split(":")
    linkName = values[1]
    if float(values[2]) > 0:
      linkName += "_T: " + values[2]
    else:
      linkName += "_1"
    url = url_format % (config.RESULTS_SERVER, values[0],)
    link = link_format % (url, linkName,)
    print "RETURN: " + link

  
if __name__=='__main__':

  
  for i in range(1, len(sys.argv)):
    test_file(sys.argv[i])


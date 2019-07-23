




































"""A set of functions to run the Tp test.

   The Tp test measures page load times in Firefox.  It does this with a
   JavaScript script that opens a new window and cycles through page loads
   from the local disk, timing each one.  The script dumps the sum of the
   mean times to open each page, and the standard deviation, to standard out.
   We can also measure performance attributes during the test.  See
   http://technet2.microsoft.com/WindowsServer/en/Library/86b5d116-6fb3-427b-af8c-9077162125fe1033.mspx?mfr=true
   for a description of what can be measured.
"""

__author__ = 'annie.sullivan@gmail.com (Annie Sullivan)'


import os
import re
import shutil
import time
import sys

import ffprocess
import ffprofile
import ffinfo
import config

if config.OS == "linux":
    from tp_linux import *
elif config.OS == "win32":
    from tp_win32 import *



TP_REGEX = re.compile('__start_page_load_report(.*)__end_page_load_report',
                      re.DOTALL | re.MULTILINE)
TP_REGEX_FAIL = re.compile('__FAIL(.*)__FAIL', re.DOTALL|re.MULTILINE)


def RunPltTests(profile_configs,
                num_cycles,
                counters,
                resolution):
  """Runs the Page Load tests with profiles created from the given
     base diectory and list of configuations.
  
  Args:
    profile_configs:  Array of configuration options for each profile.
      These are of the format:
      [{prefname:prevalue,prefname2:prefvalue2},
       {extensionguid:path_to_extension}],[{prefname...
    num_cycles: Number of times to cycle through all the urls on each test.
    counters: Array of counters ("% Processor Time", "Working Set", etc)
              See http://technet2.microsoft.com/WindowsServer/en/Library/86b5d116-6fb3-427b-af8c-9077162125fe1033.mspx?mfr=true
              for a list of available counters and their descriptions.
    resolution: Time (in seconds) between collecting counters
  
  Returns:
    A tuple containing:
      An array of plt results for each run.  For example:
        ["mean: 150.30\nstdd:34.2", "mean 170.33\nstdd:22.4"]
      An array of counter data from each run.  For example:
        [{"counter1": [1, 2, 3], "counter2":[4,5,6]},
         {"counter1":[1,3,5], "counter2":[2,4,6]}]
  """
 
  res = 0
  counter_data = []
  plt_results = []
  results_string = []
  for pconfig in profile_configs:
    print "in tp"
    print pconfig
    sys.stdout.flush()
    rstring = ""
    
    profile_dir = ffprofile.CreateTempProfileDir(pconfig[5],
                                                 pconfig[0],
                                                 pconfig[1])
    print "created profile" 
    
    
    
    ffprofile.InitializeNewProfile(pconfig[2], profile_dir)
    ffinfo.GetMetricsFromBrowser(pconfig[2], profile_dir)
    print "initialized firefox"
    sys.stdout.flush()
    ffprocess.SyncAndSleep()

    
    timeout = 10000
    total_time = 0
    output = ''
    url = config.TP_URL + '?cycles=' + str(num_cycles)
    command_line = ffprocess.GenerateFirefoxCommandLine(pconfig[2], profile_dir, url)
    handle = os.popen(command_line)
    
    time.sleep(1)

    cm = CounterManager("firefox", counters)

    cm.startMonitor()

    counts = {}
    for counter in counters:
      counts[counter] = []
    
    while total_time < timeout:
    
      
      time.sleep(resolution)
      total_time += resolution
      
      
      for count_type in counters:
        val = cm.getCounterValue(count_type)

        if (val):
          counts[count_type].append(val)

      
      (bytes, current_output) = ffprocess.NonBlockingReadProcessOutput(handle)
      output += current_output
      match = TP_REGEX.search(output)
      if match:
        rstring += match.group(1)
        plt_results.append(match.group(1))
	res = 1
        break
      match = TP_REGEX_FAIL.search(output)
      if match:
        rstring += match.group(1)
        plt_results.append(match.group(1))
        break

    cm.stopMonitor()
   
    print "got tp results from browser" 

    
    
    time.sleep(2)
    ffprocess.TerminateAllProcesses("firefox")
    ffprocess.SyncAndSleep()
    
    
    
    
    ffprofile.MakeDirectoryContentsWritable(profile_dir)
    shutil.rmtree(profile_dir)
    
    counter_data.append(counts)
    results_string.append(rstring)

  return (res, results_string, plt_results, counter_data)







































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
import win32pdh
import win32pdhutil

import ffprocess
import ffprofile
import paths



TP_REGEX = re.compile('__start_page_load_report(.*)__end_page_load_report',
                      re.DOTALL | re.MULTILINE)


def AddCounter(counter_name):
  """Adds a pdh query and counter of the given name to Firefox.
  
  Args: counter_name: The name of the counter to add, i.e. "% Processor Time"
  
  Returns:
    (query handle, counter handle)
  """
  
  path = win32pdh.MakeCounterPath( (None,
                                    'process',
                                    'firefox',
                                    None,
                                    -1,
                                    counter_name) )
  hq = win32pdh.OpenQuery()
  try:
    hc = win32pdh.AddCounter(hq, path)
  except:
    win32pdh.CloseQuery(hq)
  return hq, hc


def CleanupCounter(hq, hc):
  """Cleans up a counter after it is no longer needed.
  
  Args:
    hq: handle to the query for the counter
    hc: handle to the counter
  """
  
  try:
    win32pdh.RemoveCounter(hc)
    win32pdh.CloseQuery(hq)
  except:
    
    pass


def GetCounterValue(hq, hc):
  """Returns the current value of the given counter
  
  Args:
    hq: Handle of the query for the counter
    hc: Handle of the counter
  
  Returns:
    The current value of the counter
  """
  
  try:
    win32pdh.CollectQueryData(hq)
    type, val = win32pdh.GetFormattedCounterValue(hc, win32pdh.PDH_FMT_LONG)
    return val
  except:
    return None


def RunPltTests(source_profile_dir,
                profile_configs,
                num_cycles,
                counters,
                resolution):
  """Runs the Page Load tests with profiles created from the given
     base diectory and list of configuations.
  
  Args:
    source_profile_dir:  Full path to base directory to copy profile from.
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
  
  counter_data = []
  plt_results = []
  for config in profile_configs:
    
    profile_dir = ffprofile.CreateTempProfileDir(source_profile_dir,
                                                 config[0],
                                                 config[1])
  
    
    
    
    ffprofile.InitializeNewProfile(config[2], profile_dir)
    ffprocess.SyncAndSleep()

    
    timeout = 300
    total_time = 0
    output = ''
    url = paths.TP_URL + '?cycles=' + str(num_cycles)
    command_line = ffprocess.GenerateFirefoxCommandLine(config[2], profile_dir, url)
    handle = os.popen(command_line)
    
    
    win32pdh.EnumObjects(None, None, 0, 1)
    
    
    counts = {}
    counter_handles = {}
    for counter in counters:
      counts[counter] = []
      counter_handles[counter] = AddCounter(counter)
    
    while total_time < timeout:
    
      
      time.sleep(resolution)
      total_time += resolution
      
      
      for count_type in counters:
        val = GetCounterValue(counter_handles[count_type][0],
                              counter_handles[count_type][1])
        if (val):
          
          
          counts[count_type].append(val)

      
      (bytes, current_output) = ffprocess.NonBlockingReadProcessOutput(handle)
      output += current_output
      match = TP_REGEX.search(output)
      if match:
        plt_results.append(match.group(1))
        break;
    
    
    for counter in counters:
      CleanupCounter(counter_handles[counter][0], counter_handles[counter][1])

    
    
    time.sleep(2)
    ffprocess.TerminateAllProcesses("firefox")
    ffprocess.SyncAndSleep()
    
    
    
    
    ffprofile.MakeDirectoryContentsWritable(profile_dir)
    shutil.rmtree(profile_dir)
    
    counter_data.append(counts)
  
  return (plt_results, counter_data)

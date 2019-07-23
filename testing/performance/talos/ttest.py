





































"""A generic means of running an URL based browser test
   follows the following steps
     - creates a profile
     - tests the profile
     - gets metrics for the current test environment
     - loads the url
     - collects info on any counters while test runs
     - waits for a 'dump' from the browser
"""

__author__ = 'annie.sullivan@gmail.com (Annie Sullivan)'


import platform
import os
import re
import shutil
import time
import sys
import subprocess
import utils

import ffprocess
import ffsetup

if platform.system() == "Linux":
    from cmanager_linux import *
    platform_type = 'unix_'
elif platform.system() in ("Windows", "Microsoft"):
    from cmanager_win32 import *
    platform_type = 'win_'
elif platform.system() == "Darwin":
    from cmanager_mac import *
    platform_type = 'unix_'



RESULTS_REGEX = re.compile('__start_report(.*)__end_report',
                      re.DOTALL | re.MULTILINE)

RESULTS_TP_REGEX = re.compile('__start_tp_report(.*)__end_tp_report',
                      re.DOTALL | re.MULTILINE)
RESULTS_GENERIC = re.compile('(.*)', re.DOTALL | re.MULTILINE)
RESULTS_REGEX_FAIL = re.compile('__FAIL(.*)__FAIL', re.DOTALL|re.MULTILINE)


def runTest(browser_config, test_config):
  """
  Runs an url based test on the browser as specified in the browser_config dictionary
  
  Args:
    browser_config:  Dictionary of configuration options for the browser (paths, prefs, etc)
    test_config   :  Dictionary of configuration for the given test (url, cycles, counters, etc)
  
  """
 
  res = 0
  utils.debug("operating with platform_type : " + platform_type)
  counters = test_config[platform_type + 'counters']
  resolution = test_config['resolution']
  all_browser_results = []
  all_counter_results = []
  utils.setEnvironmentVars(browser_config['env'])

  
  for dir in browser_config['dirs']:
    ffsetup.InstallInBrowser(browser_config['firefox'], browser_config['dirs'][dir])
  
  if browser_config["profile_path"] != {}:
      
      temp_dir, profile_dir = ffsetup.CreateTempProfileDir(browser_config['profile_path'],
                                                 browser_config['preferences'],
                                                 browser_config['extensions'])
      utils.debug("created profile") 
      
      
      
      
      res = ffsetup.InitializeNewProfile(browser_config['firefox'], profile_dir, browser_config['init_url'])
      if not res:
          print "FAIL: couldn't initialize firefox"
          return (res, all_browser_results, all_counter_results)
      res = 0
  else:
      
      profile_dir = ""

  utils.debug("initialized firefox")
  sys.stdout.flush()
  ffprocess.Sleep()

  for i in range(test_config['cycles']):
    
    browser_results = ""
    if 'timeout' in test_config:
      timeout = test_config['timeout']
    else:
      timeout = 28800 
    total_time = 0
    output = ''
    url = test_config['url']
    if 'url_mod' in test_config:
      url += eval(test_config['url_mod']) 
    command_line = ffprocess.GenerateFirefoxCommandLine(browser_config['firefox'], profile_dir, url)

    utils.debug("command line: " + command_line)

    process = subprocess.Popen(command_line, stdout=subprocess.PIPE, universal_newlines=True, shell=True, bufsize=0, env=os.environ)
    handle = process.stdout

    
    
    
    ffprocess.Sleep()
    
    cm = CounterManager("firefox", counters)
    cm.startMonitor()
    counter_results = {}
    for counter in counters:
      counter_results[counter] = []
   
    busted = False 
    while total_time < timeout:
      
      time.sleep(resolution)
      total_time += resolution
      
      
      for count_type in counters:
        val = cm.getCounterValue(count_type)

        if (val):
          counter_results[count_type].append(val)

      
      (bytes, current_output) = ffprocess.NonBlockingReadProcessOutput(handle)
      output += current_output
      match = RESULTS_GENERIC.search(current_output)
      if match:
        if match.group(1):
          utils.noisy(match.group(1))
      match = RESULTS_REGEX.search(output)
      if match:
        browser_results += match.group(1)
        utils.debug("Matched basic results: " + browser_results)
        res = 1
        break
      
      match = RESULTS_TP_REGEX.search(output)
      if match:
        browser_results += match.group(1)
        utils.debug("Matched tp results: " + browser_results)
        res = 1
        break
      match = RESULTS_REGEX_FAIL.search(output)
      if match:
        browser_results += match.group(1)
        utils.debug("Matched fail results: " + browser_results)
        print "FAIL: " + match.group(1)
        break

      
      
      
      
      if busted:
        ffprocess.TerminateAllProcesses("firefox")
        ffprocess.TerminateAllProcesses("crashreporter")
        print "FAIL: browser crash"
        break
      if (total_time % 60 == 0): 
        if ffprocess.ProcessesWithNameExist("crashreporter") or not ffprocess.ProcessesWithNameExist("firefox"):
          busted = True

    if total_time >= timeout:
      print "FAIL: timeout from test"

    
    cm.stopMonitor()

    utils.debug("Completed test with: " + browser_results)
    
    ffprocess.Sleep()
    ffprocess.TerminateAllProcesses("firefox")
    all_browser_results.append(browser_results)
    all_counter_results.append(counter_results)
    
  
  
  
  ffsetup.MakeDirectoryContentsWritable(temp_dir)
  shutil.rmtree(temp_dir)
    
  utils.restoreEnvironmentVars()
    
  return (res, all_browser_results, all_counter_results)

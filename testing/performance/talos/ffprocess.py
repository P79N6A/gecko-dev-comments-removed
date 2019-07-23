



































"""A set of functions for process management on Windows.
"""

__author__ = 'annie.sullivan@gmail.com (Annie Sullivan)'



import platform
import os
import re
import time
import subprocess

if platform.system() == "Linux":
    from ffprocess_linux import *
elif platform.system() in ("Windows", "Microsoft"):
    from ffprocess_win32 import *
elif platform.system() == "Darwin":
    from ffprocess_mac import *



def Sleep():
  """Runs sync and sleeps for a few seconds between Firefox runs.
     Otherwise "Firefox is already running.." errors occur
  """
  time.sleep(5)


def RunProcessAndWaitForOutput(command, process_name, output_regex, timeout):
  """Runs the given process and waits for the output that matches the given
     regular expression.  Stops if the process exits early or times out.

  Args:
    command: String containing command to run
    process_name: Name of the process to run, in case it has to be killed
    output_regex: Regular expression to check against each output line.
                  If the output matches, the process is terminated and 
                  the function returns.
    timeout: Time to wait before terminating the process and returning

  Returns:
    A tuple (match, timedout) where match is the match of the regular 
    expression, and timed out is true if the process timed out and 
    false otherwise.
  """

  
  process = subprocess.Popen(command, stdout=subprocess.PIPE, universal_newlines=True, shell=True, env=os.environ)
  handle = process.stdout

  
  time_elapsed = 0
  output = ''
  interval = 2 

  while time_elapsed < timeout:
    time.sleep(interval)
    time_elapsed += interval

    (bytes, current_output) = NonBlockingReadProcessOutput(handle)
    output += current_output
    
    result = output_regex.search(output)
    if result:
      try:
        return_val = result.group(1)
        TerminateAllProcesses(process_name)
        return (return_val, False)
      except IndexError:
        
        pass

  
  TerminateAllProcesses(process_name)
  return (None, True)

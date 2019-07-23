





































import subprocess
import signal
import os
import time
import config
from select import select


def GenerateFirefoxCommandLine(firefox_path, profile_dir, url):
  """Generates the command line for a process to run Firefox

  Args:
    firefox_path: String containing the path to the firefox binary to use
    profile_dir: String containing the directory of the profile to run Firefox in
    url: String containing url to start with.
  """

  profile_arg = ''
  if profile_dir:
    profile_arg = '-profile %s' % profile_dir

  url_arg = ''
  if url:
    url_arg = '-url %s' % url

  cmd = '%s %s %s -width %d -height %d' % (firefox_path,
                      profile_arg,
                      url_arg,
                      config.BROWSER_WIDTH,
                      config.BROWSER_HEIGHT)
  return cmd


def GetPidsByName(process_name):
  """Searches for processes containing a given string.

  Args:
    process_name: The string to be searched for

  Returns:
    A list of PIDs containing the string. An empty list is returned if none are
    found.
  """

  matchingPids = []
  
  command = ['ps -Axc']
  handle = subprocess.Popen(command, stdout=subprocess.PIPE, universal_newlines=True, shell=True)
  
  
  handle.wait()
  data = handle.stdout.readlines()
  
  
  for line in data:
    if line.find(process_name) >= 0:
      
      pid = int(line.split()[0])
      matchingPids.append(pid)

  return matchingPids


def ProcessesWithNameExist(process_name):
  """Returns true if there are any processes running with the
     given name.  Useful to check whether a Firefox process is still running

  Args:
    process_name: String containing the process name, i.e. "firefox"

  Returns:
    True if any processes with that name are running, False otherwise.
  """

  pids = GetPidsByName(process_name)
  return len(pids) > 0


def TerminateProcess(pid):
  """Helper function to terminate a process, given the pid

  Args:
    pid: integer process id of the process to terminate.
  """
  os.kill(pid, signal.SIGTERM)

def TerminateAllProcesses(process_name):
  """Helper function to terminate all processes with the given process name

  Args:
    process_name: String containing the process name, i.e. "firefox"
  """
  pids = GetPidsByName(process_name)
  for pid in pids:
    TerminateProcess(pid)

def NonBlockingReadProcessOutput(handle):
  """Does a non-blocking read from the output of the process
     with the given handle.

  Args:
    handle: The process handle returned from os.popen()

  Returns:
    A tuple (bytes, output) containing the number of output
    bytes read, and the actual output.
  """

  output = ""
  num_avail = 0

  
  
  
  
  
  while select([handle], [], [], 0)[0]:
    line = handle.readline()
    if line:
        output += line
    else:
        break
    
    num_avail = len(output)

  return (num_avail, output)

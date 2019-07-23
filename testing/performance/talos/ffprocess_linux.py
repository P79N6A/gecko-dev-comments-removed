




































import subprocess
import signal
import os
from select import select
import time



def GenerateFirefoxCommandLine(firefox_path, profile_dir, url):
  """Generates the command line for a process to run Firefox

  Args:
    firefox_path: String containing the path to the firefox exe to use
    profile_dir: String containing the directory of the profile to run Firefox in
    url: String containing url to start with.
  """

  profile_arg = ''
  if profile_dir:
    profile_arg = '-profile %s' % profile_dir

  cmd = '%s %s %s' % (firefox_path,
                      profile_arg,
                      url)
  return cmd


def GetPidsByName(process_name):
  """Searches for processes containing a given string.
     This function is UNIX specific.

  Args:
    process_name: The string to be searched for

  Returns:
    A list of PIDs containing the string. An empty list is returned if none are
    found.
  """

  matchingPids = []
  
  command = ['ps', 'ax']
  handle = subprocess.Popen(command, stdout=subprocess.PIPE)

  
  handle.wait()
  data = handle.stdout.read()
  
  
  for line in data.splitlines():
    if line.find(process_name) >= 0:
      
      pid = int(line.split()[0])
      matchingPids.append(pid)

  return matchingPids


def ProcessesWithNameExist(*process_names):
  """Returns true if there are any processes running with the
     given name.  Useful to check whether a Firefox process is still running

  Args:
    process_names: String or strings containing the process name, i.e. "firefox"

  Returns:
    True if any processes with that name are running, False otherwise.
  """

  for process_name in process_names:
    pids = GetPidsByName(process_name)
    if len(pids) > 0:
      return True
  return False


def TerminateProcess(pid):
  """Helper function to terminate a process, given the pid

  Args:
    pid: integer process id of the process to terminate.
  """
  try:
    if ProcessesWithNameExist(str(pid)):
      os.kill(pid, signal.SIGTERM)
      time.sleep(5)
      if ProcessesWithNameExist(str(pid)):
        os.kill(pid, signal.SIGKILL)
  except OSError, (errno, strerror):
    print 'WARNING: failed os.kill: %s : %s' % (errno, strerror)

def TerminateAllProcesses(*process_names):
  """Helper function to terminate all processes with the given process name

  Args:
    process_names: String or strings containing the process name, i.e. "firefox"
  """

  
  
  for process_name in process_names:
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

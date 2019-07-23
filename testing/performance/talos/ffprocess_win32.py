




































import win32api
import win32file
import win32pdhutil
import win32pipe
import msvcrt

import config


def GetCygwinPath(dos_path):
  """Helper function to get the Cygwin path from a dos path.
     Used to generate a Firefox command line piped through the
     Cygwin bash shell

  Args:
    dos_path: String containing the dos path

  Returns:
    String containing the cygwin path
  """

  
  
  cygwin_path = '/' + dos_path[3:]                       
  cygwin_path = cygwin_path.replace('\\', '/')           
  cygwin_path = cygwin_path.replace(' ', '\\ ')          
  cygwin_path = '/cygdrive/' + dos_path[0] + cygwin_path 
  return cygwin_path


def GenerateFirefoxCommandLine(firefox_path, profile_dir, url):
  """Generates the command line for a process to run Firefox, wrapped
     by cygwin so that we can read the output from dump() statements.

  Args:
    firefox_path: String containing the path to the firefox exe to use
    profile_dir: String containing the directory of the profile to run Firefox in
    url: String containing url to start with.
  """

  profile_arg = ''
  if profile_dir:
    profile_dir = profile_dir.replace('\\', '\\\\\\')
    profile_arg = '-profile %s' % profile_dir

  url_arg = ''
  if url:
    url_arg = '-url %s' % url

  cmd = '%s "%s %s %s"' % (config.CYGWIN,
                           GetCygwinPath(firefox_path),
                           profile_arg,
                           url_arg)
  return cmd


def TerminateProcess(pid):
  """Helper function to terminate a process, given the pid

  Args:
    pid: integer process id of the process to terminate.
  """

  PROCESS_TERMINATE = 1
  handle = win32api.OpenProcess(PROCESS_TERMINATE, False, pid)
  win32api.TerminateProcess(handle, -1)
  win32api.CloseHandle(handle)


def ProcessesWithNameExist(process_name):
  """Returns true if there are any processes running with the
     given name.  Useful to check whether a Firefox process is still running

  Args:
    process_name: String containing the process name, i.e. "firefox"

  Returns:
    True if any processes with that name are running, False otherwise.
  """

  try:
    pids = win32pdhutil.FindPerformanceAttributesByName(process_name, counter="ID Process")
    return len(pids) > 0
  except:
    
    return False


def TerminateAllProcesses(process_name):
  """Helper function to terminate all processes with the given process name

  Args:
    process_name: String containing the process name, i.e. "firefox"
  """

  
  try:
    pids = win32pdhutil.FindPerformanceAttributesByName(process_name, counter="ID Process")
    for pid in pids:
      TerminateProcess(pid)
  except:
    
    pass


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

  try:
    osfhandle = msvcrt.get_osfhandle(handle.fileno())
    (read, num_avail, num_message) = win32pipe.PeekNamedPipe(osfhandle, 0)
    if num_avail > 0:
      (error_code, output) = win32file.ReadFile(osfhandle, num_avail, None)

    return (num_avail, output)
  except:
    return (0, output)

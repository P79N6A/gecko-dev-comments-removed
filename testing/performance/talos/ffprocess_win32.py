




































import win32api
import win32file
import win32pdhutil
import win32pdh
import win32pipe
import msvcrt


def GenerateFirefoxCommandLine(firefox_path, profile_dir, url):
  """Generates the command line for a process to run Firefox

  Args:
    firefox_path: String containing the path to the firefox exe to use
    profile_dir: String containing the directory of the profile to run Firefox in
    url: String containing url to start with.
  """

  profile_arg = ''
  if profile_dir:
    profile_dir = profile_dir.replace('\\', '\\\\\\')
    profile_arg = '-profile %s' % profile_dir

  cmd = '%s %s %s' % (firefox_path,
                           profile_arg,
                           url)
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


def ProcessesWithNameExist(*process_names):
  """Returns true if there are any processes running with the
     given name.  Useful to check whether a Firefox process is still running

  Args:
    process_name: String or strings containing the process name, i.e. "firefox"

  Returns:
    True if any processes with that name are running, False otherwise.
  """

  for process_name in process_names: 
    try:
      
      win32pdh.EnumObjects(None, None, 0, 1)
      pids = win32pdhutil.FindPerformanceAttributesByName(process_name, counter="ID Process")
      if len(pids) > 0:
        return True 
    except:
      
      continue
  return False


def TerminateAllProcesses(*process_names):
  """Helper function to terminate all processes with the given process name

  Args:
    process_name: String or strings containing the process name, i.e. "firefox"
  """
  for process_name in process_names:
    
    try:
      pids = win32pdhutil.FindPerformanceAttributesByName(process_name, counter="ID Process")
      for pid in pids:
        TerminateProcess(pid)
    except:
      
      continue


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





import logging
import os
import signal
import subprocess
import sys
import time


class NotImplementedError(Exception):
  pass


class TimeoutError(Exception):
  pass


def _print_line(line, flush=True):
  
  
  
  print line.rstrip() + '\n',
  if flush:
    sys.stdout.flush()


def RunSubprocessInBackground(proc):
  """Runs a subprocess in the background. Returns a handle to the process."""
  logging.info("running %s in the background" % " ".join(proc))
  return subprocess.Popen(proc)


def RunSubprocess(proc, timeout=0, detach=False, background=False):
  """ Runs a subprocess, until it finishes or |timeout| is exceeded and the
  process is killed with taskkill.  A |timeout| <= 0  means no timeout.

  Args:
    proc: list of process components (exe + args)
    timeout: how long to wait before killing, <= 0 means wait forever
    detach: Whether to pass the DETACHED_PROCESS argument to CreateProcess
        on Windows.  This is used by Purify subprocesses on buildbot which
        seem to get confused by the parent console that buildbot sets up.
  """

  logging.info("running %s, timeout %d sec" % (" ".join(proc), timeout))
  if detach:
    
    DETACHED_PROCESS = 0x8
    p = subprocess.Popen(proc, creationflags=DETACHED_PROCESS)
  else:
    
    
    
    
    
    
    
    p = subprocess.Popen(proc, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

  logging.info("started subprocess")

  
  progress_delay = 300
  progress_delay_time = time.time() + progress_delay
  did_timeout = False
  if timeout > 0:
    wait_until = time.time() + timeout
  while p.poll() is None and not did_timeout:
    if not detach:
      line = p.stdout.readline()
      while line and not did_timeout:
        _print_line(line)
        line = p.stdout.readline()
        if timeout > 0:
          did_timeout = time.time() > wait_until
    else:
      
      
      time.sleep(0.5)
      if time.time() >= progress_delay_time:
        
        
        
        
        
        logging.info("%s still running..." % os.path.basename(proc[0]))
        progress_delay_time = time.time() + progress_delay
    if timeout > 0:
      did_timeout = time.time() > wait_until

  if did_timeout:
    logging.info("process timed out")
  else:
    logging.info("process ended, did not time out")

  if did_timeout:
    if IsWindows():
      subprocess.call(["taskkill", "/T", "/F", "/PID", str(p.pid)])
    else:
      
      os.kill(p.pid, signal.SIGINT)
    logging.error("KILLED %d" % p.pid)
    
    
    time.sleep(1.0)
    logging.error("TIMEOUT waiting for %s" % proc[0])
    raise TimeoutError(proc[0])
  elif not detach:
    for line in p.stdout.readlines():
      _print_line(line, False)
    if not IsMac():   
      logging.info("flushing stdout")
      p.stdout.flush()

  logging.info("collecting result code")
  result = p.poll()
  if result:
    logging.error("%s exited with non-zero result code %d" % (proc[0], result))
  return result


def IsLinux():
  return sys.platform.startswith('linux')


def IsMac():
  return sys.platform.startswith('darwin')


def IsWindows():
  return sys.platform == 'cygwin' or sys.platform.startswith('win')


def PlatformNames():
  """Return an array of string to be used in paths for the platform
  (e.g. suppressions, gtest filters, ignore files etc.)
  The first element of the array describes the 'main' platform
  """
  if IsLinux():
    return ['linux']
  if IsMac():
    return ['mac']
  if IsWindows():
    return ['win32']
  raise NotImplementedError('Unknown platform "%s".' % sys.platform)


def PutEnvAndLog(env_name, env_value):
  os.putenv(env_name, env_value)
  logging.info('export %s=%s', env_name, env_value)

def BoringCallers(mangled, use_re_wildcards):
  """Return a list of 'boring' function names (optinally mangled)
  with */? wildcards (optionally .*/.).
  Boring = we drop off the bottom of stack traces below such functions.
  """

  need_mangling = [
    
    ("testing::Test::Run",     "_ZN7testing4Test3RunEv"),
    ("testing::TestInfo::Run", "_ZN7testing8TestInfo3RunEv"),
    ("testing::internal::Handle*ExceptionsInMethodIfSupported*",
     "_ZN7testing8internal3?Handle*ExceptionsInMethodIfSupported*"),

    
    ("MessageLoop::Run",     "_ZN11MessageLoop3RunEv"),
    ("MessageLoop::RunTask", "_ZN11MessageLoop7RunTask*"),
    ("RunnableMethod*",      "_ZN14RunnableMethod*"),
    ("DispatchToMethod*",    "_Z*16DispatchToMethod*"),
    ("base::internal::Invoker*::DoInvoke*",
     "_ZN4base8internal8Invoker*DoInvoke*"),  
    ("base::internal::RunnableAdapter*::Run*",
     "_ZN4base8internal15RunnableAdapter*Run*"),
  ]

  ret = []
  for pair in need_mangling:
    ret.append(pair[1 if mangled else 0])

  ret += [
    
    "start_thread",
    "main",
    "BaseThreadInitThunk",
  ]

  if use_re_wildcards:
    for i in range(0, len(ret)):
      ret[i] = ret[i].replace('*', '.*').replace('?', '.')

  return ret

def NormalizeWindowsPath(path):
  """If we're using Cygwin Python, turn the path into a Windows path.

  Don't turn forward slashes into backslashes for easier copy-pasting and
  escaping.

  TODO(rnk): If we ever want to cut out the subprocess invocation, we can use
  _winreg to get the root Cygwin directory from the registry key:
  HKEY_LOCAL_MACHINE\SOFTWARE\Cygwin\setup\rootdir.
  """
  if sys.platform.startswith("cygwin"):
    p = subprocess.Popen(["cygpath", "-m", path],
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    (out, err) = p.communicate()
    if err:
      logging.warning("WARNING: cygpath error: %s", err)
    return out.strip()
  else:
    return path




def PrintUsedSuppressionsList(suppcounts):
  """ Prints out the list of used suppressions in a format common to all the
      memory tools. If the list is empty, prints nothing and returns False,
      otherwise True.

      suppcounts: a dictionary of used suppression counts,
                  Key -> name, Value -> count.
  """
  if not suppcounts:
    return False

  print "-----------------------------------------------------"
  print "Suppressions used:"
  print "  count name"
  for (name, count) in sorted(suppcounts.items(), key=lambda (k,v): (v,k)):
    print "%7d %s" % (count, name)
  print "-----------------------------------------------------"
  sys.stdout.flush()
  return True

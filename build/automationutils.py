




from __future__ import with_statement
import logging
from operator import itemgetter
import os
import platform
import re
import signal
import subprocess
import sys
import tempfile
import mozinfo

__all__ = [
  "dumpLeakLog",
  "processLeakLog",
  'dumpScreen',
  "setAutomationLog",
  ]

log = logging.getLogger()
def resetGlobalLog():
  while log.handlers:
    log.removeHandler(log.handlers[0])
  handler = logging.StreamHandler(sys.stdout)
  log.setLevel(logging.INFO)
  log.addHandler(handler)
resetGlobalLog()

def setAutomationLog(alt_logger):
  global log
  log = alt_logger



def strsig(n):
  
  
  _sigtbl = [None]*signal.NSIG
  for k in dir(signal):
    if k.startswith("SIG") and not k.startswith("SIG_") and k != "SIGCLD" and k != "SIGPOLL":
      _sigtbl[getattr(signal, k)] = k
  
  if hasattr(signal, "SIGRTMIN") and hasattr(signal, "SIGRTMAX"):
    for r in range(signal.SIGRTMIN+1, signal.SIGRTMAX+1):
      _sigtbl[r] = "SIGRTMIN+" + str(r - signal.SIGRTMIN)
  
  for i in range(signal.NSIG):
    if _sigtbl[i] is None:
      _sigtbl[i] = "unrecognized signal, number " + str(i)
  if n < 0 or n >= signal.NSIG:
    return "out-of-range signal, number "+str(n)
  return _sigtbl[n]

def printstatus(status, name = ""):
  
  if os.name != 'posix':
    
    if status < 0:
      status += 2**32
    print "TEST-INFO | %s: exit status %x\n" % (name, status)
  elif os.WIFEXITED(status):
    print "TEST-INFO | %s: exit %d\n" % (name, os.WEXITSTATUS(status))
  elif os.WIFSIGNALED(status):
    
    print "TEST-INFO | {}: killed by {}".format(name,strsig(os.WTERMSIG(status)))
  else:
    
    print "TEST-INFO | %s: undecodable exit status %04x\n" % (name, status)

def dumpLeakLog(leakLogFile, filter = False):
  """Process the leak log, without parsing it.

  Use this function if you want the raw log only.
  Use it preferably with the |XPCOM_MEM_LEAK_LOG| environment variable.
  """

  
  if not os.path.exists(leakLogFile):
    return

  with open(leakLogFile, "r") as leaks:
    leakReport = leaks.read()

  
  
  if filter and not "0 TOTAL " in leakReport:
    return

  
  log.info(leakReport.rstrip("\n"))

def processSingleLeakFile(leakLogFileName, processType, leakThreshold, ignoreMissingLeaks):
  """Process a single leak log.
  """

  
  
  
  
  lineRe = re.compile(r"^\s*\d+ \|"
                      r"(?P<name>[^|]+)\|"
                      r"\s*(?P<size>-?\d+)\s+(?P<bytesLeaked>-?\d+)\s*\|"
                      r"\s*-?\d+\s+(?P<numLeaked>-?\d+)")
  

  processString = "%s process:" % processType
  crashedOnPurpose = False
  totalBytesLeaked = None
  logAsWarning = False
  leakAnalysis = []
  leakedObjectAnalysis = []
  leakedObjectNames = []
  recordLeakedObjects = False
  with open(leakLogFileName, "r") as leaks:
    for line in leaks:
      if line.find("purposefully crash") > -1:
        crashedOnPurpose = True
      matches = lineRe.match(line)
      if not matches:
        
        log.info(line.rstrip())
        continue
      name = matches.group("name").rstrip()
      size = int(matches.group("size"))
      bytesLeaked = int(matches.group("bytesLeaked"))
      numLeaked = int(matches.group("numLeaked"))
      
      
      if numLeaked != 0 or name == "TOTAL":
        log.info(line.rstrip())
      
      if name == "TOTAL":
        
        
        
        if totalBytesLeaked != None:
          leakAnalysis.append("WARNING | leakcheck | %s multiple BloatView byte totals found"
                              % processString)
        else:
          totalBytesLeaked = 0
        if bytesLeaked > totalBytesLeaked:
          totalBytesLeaked = bytesLeaked
          
          leakedObjectNames = []
          leakedObjectAnalysis = []
          recordLeakedObjects = True
        else:
          recordLeakedObjects = False
      if size < 0 or bytesLeaked < 0 or numLeaked < 0:
        leakAnalysis.append("TEST-UNEXPECTED-FAIL | leakcheck | %s negative leaks caught!"
                            % processString)
        logAsWarning = True
        continue
      if name != "TOTAL" and numLeaked != 0 and recordLeakedObjects:
        leakedObjectNames.append(name)
        leakedObjectAnalysis.append("TEST-INFO | leakcheck | %s leaked %d %s (%s bytes)"
                                    % (processString, numLeaked, name, bytesLeaked))

  leakAnalysis.extend(leakedObjectAnalysis)
  if logAsWarning:
    log.warning('\n'.join(leakAnalysis))
  else:
    log.info('\n'.join(leakAnalysis))

  logAsWarning = False

  if totalBytesLeaked is None:
    
    if crashedOnPurpose:
      log.info("TEST-INFO | leakcheck | %s deliberate crash and thus no leak log"
               % processString)
    elif ignoreMissingLeaks:
      log.info("TEST-INFO | leakcheck | %s ignoring missing output line for total leaks"
               % processString)
    else:
      log.info("TEST-UNEXPECTED-FAIL | leakcheck | %s missing output line for total leaks!"
               % processString)
      log.info("TEST-INFO | leakcheck | missing output line from log file %s"
               % leakLogFileName)
    return

  if totalBytesLeaked == 0:
    log.info("TEST-PASS | leakcheck | %s no leaks detected!" % processString)
    return

  
  if totalBytesLeaked > leakThreshold:
    logAsWarning = True
    
    prefix = "TEST-UNEXPECTED-FAIL"
  else:
    prefix = "WARNING"
  
  
  
  maxSummaryObjects = 5
  leakedObjectSummary = ', '.join(leakedObjectNames[:maxSummaryObjects])
  if len(leakedObjectNames) > maxSummaryObjects:
    leakedObjectSummary += ', ...'

  if logAsWarning:
    log.warning("%s | leakcheck | %s %d bytes leaked (%s)"
                % (prefix, processString, totalBytesLeaked, leakedObjectSummary))
  else:
    log.info("%s | leakcheck | %s %d bytes leaked (%s)"
             % (prefix, processString, totalBytesLeaked, leakedObjectSummary))

def processLeakLog(leakLogFile, options):
  """Process the leak log, including separate leak logs created
  by child processes.

  Use this function if you want an additional PASS/FAIL summary.
  It must be used with the |XPCOM_MEM_BLOAT_LOG| environment variable.

  The base of leakLogFile for a non-default process needs to end with
    _proctype_pid12345.log
  "proctype" is a string denoting the type of the process, which should
  be the result of calling XRE_ChildProcessTypeToString(). 12345 is
  a series of digits that is the pid for the process. The .log is
  optional.

  All other file names are treated as being for default processes.

  The options argument is checked for two optional attributes,
  leakThresholds and ignoreMissingLeaks.

  leakThresholds should be a dict mapping process types to leak thresholds,
  in bytes. If a process type is not present in the dict the threshold
  will be 0.

  ignoreMissingLeaks should be a list of process types. If a process
  creates a leak log without a TOTAL, then we report an error if it isn't
  in the list ignoreMissingLeaks.
  """

  if not os.path.exists(leakLogFile):
    log.info("WARNING | leakcheck | refcount logging is off, so leaks can't be detected!")
    return

  leakThresholds = getattr(options, 'leakThresholds', {})
  ignoreMissingLeaks = getattr(options, 'ignoreMissingLeaks', [])

  
  
  knownProcessTypes = ["default", "plugin", "tab", "geckomediaplugin"]

  for processType in knownProcessTypes:
    log.info("TEST-INFO | leakcheck | %s process: leak threshold set at %d bytes"
             % (processType, leakThresholds.get(processType, 0)))

  for processType in leakThresholds:
    if not processType in knownProcessTypes:
      log.info("TEST-UNEXPECTED-FAIL | leakcheck | Unknown process type %s in leakThresholds"
               % processType)

  (leakLogFileDir, leakFileBase) = os.path.split(leakLogFile)
  if leakFileBase[-4:] == ".log":
    leakFileBase = leakFileBase[:-4]
    fileNameRegExp = re.compile(r"_([a-z]*)_pid\d*.log$")
  else:
    fileNameRegExp = re.compile(r"_([a-z]*)_pid\d*$")

  for fileName in os.listdir(leakLogFileDir):
    if fileName.find(leakFileBase) != -1:
      thisFile = os.path.join(leakLogFileDir, fileName)
      m = fileNameRegExp.search(fileName)
      if m:
        processType = m.group(1)
      else:
        processType = "default"
      if not processType in knownProcessTypes:
        log.info("TEST-UNEXPECTED-FAIL | leakcheck | Leak log with unknown process type %s"
                 % processType)
      leakThreshold = leakThresholds.get(processType, 0)
      processSingleLeakFile(thisFile, processType, leakThreshold,
                            processType in ignoreMissingLeaks)

def dumpScreen(utilityPath):
  """dumps a screenshot of the entire screen to a directory specified by
  the MOZ_UPLOAD_DIR environment variable"""

  
  if mozinfo.isUnix:
    utility = [os.path.join(utilityPath, "screentopng")]
    utilityname = "screentopng"
  elif mozinfo.isMac:
    utility = ['/usr/sbin/screencapture', '-C', '-x', '-t', 'png']
    utilityname = "screencapture"
  elif mozinfo.isWin:
    utility = [os.path.join(utilityPath, "screenshot.exe")]
    utilityname = "screenshot"

  
  parent_dir = os.environ.get('MOZ_UPLOAD_DIR', None)
  if not parent_dir:
    log.info('Failed to retrieve MOZ_UPLOAD_DIR env var')
    return

  
  try:
    tmpfd, imgfilename = tempfile.mkstemp(prefix='mozilla-test-fail-screenshot_', suffix='.png', dir=parent_dir)
    os.close(tmpfd)
    returncode = subprocess.call(utility + [imgfilename])
    printstatus(returncode, utilityname)
  except OSError, err:
    log.info("Failed to start %s for screenshot: %s" %
             utility[0], err.strerror)
    return

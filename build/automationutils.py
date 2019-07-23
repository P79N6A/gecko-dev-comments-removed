





































import glob, logging, os, subprocess, sys
import re

__all__ = [
  "addCommonOptions",
  "checkForCrashes",
  "dumpLeakLog",
  "processLeakLog",
  "getDebuggerInfo",
  "DEBUGGER_INFO",
  ]



DEBUGGER_INFO = {
  
  
  "gdb": {
    "interactive": True,
    "args": "-q --args"
  },

  
  
  "valgrind": {
    "interactive": False,
    "args": "--leak-check=full"
  }
}

log = logging.getLogger()

def addCommonOptions(parser, defaults={}):
  parser.add_option("--xre-path",
                    action = "store", type = "string", dest = "xrePath",
                    
                    default = None,
                    help = "absolute path to directory containing XRE (probably xulrunner)")
  if 'SYMBOLS_PATH' not in defaults:
    defaults['SYMBOLS_PATH'] = None
  parser.add_option("--symbols-path",
                    action = "store", type = "string", dest = "symbolsPath",
                    default = defaults['SYMBOLS_PATH'],
                    help = "absolute path to directory containing breakpad symbols")
  parser.add_option("--debugger",
                    action = "store", dest = "debugger",
                    help = "use the given debugger to launch the application")
  parser.add_option("--debugger-args",
                    action = "store", dest = "debuggerArgs",
                    help = "pass the given args to the debugger _before_ "
                           "the application on the command line")
  parser.add_option("--debugger-interactive",
                    action = "store_true", dest = "debuggerInteractive",
                    help = "prevents the test harness from redirecting "
                        "stdout and stderr for interactive debuggers")

def checkForCrashes(dumpDir, symbolsPath, testName=None):
  stackwalkPath = os.environ.get('MINIDUMP_STACKWALK', None)
  
  if testName is None:
    try:
      testName = os.path.basename(sys._getframe(1).f_code.co_filename)
    except:
      testName = "unknown"

  foundCrash = False
  dumps = glob.glob(os.path.join(dumpDir, '*.dmp'))
  for d in dumps:
    log.info("TEST-UNEXPECTED-FAIL | %s | application crashed (minidump found)", testName)
    if symbolsPath and stackwalkPath and os.path.exists(stackwalkPath):
      nullfd = open(os.devnull, 'w')
      
      subprocess.call([stackwalkPath, d, symbolsPath], stderr=nullfd)
      nullfd.close()
    else:
      if not symbolsPath:
        print "No symbols path given, can't process dump."
      if not stackwalkPath:
        print "MINIDUMP_STACKWALK not set, can't process dump."
      else:
        if not os.path.exists(stackwalkPath):
          print "MINIDUMP_STACKWALK binary not found: %s" % stackwalkPath
    os.remove(d)
    extra = os.path.splitext(d)[0] + ".extra"
    if os.path.exists(extra):
      os.remove(extra)
    foundCrash = True

  return foundCrash
  
def getFullPath(directory, path):
  "Get an absolute path relative to 'directory'."
  return os.path.normpath(os.path.join(directory, os.path.expanduser(path)))

def searchPath(directory, path):
  "Go one step beyond getFullPath and try the various folders in PATH"
  
  newpath = getFullPath(directory, path)
  if os.path.exists(newpath):
    return newpath

  
  
  if not os.path.dirname(path):
    for dir in os.environ['PATH'].split(os.pathsep):
      newpath = os.path.join(dir, path)
      if os.path.exists(newpath):
        return newpath
  return None

def getDebuggerInfo(directory, debugger, debuggerArgs, debuggerInteractive = False):

  debuggerInfo = None

  if debugger:
    debuggerPath = searchPath(directory, debugger)
    if not debuggerPath:
      print "Error: Path %s doesn't exist." % debugger
      sys.exit(1)

    debuggerName = os.path.basename(debuggerPath).lower()

    def getDebuggerInfo(type, default):
      if debuggerName in DEBUGGER_INFO and type in DEBUGGER_INFO[debuggerName]:
        return DEBUGGER_INFO[debuggerName][type]
      return default

    debuggerInfo = {
      "path": debuggerPath,
      "interactive" : getDebuggerInfo("interactive", False),
      "args": getDebuggerInfo("args", "").split()
    }

    if debuggerArgs:
      debuggerInfo["args"] = debuggerArgs.split()
    if debuggerInteractive:
      debuggerInfo["interactive"] = debuggerInteractive
  
  return debuggerInfo


def dumpLeakLog(leakLogFile, filter = False):
  """Process the leak log, without parsing it.

  Use this function if you want the raw log only.
  Use it preferably with the |XPCOM_MEM_LEAK_LOG| environment variable.
  """

  
  if not os.path.exists(leakLogFile):
    return

  leaks = open(leakLogFile, "r")
  leakReport = leaks.read()
  leaks.close()

  
  
  if filter and not "0 TOTAL " in leakReport:
    return

  
  log.info(leakReport.rstrip("\n"))

def processSingleLeakFile(leakLogFileName, PID, processType, leakThreshold):
  """Process a single leak log, corresponding to the specified
  process PID and type.
  """

  
  
  
  lineRe = re.compile(r"^\s*\d+\s+(?P<name>\S+)\s+"
                      r"(?P<size>-?\d+)\s+(?P<bytesLeaked>-?\d+)\s+"
                      r"-?\d+\s+(?P<numLeaked>-?\d+)")

  processString = ""
  if PID and processType:
    processString = "| %s process %s " % (processType, PID)
  leaks = open(leakLogFileName, "r")
  for line in leaks:
    matches = lineRe.match(line)
    if (matches and
        int(matches.group("numLeaked")) == 0 and
        matches.group("name") != "TOTAL"):
      continue
    log.info(line.rstrip())
  leaks.close()

  leaks = open(leakLogFileName, "r")
  seenTotal = False
  prefix = "TEST-PASS"
  for line in leaks:
    matches = lineRe.match(line)
    if not matches:
      continue
    name = matches.group("name")
    size = int(matches.group("size"))
    bytesLeaked = int(matches.group("bytesLeaked"))
    numLeaked = int(matches.group("numLeaked"))
    if size < 0 or bytesLeaked < 0 or numLeaked < 0:
      log.info("TEST-UNEXPECTED-FAIL %s| automationutils.processLeakLog() | negative leaks caught!" %
               processString)
      if name == "TOTAL":
        seenTotal = True
    elif name == "TOTAL":
      seenTotal = True
      
      if bytesLeaked < 0 or bytesLeaked > leakThreshold:
        prefix = "TEST-UNEXPECTED-FAIL"
        leakLog = "TEST-UNEXPECTED-FAIL %s| automationutils.processLeakLog() | leaked" \
                  " %d bytes during test execution" % (processString, bytesLeaked)
      elif bytesLeaked > 0:
        leakLog = "TEST-PASS %s| automationutils.processLeakLog() | WARNING leaked" \
                  " %d bytes during test execution" % (processString, bytesLeaked)
      else:
        leakLog = "TEST-PASS %s| automationutils.processLeakLog() | no leaks detected!" \
                  % processString
      
      if leakThreshold != 0:
        leakLog += " (threshold set at %d bytes)" % leakThreshold
      
      log.info(leakLog)
    else:
      if numLeaked != 0:
        if numLeaked > 1:
          instance = "instances"
          rest = " each (%s bytes total)" % matches.group("bytesLeaked")
        else:
          instance = "instance"
          rest = ""
        log.info("%(prefix)s %(process)s| automationutils.processLeakLog() | leaked %(numLeaked)d %(instance)s of %(name)s "
                 "with size %(size)s bytes%(rest)s" %
                 { "prefix": prefix,
                   "process": processString,
                   "numLeaked": numLeaked,
                   "instance": instance,
                   "name": name,
                   "size": matches.group("size"),
                   "rest": rest })
  if not seenTotal:
    log.info("TEST-UNEXPECTED-FAIL %s| automationutils.processLeakLog() | missing output line for total leaks!" %
             processString)
  leaks.close()


def processLeakLog(leakLogFile, leakThreshold = 0):
  """Process the leak log, including separate leak logs created
  by child processes.

  Use this function if you want an additional PASS/FAIL summary.
  It must be used with the |XPCOM_MEM_BLOAT_LOG| environment variable.
  """

  if not os.path.exists(leakLogFile):
    log.info("WARNING | automationutils.processLeakLog() | refcount logging is off, so leaks can't be detected!")
    return

  (leakLogFileDir, leakFileBase) = os.path.split(leakLogFile)
  pidRegExp = re.compile(r".*?_([a-z]*)_pid(\d*)$")
  if leakFileBase[-4:] == ".log":
    leakFileBase = leakFileBase[:-4]
    pidRegExp = re.compile(r".*?_([a-z]*)_pid(\d*).log$")

  for fileName in os.listdir(leakLogFileDir):
    if fileName.find(leakFileBase) != -1:
      thisFile = os.path.join(leakLogFileDir, fileName)
      processPID = 0
      processType = None
      m = pidRegExp.search(fileName)
      if m:
        processType = m.group(1)
        processPID = m.group(2)
      processSingleLeakFile(thisFile, processPID, processType, leakThreshold)

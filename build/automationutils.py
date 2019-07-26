




from __future__ import with_statement
import glob, logging, os, platform, shutil, subprocess, sys, tempfile, urllib2, zipfile
import re
from urlparse import urlparse

__all__ = [
  "ZipFileReader",
  "addCommonOptions",
  "checkForCrashes",
  "dumpLeakLog",
  "isURL",
  "processLeakLog",
  "getDebuggerInfo",
  "DEBUGGER_INFO",
  "replaceBackSlashes",
  "wrapCommand",
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

class ZipFileReader(object):
  """
  Class to read zip files in Python 2.5 and later. Limited to only what we
  actually use.
  """

  def __init__(self, filename):
    self._zipfile = zipfile.ZipFile(filename, "r")

  def __del__(self):
    self._zipfile.close()

  def _getnormalizedpath(self, path):
    """
    Gets a normalized path from 'path' (or the current working directory if
    'path' is None). Also asserts that the path exists.
    """
    if path is None:
      path = os.curdir
    path = os.path.normpath(os.path.expanduser(path))
    assert os.path.isdir(path)
    return path

  def _extractname(self, name, path):
    """
    Extracts a file with the given name from the zip file to the given path.
    Also creates any directories needed along the way.
    """
    filename = os.path.normpath(os.path.join(path, name))
    if name.endswith("/"):
      os.makedirs(filename)
    else:
      path = os.path.split(filename)[0]
      if not os.path.isdir(path):
        os.makedirs(path)
      with open(filename, "wb") as dest:
        dest.write(self._zipfile.read(name))

  def namelist(self):
    return self._zipfile.namelist()

  def read(self, name):
    return self._zipfile.read(name)

  def extract(self, name, path = None):
    if hasattr(self._zipfile, "extract"):
      return self._zipfile.extract(name, path)

    
    self._zipfile.getinfo(name)

    self._extractname(name, self._getnormalizedpath(path))

  def extractall(self, path = None):
    if hasattr(self._zipfile, "extractall"):
      return self._zipfile.extractall(path)

    path = self._getnormalizedpath(path)

    for name in self._zipfile.namelist():
      self._extractname(name, path)

log = logging.getLogger()

def isURL(thing):
  """Return True if |thing| looks like a URL."""
  
  return len(urlparse(thing).scheme) >= 2

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
                    help = "absolute path to directory containing breakpad symbols, or the URL of a zip file containing symbols")
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

  
  dumps = glob.glob(os.path.join(dumpDir, '*.dmp'))
  if len(dumps) == 0:
    return False

  try:
    removeSymbolsPath = False

    
    if symbolsPath and isURL(symbolsPath):
      print "Downloading symbols from: " + symbolsPath
      removeSymbolsPath = True
      
      data = urllib2.urlopen(symbolsPath)
      symbolsFile = tempfile.TemporaryFile()
      symbolsFile.write(data.read())
      
      
      symbolsPath = tempfile.mkdtemp()
      zfile = ZipFileReader(symbolsFile)
      zfile.extractall(symbolsPath)

    for d in dumps:
      stackwalkOutput = []
      stackwalkOutput.append("Crash dump filename: " + d)
      topFrame = None
      if symbolsPath and stackwalkPath and os.path.exists(stackwalkPath):
        
        p = subprocess.Popen([stackwalkPath, d, symbolsPath],
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        (out, err) = p.communicate()
        if len(out) > 3:
          
          stackwalkOutput.append(out)
          
          
          
          
          
          
          lines = out.splitlines()
          for i, line in enumerate(lines):
            if "(crashed)" in line:
              match = re.search(r"^ 0  (?:.*!)?(?:void )?([^\[]+)", lines[i+1])
              if match:
                topFrame = "@ %s" % match.group(1).strip()
              break
        else:
          stackwalkOutput.append("stderr from minidump_stackwalk:")
          stackwalkOutput.append(err)
        if p.returncode != 0:
          stackwalkOutput.append("minidump_stackwalk exited with return code %d" % p.returncode)
      else:
        if not symbolsPath:
          stackwalkOutput.append("No symbols path given, can't process dump.")
        if not stackwalkPath:
          stackwalkOutput.append("MINIDUMP_STACKWALK not set, can't process dump.")
        elif stackwalkPath and not os.path.exists(stackwalkPath):
          stackwalkOutput.append("MINIDUMP_STACKWALK binary not found: %s" % stackwalkPath)
      if not topFrame:
        topFrame = "Unknown top frame"
      log.info("PROCESS-CRASH | %s | application crashed [%s]", testName, topFrame)
      print '\n'.join(stackwalkOutput)
      dumpSavePath = os.environ.get('MINIDUMP_SAVE_PATH', None)
      if dumpSavePath:
        shutil.move(d, dumpSavePath)
        print "Saved dump as %s" % os.path.join(dumpSavePath,
                                                os.path.basename(d))
      else:
        os.remove(d)
      extra = os.path.splitext(d)[0] + ".extra"
      if os.path.exists(extra):
        os.remove(extra)
  finally:
    if removeSymbolsPath:
      shutil.rmtree(symbolsPath)

  return True

def getFullPath(directory, path):
  "Get an absolute path relative to 'directory'."
  return os.path.normpath(os.path.join(directory, os.path.expanduser(path)))

def searchPath(directory, path):
  "Go one step beyond getFullPath and try the various folders in PATH"
  
  newpath = getFullPath(directory, path)
  if os.path.isfile(newpath):
    return newpath

  
  
  if not os.path.dirname(path):
    for dir in os.environ['PATH'].split(os.pathsep):
      newpath = os.path.join(dir, path)
      if os.path.isfile(newpath):
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
  crashedOnPurpose = False
  totalBytesLeaked = None
  leakedObjectNames = []
  for line in leaks:
    if line.find("purposefully crash") > -1:
      crashedOnPurpose = True
    matches = lineRe.match(line)
    if not matches:
      continue
    name = matches.group("name")
    size = int(matches.group("size"))
    bytesLeaked = int(matches.group("bytesLeaked"))
    numLeaked = int(matches.group("numLeaked"))
    if size < 0 or bytesLeaked < 0 or numLeaked < 0:
      log.info("TEST-UNEXPECTED-FAIL %s| leakcheck | negative leaks caught!" %
               processString)
      if name == "TOTAL":
        totalBytesLeaked = bytesLeaked
    elif name == "TOTAL":
      totalBytesLeaked = bytesLeaked
    else:
      if numLeaked != 0:
        leakedObjectNames.append(name)
        log.info("TEST-INFO %s| leakcheck | leaked %d %s (%s bytes)"
                 % (processString, numLeaked, name, bytesLeaked))

  if totalBytesLeaked is None:
    
    if crashedOnPurpose:
      log.info("TEST-INFO | leakcheck | process %s was " \
               "deliberately crashed and thus has no leak log" % PID)
    else:
      
      
      log.info("WARNING | leakcheck | missing output line for total leaks!")
  else:
    if totalBytesLeaked == 0:
      log.info("TEST-PASS %s| leakcheck | no leaks detected!" % processString)
    else:
      
      if totalBytesLeaked > leakThreshold:
        prefix = "TEST-UNEXPECTED-FAIL"
      else:
        prefix = "WARNING"
      
      
      maxSummaryObjects = 5
      leakedObjectSummary = ', '.join(leakedObjectNames[:maxSummaryObjects])
      
      
      
      if len(leakedObjectNames) > maxSummaryObjects:
        leakedObjectSummary += ', ...'
      log.info("%s %s| leakcheck | %d bytes leaked (%s)"
               % (prefix, processString, totalBytesLeaked, leakedObjectSummary))
    
    if leakThreshold != 0:
      log.info("TEST-INFO | leakcheck | threshold set at %d bytes" % leakThreshold)
  leaks.close()

def processLeakLog(leakLogFile, leakThreshold = 0):
  """Process the leak log, including separate leak logs created
  by child processes.

  Use this function if you want an additional PASS/FAIL summary.
  It must be used with the |XPCOM_MEM_BLOAT_LOG| environment variable.
  """

  if not os.path.exists(leakLogFile):
    log.info("WARNING | leakcheck | refcount logging is off, so leaks can't be detected!")
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

def replaceBackSlashes(input):
  return input.replace('\\', '/')

def wrapCommand(cmd):
  """
  If running on OS X 10.5 or older, wrap |cmd| so that it will
  be executed as an i386 binary, in case it's a 32-bit/64-bit universal
  binary.
  """
  if platform.system() == "Darwin" and \
     hasattr(platform, 'mac_ver') and \
     platform.mac_ver()[0][:4] < '10.6':
    return ["arch", "-arch", "i386"] + cmd
  
  return cmd







































import glob, logging, os, subprocess, sys
import re

__all__ = [
  "addCommonOptions",
  "checkForCrashes",
  "dumpLeakLog",
  "processLeakLog",
  ]

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
    if symbolsPath and stackwalkPath:
      nullfd = open(os.devnull, 'w')
      
      subprocess.call([stackwalkPath, d, symbolsPath], stderr=nullfd)
      nullfd.close()
    else:
      if not symbolsPath:
        print "No symbols path given, can't process dump."
      if not stackwalkPath:
        print "MINIDUMP_STACKWALK not set, can't process dump."
    os.remove(d)
    extra = os.path.splitext(d)[0] + ".extra"
    if os.path.exists(extra):
      os.remove(extra)
    foundCrash = True

  return foundCrash

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

def processLeakLog(leakLogFile, leakThreshold = 0):
  """Process the leak log, parsing it.

  Use this function if you want an additional PASS/FAIL summary.
  It must be used with the |XPCOM_MEM_BLOAT_LOG| environment variable.
  """

  if not os.path.exists(leakLogFile):
    log.info("WARNING | automationutils.processLeakLog() | refcount logging is off, so leaks can't be detected!")
    return

  
  
  
  lineRe = re.compile(r"^\s*\d+\s+(?P<name>\S+)\s+"
                      r"(?P<size>-?\d+)\s+(?P<bytesLeaked>-?\d+)\s+"
                      r"-?\d+\s+(?P<numLeaked>-?\d+)")

  leaks = open(leakLogFile, "r")
  for line in leaks:
    matches = lineRe.match(line)
    if (matches and
        int(matches.group("numLeaked")) == 0 and
        matches.group("name") != "TOTAL"):
      continue
    log.info(line.rstrip())
  leaks.close()

  leaks = open(leakLogFile, "r")
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
      log.info("TEST-UNEXPECTED-FAIL | automationutils.processLeakLog() | negative leaks caught!")
      if name == "TOTAL":
        seenTotal = True
    elif name == "TOTAL":
      seenTotal = True
      
      if bytesLeaked < 0 or bytesLeaked > leakThreshold:
        prefix = "TEST-UNEXPECTED-FAIL"
        leakLog = "TEST-UNEXPECTED-FAIL | automationutils.processLeakLog() | leaked" \
                  " %d bytes during test execution" % bytesLeaked
      elif bytesLeaked > 0:
        leakLog = "TEST-PASS | automationutils.processLeakLog() | WARNING leaked" \
                  " %d bytes during test execution" % bytesLeaked
      else:
        leakLog = "TEST-PASS | automationutils.processLeakLog() | no leaks detected!"
      
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
        log.info("%(prefix)s | automationutils.processLeakLog() | leaked %(numLeaked)d %(instance)s of %(name)s "
                 "with size %(size)s bytes%(rest)s" %
                 { "prefix": prefix,
                   "numLeaked": numLeaked,
                   "instance": instance,
                   "name": name,
                   "size": matches.group("size"),
                   "rest": rest })
  if not seenTotal:
    log.info("TEST-UNEXPECTED-FAIL | automationutils.processLeakLog() | missing output line for total leaks!")
  leaks.close()

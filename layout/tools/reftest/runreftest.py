





































"""
Runs the reftest test harness.
"""

import sys, shutil, os, os.path
SCRIPT_DIRECTORY = os.path.abspath(os.path.realpath(os.path.dirname(sys.argv[0])))
sys.path.append(SCRIPT_DIRECTORY)
import automation
from automationutils import addCommonOptions
from optparse import OptionParser
from tempfile import mkdtemp

oldcwd = os.getcwd()
os.chdir(SCRIPT_DIRECTORY)

def getFullPath(path):
  "Get an absolute path relative to oldcwd."
  return os.path.normpath(os.path.join(oldcwd, os.path.expanduser(path)))

def createReftestProfile(options, profileDir):
  "Sets up a clean profile for reftest."

  
  shutil.rmtree(profileDir, True)
  os.mkdir(profileDir)
  
  prefsFile = open(os.path.join(profileDir, "user.js"), "w")
  prefsFile.write("""user_pref("browser.dom.window.dump.enabled", true);
""")
  prefsFile.write('user_pref("reftest.timeout", %d);' % options.timeout)

  prefsFile.close()
  
  profileExtensionsPath = os.path.join(profileDir, "extensions")
  os.mkdir(profileExtensionsPath)
  reftestExtensionPath = os.path.join(SCRIPT_DIRECTORY, "reftest")
  extFile = open(os.path.join(profileExtensionsPath, "reftest@mozilla.org"), "w")
  extFile.write(reftestExtensionPath)
  extFile.close()

def main():
  parser = OptionParser()

  
  addCommonOptions(parser, defaults=dict(zip(automation.__all__, [getattr(automation, x) for x in automation.__all__])))
  parser.add_option("--appname",
                    action = "store", type = "string", dest = "app",
                    default = os.path.join(SCRIPT_DIRECTORY, automation.DEFAULT_APP),
                    help = "absolute path to application, overriding default")
  parser.add_option("--extra-profile-file",
                    action = "append", dest = "extraProfileFiles",
                    default = [],
                    help = "copy specified files/dirs to testing profile")
  parser.add_option("--timeout",              
                    action = "store", dest = "timeout", type = "int", 
                    default = 5 * 60 * 1000, 
                    help = "reftest will timeout in specified number of milleseconds. [default %default ms].")
  parser.add_option("--leak-threshold",
                    action = "store", type = "int", dest = "leakThreshold",
                    default = 0,
                    help = "fail if the number of bytes leaked through "
                           "refcounted objects (or bytes in classes with "
                           "MOZ_COUNT_CTOR and MOZ_COUNT_DTOR) is greater "
                           "than the given number")

  options, args = parser.parse_args()

  if len(args) != 1:
    print >>sys.stderr, "No reftest.list specified."
    sys.exit(1)

  options.app = getFullPath(options.app)
  if not os.path.exists(options.app):
    print """Error: Path %(app)s doesn't exist.
Are you executing $objdir/_tests/reftest/runreftest.py?""" \
        % {"app": options.app}
    sys.exit(1)

  if options.xrePath is None:
    options.xrePath = os.path.dirname(options.app)
  else:
    
    options.xrePath = getFullPath(options.xrePath)

  profileDir = None
  try:
    profileDir = mkdtemp()
    createReftestProfile(options, profileDir)
    copyExtraFilesToProfile(options, profileDir)

    
    browserEnv = dict(os.environ)

    
    
    
    browserEnv["NO_EM_RESTART"] = "1"
    browserEnv["XPCOM_DEBUG_BREAK"] = "stack"
    if automation.UNIXISH:
      browserEnv["LD_LIBRARY_PATH"] = options.xrePath
      browserEnv["MOZILLA_FIVE_HOME"] = options.xrePath
      browserEnv["GNOME_DISABLE_CRASH_DIALOG"] = "1"

    
    leakLogFile = os.path.join(profileDir, "runreftest_leaks.log")
    browserEnv["XPCOM_MEM_BLOAT_LOG"] = leakLogFile

    
    
    automation.log.info("REFTEST INFO | runreftest.py | Performing extension manager registration: start.\n")
    
    status = automation.runApp(None, browserEnv, options.app, profileDir,
                               ["-silent"],
                               xrePath=options.xrePath,
                               symbolsPath=options.symbolsPath)
    
    automation.log.info("\nREFTEST INFO | runreftest.py | Performing extension manager registration: end.")

    
    
    if os.path.exists(leakLogFile):
      os.remove(leakLogFile)

    
    automation.log.info("REFTEST INFO | runreftest.py | Running tests: start.\n")
    reftestlist = getFullPath(args[0])
    status = automation.runApp(None, browserEnv, options.app, profileDir,
                               ["-reftest", reftestlist],
                               xrePath=options.xrePath,
                               symbolsPath=options.symbolsPath)
    automation.processLeakLog(leakLogFile, options.leakThreshold)
    automation.log.info("\nREFTEST INFO | runreftest.py | Running tests: end.")
  finally:
    if profileDir is not None:
      shutil.rmtree(profileDir)
  sys.exit(status)

def copyExtraFilesToProfile(options, profileDir):
  "Copy extra files or dirs specified on the command line to the testing profile."
  for f in options.extraProfileFiles:
    abspath = getFullPath(f)
    dest = os.path.join(profileDir, os.path.basename(abspath))
    if os.path.isdir(abspath):
      shutil.copytree(abspath, dest)
    else:
      shutil.copy(abspath, dest)

if __name__ == "__main__":
  main()

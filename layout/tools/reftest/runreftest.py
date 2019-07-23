






































"""
Runs the reftest test harness.
"""

import sys, shutil, os, os.path
SCRIPT_DIRECTORY = os.path.abspath(os.path.realpath(os.path.dirname(sys.argv[0])))
sys.path.append(SCRIPT_DIRECTORY)
from automation import Automation
from automationutils import *
from optparse import OptionParser
from tempfile import mkdtemp

class RefTest(object):

  oldcwd = os.getcwd()

  def __init__(self, automation):
    self.automation = automation
    os.chdir(SCRIPT_DIRECTORY)

  def getFullPath(self, path):
    "Get an absolute path relative to self.oldcwd."
    return os.path.normpath(os.path.join(self.oldcwd, os.path.expanduser(path)))

  def createReftestProfile(self, options, profileDir):
    "Sets up a profile for reftest."

    
    prefsFile = open(os.path.join(profileDir, "user.js"), "w")
    prefsFile.write("""user_pref("browser.dom.window.dump.enabled", true);
    """)
    prefsFile.write('user_pref("reftest.timeout", %d);\n' % (options.timeout * 1000))
    prefsFile.write('user_pref("ui.caretBlinkTime", -1);\n')

    for v in options.extraPrefs:
      thispref = v.split("=")
      if len(thispref) < 2:
        print "Error: syntax error in --setpref=" + v
        sys.exit(1)
      part = 'user_pref("%s", %s);\n' % (thispref[0], thispref[1])
      prefsFile.write(part)
    
    prefsFile.write('user_pref("dom.max_script_run_time", 0);')
    prefsFile.write('user_pref("dom.max_chrome_script_run_time", 0);')
    prefsFile.close()

    
    profileExtensionsPath = os.path.join(profileDir, "extensions")
    os.mkdir(profileExtensionsPath)
    reftestExtensionPath = os.path.join(SCRIPT_DIRECTORY, "reftest")
    extFile = open(os.path.join(profileExtensionsPath, "reftest@mozilla.org"), "w")
    extFile.write(reftestExtensionPath)
    extFile.close()

  def runTests(self, manifest, options):
    debuggerInfo = getDebuggerInfo(self.oldcwd, options.debugger, options.debuggerArgs,
        options.debuggerInteractive);

    profileDir = None
    try:
      profileDir = mkdtemp()
      self.createReftestProfile(options, profileDir)
      self.copyExtraFilesToProfile(options, profileDir)

      
      browserEnv = self.automation.environment(xrePath = options.xrePath)
      browserEnv["XPCOM_DEBUG_BREAK"] = "stack"

      
      leakLogFile = os.path.join(profileDir, "runreftest_leaks.log")
      browserEnv["XPCOM_MEM_BLOAT_LOG"] = leakLogFile

      
      
      self.automation.log.info("REFTEST INFO | runreftest.py | Performing extension manager registration: start.\n")
      
      status = self.automation.runApp(None, browserEnv, options.app, profileDir,
                                 ["-silent"],
                                 utilityPath = options.utilityPath,
                                 xrePath=options.xrePath,
                                 symbolsPath=options.symbolsPath)
      
      self.automation.log.info("\nREFTEST INFO | runreftest.py | Performing extension manager registration: end.")

      
      
      if os.path.exists(leakLogFile):
        os.remove(leakLogFile)

      
      self.automation.log.info("REFTEST INFO | runreftest.py | Running tests: start.\n")
      reftestlist = self.getFullPath(manifest)
      status = self.automation.runApp(None, browserEnv, options.app, profileDir,
                                 ["-reftest", reftestlist],
                                 utilityPath = options.utilityPath,
                                 xrePath=options.xrePath,
                                 debuggerInfo=debuggerInfo,
                                 symbolsPath=options.symbolsPath,
                                 
                                 
                                 timeout=options.timeout + 30.0)
      processLeakLog(leakLogFile, options.leakThreshold)
      self.automation.log.info("\nREFTEST INFO | runreftest.py | Running tests: end.")
    finally:
      if profileDir:
        shutil.rmtree(profileDir)
    return status

  def copyExtraFilesToProfile(self, options, profileDir):
    "Copy extra files or dirs specified on the command line to the testing profile."
    for f in options.extraProfileFiles:
      abspath = self.getFullPath(f)
      dest = os.path.join(profileDir, os.path.basename(abspath))
      if os.path.isdir(abspath):
        shutil.copytree(abspath, dest)
      else:
        shutil.copy(abspath, dest)


def main():
  automation = Automation()
  parser = OptionParser()
  reftest = RefTest(automation)

  
  addCommonOptions(parser, 
                   defaults=dict(zip(automation.__all__, 
                            [getattr(automation, x) for x in automation.__all__])))
  automation.addCommonOptions(parser)
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
                    default = 5 * 60, 
                    help = "reftest will timeout in specified number of seconds. [default %default s].")
  parser.add_option("--leak-threshold",
                    action = "store", type = "int", dest = "leakThreshold",
                    default = 0,
                    help = "fail if the number of bytes leaked through "
                           "refcounted objects (or bytes in classes with "
                           "MOZ_COUNT_CTOR and MOZ_COUNT_DTOR) is greater "
                           "than the given number")
  parser.add_option("--utility-path",
                    action = "store", type = "string", dest = "utilityPath",
                    default = automation.DIST_BIN,
                    help = "absolute path to directory containing utility "
                           "programs (xpcshell, ssltunnel, certutil)")

  options, args = parser.parse_args()
  if len(args) != 1:
    print >>sys.stderr, "No reftest.list specified."
    sys.exit(1)

  options.app = reftest.getFullPath(options.app)
  if not os.path.exists(options.app):
    print """Error: Path %(app)s doesn't exist.
Are you executing $objdir/_tests/reftest/runreftest.py?""" \
            % {"app": options.app}
    sys.exit(1)

  if options.xrePath is None:
    options.xrePath = os.path.dirname(options.app)
  else:
    
    options.xrePath = reftest.getFullPath(options.xrePath)

  if options.symbolsPath:
    options.symbolsPath = reftest.getFullPath(options.symbolsPath)
  options.utilityPath = reftest.getFullPath(options.utilityPath)

  sys.exit(reftest.runTests(args[0], options))
  
if __name__ == "__main__":
  main()

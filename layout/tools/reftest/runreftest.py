






































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

  def getFullPath(self, path):
    "Get an absolute path relative to self.oldcwd."
    return os.path.normpath(os.path.join(self.oldcwd, os.path.expanduser(path)))

  def getManifestPath(self, path):
    "Get the path of the manifest, and for remote testing this function is subclassed to point to remote manifest"
    path = self.getFullPath(path)
    if os.path.isdir(path):
      defaultManifestPath = os.path.join(path, 'reftest.list')
      if os.path.exists(defaultManifestPath):
        path = defaultManifestPath
    return path

  def createReftestProfile(self, options, profileDir, server='localhost'):
    "Sets up a profile for reftest."

    self.automation.setupPermissionsDatabase(profileDir,
      {'allowXULXBL': [(server, True), ('<file>', True)]})

    
    
    
    prefsFile = open(os.path.join(profileDir, "user.js"), "a")
    prefsFile.write('user_pref("reftest.timeout", %d);\n' % (options.timeout * 1000))

    if options.totalChunks != None:
      prefsFile.write('user_pref("reftest.totalChunks", %d);\n' % options.totalChunks)
    if options.thisChunk != None:
      prefsFile.write('user_pref("reftest.thisChunk", %d);\n' % options.thisChunk)
    if options.logFile != None:
      prefsFile.write('user_pref("reftest.logFile", "%s");\n' % options.logFile)

    for v in options.extraPrefs:
      thispref = v.split("=")
      if len(thispref) < 2:
        print "Error: syntax error in --setpref=" + v
        sys.exit(1)
      part = 'user_pref("%s", %s);\n' % (thispref[0], thispref[1])
      prefsFile.write(part)
    prefsFile.close()

    
    self.automation.installExtension(os.path.join(SCRIPT_DIRECTORY, "reftest"),
                                                  profileDir,
                                                  "reftest@mozilla.org")


  def registerExtension(self, browserEnv, options, profileDir, extraArgs = ['-silent']):
    
    
    self.automation.log.info("REFTEST INFO | runreftest.py | Performing extension manager registration: start.\n")
    
    status = self.automation.runApp(None, browserEnv, options.app, profileDir,
                                 extraArgs,
                                 utilityPath = options.utilityPath,
                                 xrePath=options.xrePath,
                                 symbolsPath=options.symbolsPath)
    
    self.automation.log.info("\nREFTEST INFO | runreftest.py | Performing extension manager registration: end.")

    
    
    if os.path.exists(self.leakLogFile):
      os.remove(self.leakLogFile)

  def buildBrowserEnv(self, options, profileDir):
    browserEnv = self.automation.environment(xrePath = options.xrePath)
    browserEnv["XPCOM_DEBUG_BREAK"] = "stack"

    
    self.leakLogFile = os.path.join(profileDir, "runreftest_leaks.log")
    browserEnv["XPCOM_MEM_BLOAT_LOG"] = self.leakLogFile
    return browserEnv

  def cleanup(self, profileDir):
    if profileDir:
      shutil.rmtree(profileDir)

  def runTests(self, testPath, options):
    debuggerInfo = getDebuggerInfo(self.oldcwd, options.debugger, options.debuggerArgs,
        options.debuggerInteractive);

    profileDir = None
    try:
      profileDir = mkdtemp()
      self.copyExtraFilesToProfile(options, profileDir)
      self.createReftestProfile(options, profileDir)
      self.installExtensionsToProfile(options, profileDir)

      
      browserEnv = self.buildBrowserEnv(options, profileDir)

      self.registerExtension(browserEnv, options, profileDir)

      
      self.automation.log.info("REFTEST INFO | runreftest.py | Running tests: start.\n")
      reftestlist = self.getManifestPath(testPath)
      status = self.automation.runApp(None, browserEnv, options.app, profileDir,
                                 ["-reftest", reftestlist],
                                 utilityPath = options.utilityPath,
                                 xrePath=options.xrePath,
                                 debuggerInfo=debuggerInfo,
                                 symbolsPath=options.symbolsPath,
                                 
                                 
                                 timeout=options.timeout + 30.0)
      processLeakLog(self.leakLogFile, options.leakThreshold)
      self.automation.log.info("\nREFTEST INFO | runreftest.py | Running tests: end.")
    finally:
      self.cleanup(profileDir)
    return status

  def copyExtraFilesToProfile(self, options, profileDir):
    "Copy extra files or dirs specified on the command line to the testing profile."
    for f in options.extraProfileFiles:
      abspath = self.getFullPath(f)
      if os.path.isfile(abspath):
        shutil.copy2(abspath, profileDir)
      elif os.path.isdir(abspath):
        dest = os.path.join(profileDir, os.path.basename(abspath))
        shutil.copytree(abspath, dest)
      else:
        self.automation.log.warning("WARNING | runreftest.py | Failed to copy %s to profile", abspath)
        continue

  def installExtensionsToProfile(self, options, profileDir):
    "Install application distributed extensions and specified on the command line ones to testing profile."
    
    distExtDir = os.path.join(options.app[ : options.app.rfind(os.sep)], "distribution", "extensions")
    if os.path.isdir(distExtDir):
      for f in os.listdir(distExtDir):
        self.automation.installExtension(os.path.join(distExtDir, f), profileDir)

    
    for f in options.extensionsToInstall:
      self.automation.installExtension(self.getFullPath(f), profileDir)


class ReftestOptions(OptionParser):

  def __init__(self, automation):
    self._automation = automation
    OptionParser.__init__(self)
    defaults = {}

    
    addCommonOptions(self, 
                     defaults=dict(zip(self._automation.__all__, 
                            [getattr(self._automation, x) for x in self._automation.__all__])))
    self._automation.addCommonOptions(self)
    self.add_option("--appname",
                    action = "store", type = "string", dest = "app",
                    default = os.path.join(SCRIPT_DIRECTORY, automation.DEFAULT_APP),
                    help = "absolute path to application, overriding default")
    self.add_option("--extra-profile-file",
                    action = "append", dest = "extraProfileFiles",
                    default = [],
                    help = "copy specified files/dirs to testing profile")
    self.add_option("--timeout",              
                    action = "store", dest = "timeout", type = "int", 
                    default = 5 * 60, 
                    help = "reftest will timeout in specified number of seconds. [default %default s].")
    self.add_option("--leak-threshold",
                    action = "store", type = "int", dest = "leakThreshold",
                    default = 0,
                    help = "fail if the number of bytes leaked through "
                           "refcounted objects (or bytes in classes with "
                           "MOZ_COUNT_CTOR and MOZ_COUNT_DTOR) is greater "
                           "than the given number")
    self.add_option("--utility-path",
                    action = "store", type = "string", dest = "utilityPath",
                    default = self._automation.DIST_BIN,
                    help = "absolute path to directory containing utility "
                           "programs (xpcshell, ssltunnel, certutil)")
    defaults["utilityPath"] = self._automation.DIST_BIN

    self.add_option("--total-chunks",
                    type = "int", dest = "totalChunks",
                    help = "how many chunks to split the tests up into")
    defaults["totalChunks"] = None

    self.add_option("--this-chunk",
                    type = "int", dest = "thisChunk",
                    help = "which chunk to run between 1 and --total-chunks")
    defaults["thisChunk"] = None

    self.add_option("--log-file",
                    action = "store", type = "string", dest = "logFile",
                    default = None,
                    help = "file to log output to in addition to stdout")
    defaults["logFile"] = None
 
    self.add_option("--skip-slow-tests",
                    dest = "skipSlowTests", action = "store_true",
                    help = "skip tests marked as slow when running")
    defaults["skipSlowTests"] = False

    self.add_option("--install-extension",
                    action = "append", dest = "extensionsToInstall",
                    help = "install the specified extension in the testing profile."
                           "The extension file's name should be <id>.xpi where <id> is"
                           "the extension's id as indicated in its install.rdf."
                           "An optional path can be specified too.")
    defaults["extensionsToInstall"] = []

    self.set_defaults(**defaults)

def main():
  automation = Automation()
  parser = ReftestOptions(automation)
  reftest = RefTest(automation)

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

  if options.symbolsPath and not isURL(options.symbolsPath):
    options.symbolsPath = reftest.getFullPath(options.symbolsPath)
  options.utilityPath = reftest.getFullPath(options.utilityPath)

  if options.totalChunks is not None and options.thisChunk is None:
    print "thisChunk must be specified when totalChunks is specified"
    sys.exit(1)

  if options.totalChunks:
    if not 1 <= options.thisChunk <= options.totalChunks:
      print "thisChunk must be between 1 and totalChunks"
      sys.exit(1)
  
  if options.logFile:
    options.logFile = reftest.getFullPath(options.logFile)

  sys.exit(reftest.runTests(args[0], options))
  
if __name__ == "__main__":
  main()

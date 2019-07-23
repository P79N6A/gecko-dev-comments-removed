






































import re, sys, os, os.path, logging, shutil
from glob import glob
from optparse import OptionParser
from subprocess import Popen, PIPE, STDOUT
from tempfile import mkdtemp

from automationutils import *


log = logging.getLogger()
handler = logging.StreamHandler(sys.stdout)
log.setLevel(logging.INFO)
log.addHandler(handler)

oldcwd = os.getcwd()

def readManifest(manifest):
  """Given a manifest file containing a list of test directories,
  return a list of absolute paths to the directories contained within."""
  manifestdir = os.path.dirname(manifest)
  testdirs = []
  try:
    f = open(manifest, "r")
    for line in f:
      dir = line.rstrip()
      path = os.path.join(manifestdir, dir)
      if os.path.isdir(path):
        testdirs.append(path)
    f.close()
  except:
    pass 
  return testdirs

def runTests(xpcshell, xrePath=None, symbolsPath=None,
             manifest=None, testdirs=[], testPath=None,
             interactive=False, logfiles=True,
             debuggerInfo=None):
  """Run xpcshell tests.

  |xpcshell|, is the xpcshell executable to use to run the tests.
  |xrePath|, if provided, is the path to the XRE to use.
  |symbolsPath|, if provided is the path to a directory containing
    breakpad symbols for processing crashes in tests.
  |manifest|, if provided, is a file containing a list of
    test directories to run.
  |testdirs|, if provided, is a list of absolute paths of test directories.
    No-manifest only option.
  |testPath|, if provided, indicates a single path and/or test to run.
  |interactive|, if set to True, indicates to provide an xpcshell prompt
    instead of automatically executing the test.
  |logfiles|, if set to False, indicates not to save output to log files.
    Non-interactive only option.
  |debuggerInfo|, if set, specifies the debugger and debugger arguments
    that will be used to launch xpcshell.
  """

  if not testdirs and not manifest:
    
    print >>sys.stderr, "Error: No test dirs or test manifest specified!"
    return False

  passCount = 0
  failCount = 0

  testharnessdir = os.path.dirname(os.path.abspath(__file__))
  xpcshell = os.path.abspath(xpcshell)
  
  httpdJSPath = os.path.join(os.path.dirname(xpcshell), "components", "httpd.js").replace("\\", "/");

  env = dict(os.environ)
  
  env["XPCOM_DEBUG_BREAK"] = "stack-and-abort"
  
  env["MOZ_CRASHREPORTER_NO_REPORT"] = "1"

  if xrePath is None:
    xrePath = os.path.dirname(xpcshell)
  else:
    xrePath = os.path.abspath(xrePath)
  if sys.platform == 'win32':
    env["PATH"] = env["PATH"] + ";" + xrePath
  elif sys.platform in ('os2emx', 'os2knix'):
    os.environ["BEGINLIBPATH"] = xrePath + ";" + env["BEGINLIBPATH"]
    os.environ["LIBPATHSTRICT"] = "T"
  elif sys.platform == 'osx':
    env["DYLD_LIBRARY_PATH"] = xrePath
  else: 
    env["LD_LIBRARY_PATH"] = xrePath

  
  
  if interactive:
    xpcsRunArgs = [
      '-e', 'print("To start the test, type |_execute_test();|.");',
      '-i']
    pStdout = None
    pStderr = None
  else:
    xpcsRunArgs = ['-e', '_execute_test();']
    if (debuggerInfo and debuggerInfo["interactive"]):
      pStdout = None
      pStderr = None
    else:
      if sys.platform == 'os2emx':
        pStdout = None
      else:
        pStdout = PIPE
      pStderr = STDOUT

  
  xpcsCmd = [xpcshell, '-g', xrePath, '-j', '-s'] + \
            ['-e', 'const _HTTPD_JS_PATH = "%s";' % httpdJSPath,
             '-f', os.path.join(testharnessdir, 'head.js')]

  if debuggerInfo:
    xpcsCmd = [debuggerInfo["path"]] + debuggerInfo["args"] + xpcsCmd

  
  
  singleFile = None
  if testPath:
    if testPath.endswith('.js'):
      
      if testPath.find('/') == -1:
        
        singleFile = testPath
        testPath = None
      else:
        
        
        testPath = testPath.rsplit('/', 1)
        singleFile = testPath[1]
        testPath = testPath[0]
    else:
      
      
      testPath = testPath.rstrip("/")

  
  if manifest is not None:
    testdirs = readManifest(os.path.abspath(manifest))

  
  for testdir in testdirs:
    if testPath and not testdir.endswith(testPath):
      continue

    testdir = os.path.abspath(testdir)

    
    testHeadFiles = []
    for f in sorted(glob(os.path.join(testdir, "head_*.js"))):
      if os.path.isfile(f):
        testHeadFiles += [f]
    testTailFiles = []
    
    
    for f in reversed(sorted(glob(os.path.join(testdir, "tail_*.js")))):
      if os.path.isfile(f):
        testTailFiles += [f]

    
    testfiles = sorted(glob(os.path.join(testdir, "test_*.js")))
    if singleFile:
      if singleFile in [os.path.basename(x) for x in testfiles]:
        testfiles = [os.path.join(testdir, singleFile)]
      else: 
        continue

    cmdH = ", ".join(['"' + f.replace('\\', '/') + '"'
                       for f in testHeadFiles])
    cmdT = ", ".join(['"' + f.replace('\\', '/') + '"'
                       for f in testTailFiles])
    cmdH = xpcsCmd + \
           ['-e', 'const _HEAD_FILES = [%s];' % cmdH] + \
           ['-e', 'const _TAIL_FILES = [%s];' % cmdT]

    
    for test in testfiles:
      
      cmdT = ['-e', 'const _TEST_FILE = ["%s"];' %
                      os.path.join(testdir, test).replace('\\', '/')]

      
      profileDir = None
      try:
        profileDir = mkdtemp()
        env["XPCSHELL_TEST_PROFILE_DIR"] = profileDir

        
        leakLogFile = os.path.join(profileDir, "runxpcshelltests_leaks.log")
        env["XPCOM_MEM_LEAK_LOG"] = leakLogFile

        proc = Popen(cmdH + cmdT + xpcsRunArgs,
                     stdout=pStdout, stderr=pStderr, env=env, cwd=testdir)
        
        stdout, stderr = proc.communicate()

        if interactive:
          
          return True

        if proc.returncode != 0 or (stdout and re.search("^TEST-UNEXPECTED-FAIL", stdout, re.MULTILINE)):
          print """TEST-UNEXPECTED-FAIL | %s | test failed (with xpcshell return code: %d), see following log:
  >>>>>>>
  %s
  <<<<<<<""" % (test, proc.returncode, stdout)
          checkForCrashes(testdir, symbolsPath, testName=test)
          failCount += 1
        else:
          print "TEST-PASS | %s | test passed" % test
          passCount += 1

        dumpLeakLog(leakLogFile, True)

        if logfiles and stdout:
          try:
            f = open(test + ".log", "w")
            f.write(stdout)

            if os.path.exists(leakLogFile):
              leaks = open(leakLogFile, "r")
              f.write(leaks.read())
              leaks.close()
          finally:
            if f:
              f.close()
      finally:
        if profileDir:
          shutil.rmtree(profileDir)

  if passCount == 0 and failCount == 0:
    print "TEST-UNEXPECTED-FAIL | runxpcshelltests.py | No tests run. Did you pass an invalid --test-path?"
    failCount = 1

  print """INFO | Result summary:
INFO | Passed: %d
INFO | Failed: %d""" % (passCount, failCount)

  return failCount == 0

def main():
  """Process command line arguments and call runTests() to do the real work."""
  parser = OptionParser()

  addCommonOptions(parser)
  parser.add_option("--interactive",
                    action="store_true", dest="interactive", default=False,
                    help="don't automatically run tests, drop to an xpcshell prompt")
  parser.add_option("--logfiles",
                    action="store_true", dest="logfiles", default=True,
                    help="create log files (default, only used to override --no-logfiles)")
  parser.add_option("--manifest",
                    type="string", dest="manifest", default=None,
                    help="Manifest of test directories to use")
  parser.add_option("--no-logfiles",
                    action="store_false", dest="logfiles",
                    help="don't create log files")
  parser.add_option("--test-path",
                    type="string", dest="testPath", default=None,
                    help="single path and/or test filename to test")
  options, args = parser.parse_args()

  if len(args) < 2 and options.manifest is None or \
     (len(args) < 1 and options.manifest is not None):
    print >>sys.stderr, """Usage: %s <path to xpcshell> <test dirs>
  or: %s --manifest=test.manifest <path to xpcshell>""" % (sys.argv[0],
                                                           sys.argv[0])
    sys.exit(1)

  debuggerInfo = getDebuggerInfo(oldcwd, options.debugger, options.debuggerArgs,
    options.debuggerInteractive);

  if options.interactive and not options.testPath:
    print >>sys.stderr, "Error: You must specify a test filename in interactive mode!"
    sys.exit(1)

  if not runTests(args[0],
                  xrePath=options.xrePath,
                  symbolsPath=options.symbolsPath,
                  manifest=options.manifest,
                  testdirs=args[1:],
                  testPath=options.testPath,
                  interactive=options.interactive,
                  logfiles=options.logfiles,
                  debuggerInfo=debuggerInfo):
    sys.exit(1)

if __name__ == '__main__':
  main()

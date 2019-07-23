






































import re, sys, os, os.path, logging, shutil
import tempfile
from glob import glob
from optparse import OptionParser
from subprocess import Popen, PIPE, STDOUT
from tempfile import mkdtemp

from automationutils import addCommonOptions, checkForCrashes


log = logging.getLogger()
handler = logging.StreamHandler(sys.stdout)
log.setLevel(logging.INFO)
log.addHandler(handler)

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

def runTests(xpcshell, testdirs=[], xrePath=None, testPath=None,
             manifest=None, interactive=False, symbolsPath=None):
  """Run the tests in |testdirs| using the |xpcshell| executable.

  |xrePath|, if provided, is the path to the XRE to use.
  |testPath|, if provided, indicates a single path and/or test to run.
  |manifest|, if provided, is a file containing a list of
    test directories to run.
  |interactive|, if set to True, indicates to provide an xpcshell prompt
    instead of automatically executing the test.
  |symbolsPath|, if provided is the path to a directory containing
    breakpad symbols for processing crashes in tests.
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

  
  
  leakLogFile = os.path.join(tempfile.gettempdir(), "runxpcshelltests_leaks.log")
  env["XPCOM_MEM_LEAK_LOG"] = leakLogFile

  def processLeakLog(leakLogFile):
    """Process the leak log."""
    
    if not os.path.exists(leakLogFile):
      return None

    leaks = open(leakLogFile, "r")
    leakReport = leaks.read()
    leaks.close()

    
    if "0 TOTAL " in leakReport:
      
      print leakReport.rstrip("\n")

    return leakReport

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
    if sys.platform == 'os2emx':
      pStdout = None 
    else:
      pStdout = PIPE
    pStderr = STDOUT

  
  xpcsCmd = [xpcshell, '-g', xrePath, '-j', '-s'] + \
            ['-e', 'const _HTTPD_JS_PATH = "%s";' % httpdJSPath,
             '-f', os.path.join(testharnessdir, 'head.js')]
  xpcsTailFile = [os.path.join(testharnessdir, 'tail.js')]

  
  
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
                       for f in (testTailFiles + xpcsTailFile)])
    cmdH = xpcsCmd + \
           ['-e', 'const _HEAD_FILES = [%s];' % cmdH] + \
           ['-e', 'const _TAIL_FILES = [%s];' % cmdT]

    
    for test in testfiles:
      
      cmdT = ['-e', 'const _TEST_FILE = ["%s"];' %
                      os.path.join(testdir, test).replace('\\', '/')]
      
      profd = mkdtemp()
      env["XPCSHELL_TEST_PROFILE_DIR"] = profd

      proc = Popen(cmdH + cmdT + xpcsRunArgs,
                   stdout=pStdout, stderr=pStderr, env=env, cwd=testdir)
      
      stdout, stderr = proc.communicate()

      shutil.rmtree(profd, True)

      if interactive:
        
        return True

      if proc.returncode != 0 or (stdout is not None and re.search("^TEST-UNEXPECTED-FAIL", stdout, re.MULTILINE)):
        print """TEST-UNEXPECTED-FAIL | %s | test failed (with xpcshell return code: %d), see following log:
  >>>>>>>
  %s
  <<<<<<<""" % (test, proc.returncode, stdout)
        checkForCrashes(testdir, symbolsPath, testName=test)
        failCount += 1
      else:
        print "TEST-PASS | %s | test passed" % test
        passCount += 1

      leakReport = processLeakLog(leakLogFile)

      if stdout is not None:
        try:
          f = open(test + '.log', 'w')
          f.write(stdout)
          if leakReport:
            f.write(leakReport)
        finally:
          if f:
            f.close()

      
      
      if os.path.exists(leakLogFile):
        os.remove(leakLogFile)

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
  parser.add_option("--test-path",
                    action="store", type="string", dest="testPath",
                    default=None, help="single path and/or test filename to test")
  parser.add_option("--interactive",
                    action="store_true", dest="interactive", default=False,
                    help="don't automatically run tests, drop to an xpcshell prompt")
  parser.add_option("--manifest",
                    action="store", type="string", dest="manifest",
                    default=None, help="Manifest of test directories to use")
  options, args = parser.parse_args()

  if len(args) < 2 and options.manifest is None or \
     (len(args) < 1 and options.manifest is not None):
    print >>sys.stderr, """Usage: %s <path to xpcshell> <test dirs>
  or: %s --manifest=test.manifest <path to xpcshell>""" % (sys.argv[0],
                                                           sys.argv[0])
    sys.exit(1)

  if options.interactive and not options.testPath:
    print >>sys.stderr, "Error: You must specify a test filename in interactive mode!"
    sys.exit(1)

  if not runTests(args[0], testdirs=args[1:],
                  xrePath=options.xrePath,
                  testPath=options.testPath,
                  interactive=options.interactive,
                  manifest=options.manifest,
                  symbolsPath=options.symbolsPath):
    sys.exit(1)

if __name__ == '__main__':
  main()

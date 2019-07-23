





































import sys, os, os.path
from glob import glob
from optparse import OptionParser
from subprocess import Popen, PIPE, STDOUT

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

def runTests(xpcshell, testdirs=[], xrePath=None, testFile=None,
             manifest=None, interactive=False, keepGoing=False):
  """Run the tests in |testdirs| using the |xpcshell| executable.
  |xrePath|, if provided, is the path to the XRE to use.
  |testFile|, if provided, indicates a single test to run.
  |manifeest|, if provided, is a file containing a list of
    test directories to run.
  |interactive|, if set to True, indicates to provide an xpcshell prompt
    instead of automatically executing  the test.
  |keepGoing|, if set to True, indicates that if a test fails
    execution should continue."""
  testharnessdir = os.path.dirname(os.path.abspath(__file__))
  xpcshell = os.path.abspath(xpcshell)
  
  httpdJSPath = os.path.join(os.path.dirname(xpcshell), "components", "httpd.js").replace("\\", "/");

  env = dict(os.environ)
  
  env["XPCOM_DEBUG_BREAK"] = "stack-and-abort"

  if not testdirs and not manifest:
    
    raise Exception("No test dirs or test manifest specified!")

  if xrePath is None:
    xrePath = os.path.dirname(xpcshell)
  else:
    xrePath = os.path.abspath(xrePath)
  if sys.platform == 'win32':
    env["PATH"] = env["PATH"] + ";" + xrePath
  elif sys.platform == 'osx':
    env["DYLD_LIBRARY_PATH"] = xrePath
  else: 
    env["LD_LIBRARY_PATH"] = xrePath
  args = [xpcshell, '-g', xrePath, '-j', '-s']

  headfiles = ['-f', os.path.join(testharnessdir, 'head.js'),
               '-e', 'function do_load_httpd_js() {load("%s");}' % httpdJSPath]
  tailfiles = ['-f', os.path.join(testharnessdir, 'tail.js')]
  if not interactive:
    tailfiles += ['-e', '_execute_test();']

  
  
  
  singleDir = None
  if testFile and testFile.find('/') != -1:
    
    bits = testFile.split('/', 1)
    singleDir = bits[0]
    testFile = bits[1]

  if manifest is not None:
    testdirs = readManifest(os.path.abspath(manifest))

  success = True
  for testdir in testdirs:
    if singleDir and singleDir != os.path.basename(testdir):
      continue
    testdir = os.path.abspath(testdir)

    
    testheadfiles = []
    for f in sorted(glob(os.path.join(testdir, "head_*.js"))):
      if os.path.isfile(f):
        testheadfiles += ['-f', f]
    testtailfiles = []
    for f in sorted(glob(os.path.join(testdir, "tail_*.js"))):
      if os.path.isfile(f):
        testtailfiles += ['-f', f]

    
    
    testfiles = sorted(glob(os.path.join(testdir, "test_*.js")))
    if testFile:
      if testFile in [os.path.basename(x) for x in testfiles]:
        testfiles = [os.path.join(testdir, testFile)]
      else: 
        continue
    for test in testfiles:
      pstdout = PIPE
      pstderr = STDOUT
      interactiveargs = []
      if interactive:
        pstdout = None
        pstderr = None
        interactiveargs = ['-e', 'print("To start the test, type _execute_test();")', '-i']
      full_args = args + headfiles + testheadfiles \
                  + ['-f', test] \
                  + tailfiles + testtailfiles + interactiveargs
      proc = Popen(full_args, stdout=pstdout, stderr=pstderr,
                   env=env, cwd=testdir)
      stdout, stderr = proc.communicate()

      if interactive:
        
        return True

      if proc.returncode != 0 or stdout.find("*** PASS") == -1:
        print """TEST-UNEXPECTED-FAIL | %s | test failed, see log
  %s.log:
  >>>>>>>
  %s
  <<<<<<<""" % (test, test, stdout)
        if not keepGoing:
          return False
        success = False
      else:
        print "TEST-PASS | %s | all tests passed" % test
  return success

def main():
  """Process command line arguments and call runTests() to do the real work."""
  parser = OptionParser()
  parser.add_option("--xre-path",
                    action="store", type="string", dest="xrePath", default=None,
                    help="absolute path to directory containing XRE (probably xulrunner)")
  parser.add_option("--test",
                    action="store", type="string", dest="testFile",
                    default=None, help="single test filename to test")
  parser.add_option("--interactive",
                    action="store_true", dest="interactive", default=False,
                    help="don't automatically run tests, drop to an xpcshell prompt")
  parser.add_option("--keep-going",
                    action="store_true", dest="keepGoing", default=False,
                    help="continue running tests past the first failure")
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

  if options.interactive and not options.testFile:
    print >>sys.stderr, "Error: You must specify a test filename in interactive mode!"
    sys.exit(1)

  if not runTests(args[0], testdirs=args[1:],
                  xrePath=options.xrePath,
                  testFile=options.testFile,
                  interactive=options.interactive,
                  keepGoing=options.keepGoing,
                  manifest=options.manifest):
    sys.exit(1)

if __name__ == '__main__':
  main()

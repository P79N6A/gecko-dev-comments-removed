





































import sys, os, os.path
from glob import glob
from optparse import OptionParser
from subprocess import Popen, PIPE, STDOUT

def runTests(xpcshell, topsrcdir, testdirs, xrePath=None, testFile=None, interactive=False):
  """Run the tests in |testdirs| using the |xpcshell| executable.
  If provided, |xrePath| is the path to the XRE to use. If provided,
  |testFile| indicates a single test to run. |interactive|, if set to True,
  indicates to provide an xpcshell prompt instead of automatically executing
  the test."""
  xpcshell = os.path.abspath(xpcshell)
  env = dict(os.environ)
  env["NATIVE_TOPSRCDIR"] = os.path.normpath(topsrcdir)
  env["TOPSRCDIR"] = topsrcdir
  
  env["XPCOM_DEBUG_BREAK"] = "stack-and-abort"

  if xrePath is None:
    xrePath = os.path.dirname(xpcshell)
  if sys.platform == 'win32':
    env["PATH"] = env["PATH"] + ";" + xrePath
  elif sys.platform == 'osx':
    env["DYLD_LIBRARY_PATH"] = xrePath
  else: 
    env["LD_LIBRARY_PATH"] = xrePath
  args = [xpcshell, '-g', xrePath, '-j', '-s']

  testharnessdir = os.path.dirname(os.path.abspath(sys.argv[0]))
  headfiles = ['-f', os.path.join(testharnessdir, 'head.js')]
  tailfiles = ['-f', os.path.join(testharnessdir, 'tail.js')]
  if not interactive:
    tailfiles += ['-e', '_execute_test();']

  
  
  
  singleDir = None
  if testFile and testFile.find('/') != -1:
    
    bits = testFile.split('/', 1)
    singleDir = bits[0]
    testFile = bits[1]
  for testdir in testdirs:
    if singleDir and singleDir != os.path.basename(testdir):
      continue

    
    testheadfiles = []
    for f in glob(os.path.join(testdir, "head_*.js")):
      if os.path.isfile(f):
        testheadfiles += ['-f', f]
    testtailfiles = []
    for f in glob(os.path.join(testdir, "tail_*.js")):
      if os.path.isfile(f):
        testtailfiles += ['-f', f]

    
    
    testfiles = glob(os.path.join(testdir, "test_*.js"))
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
      proc = Popen(args + headfiles + testheadfiles
                   + ['-f', test]
                   + tailfiles + testtailfiles + interactiveargs,
                   stdout=pstdout, stderr=pstderr, env=env)
      stdout, stderr = proc.communicate()

      if interactive:
        
        return True

      if proc.returncode != 0 or stdout.find("*** PASS") == -1:
        print """TEST-UNEXPECTED-FAIL | %s | test failed, see log
  %s.log:
  >>>>>>>
  %s
  <<<<<<<""" % (test, test, stdout)
        return False

      print "TEST-PASS | %s | all tests passed" % test
  return True

def main():
  """Process command line arguments and call runTests() to do the real work."""
  parser = OptionParser()
  parser.add_option("--xre-path",
                    action="store", type="string", dest="xrePath", default=None,
                    help="absolute path to directory containing XRE (probably xulrunner)")
  parser.add_option("--test",
                    action="store", type="string", dest="testFile", default=None,
                    help="single test filename to test")
  parser.add_option("--interactive",
                    action="store_true", dest="interactive", default=False,
                    help="don't automatically run tests, drop to an xpcshell prompt")
  options, args = parser.parse_args()

  if len(args) < 3:
    print >>sys.stderr, "Usage: %s <path to xpcshell> <topsrcdir> <test dirs>" % sys.argv[0]
    sys.exit(1)

  if options.interactive and not options.testFile:
    print >>sys.stderr, "Error: You must specify a test filename in interactive mode!"
    sys.exit(1)

  if not runTests(args[0], args[1], args[2:], xrePath=options.xrePath, testFile=options.testFile, interactive=options.interactive):
    sys.exit(1)

if __name__ == '__main__':
  main()

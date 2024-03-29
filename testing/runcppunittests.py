





from __future__ import with_statement
import sys, os, tempfile, shutil
from optparse import OptionParser
import manifestparser
import mozprocess
import mozinfo
import mozcrash
import mozfile
import mozlog
from contextlib import contextmanager
from subprocess import PIPE

SCRIPT_DIR = os.path.abspath(os.path.realpath(os.path.dirname(__file__)))

class CPPUnitTests(object):
    
    TEST_PROC_TIMEOUT = 900
    
    TEST_PROC_NO_OUTPUT_TIMEOUT = 300

    def run_one_test(self, prog, env, symbols_path=None, interactive=False):
        """
        Run a single C++ unit test program.

        Arguments:
        * prog: The path to the test program to run.
        * env: The environment to use for running the program.
        * symbols_path: A path to a directory containing Breakpad-formatted
                        symbol files for producing stack traces on crash.

        Return True if the program exits with a zero status, False otherwise.
        """
        basename = os.path.basename(prog)
        self.log.test_start(basename)
        with mozfile.TemporaryDirectory() as tempdir:
            if interactive:
                
                proc = mozprocess.ProcessHandler([prog],
                                                 cwd=tempdir,
                                                 env=env,
                                                 storeOutput=False)
            else:
                proc = mozprocess.ProcessHandler([prog],
                                                 cwd=tempdir,
                                                 env=env,
                                                 storeOutput=True,
                                                 processOutputLine=lambda _: None)
            
            
            proc.run(timeout=CPPUnitTests.TEST_PROC_TIMEOUT,
                     outputTimeout=CPPUnitTests.TEST_PROC_NO_OUTPUT_TIMEOUT)
            proc.wait()
            if proc.output:
                output = "\n%s" % "\n".join(proc.output)
                self.log.process_output(proc.pid, output, command=[prog])
            if proc.timedOut:
                message = "timed out after %d seconds" % CPPUnitTests.TEST_PROC_TIMEOUT
                self.log.test_end(basename, status='TIMEOUT', expected='PASS',
                                  message=message)
                return False
            if mozcrash.check_for_crashes(tempdir, symbols_path,
                                          test_name=basename):
                self.log.test_end(basename, status='CRASH', expected='PASS')
                return False
            result = proc.proc.returncode == 0
            if not result:
                self.log.test_end(basename, status='FAIL', expected='PASS',
                                  message=("test failed with return code %d" %
                                           proc.proc.returncode))
            else:
                self.log.test_end(basename, status='PASS', expected='PASS')
            return result

    def build_core_environment(self, env = {}):
        """
        Add environment variables likely to be used across all platforms, including remote systems.
        """
        env["MOZILLA_FIVE_HOME"] = self.xre_path
        env["MOZ_XRE_DIR"] = self.xre_path
        
        
        env["XPCOM_DEBUG_BREAK"] = "stack-and-abort"
        env["MOZ_CRASHREPORTER_NO_REPORT"] = "1"
        env["MOZ_CRASHREPORTER"] = "1"
        return env

    def build_environment(self):
        """
        Create and return a dictionary of all the appropriate env variables and values.
        On a remote system, we overload this to set different values and are missing things like os.environ and PATH.
        """
        if not os.path.isdir(self.xre_path):
            raise Exception("xre_path does not exist: %s", self.xre_path)
        env = dict(os.environ)
        env = self.build_core_environment(env)
        pathvar = ""
        libpath = self.xre_path
        if mozinfo.os == "linux":
            pathvar = "LD_LIBRARY_PATH"
        elif mozinfo.os == "mac":
            applibpath = os.path.join(os.path.dirname(libpath), 'MacOS')
            if os.path.exists(applibpath):
                
                
                libpath = applibpath
            pathvar = "DYLD_LIBRARY_PATH"
        elif mozinfo.os == "win":
            pathvar = "PATH"
        if pathvar:
            if pathvar in env:
                env[pathvar] = "%s%s%s" % (libpath, os.pathsep, env[pathvar])
            else:
                env[pathvar] = libpath

        
        llvmsym = os.path.join(self.xre_path, "llvm-symbolizer")
        if os.path.isfile(llvmsym):
            env["ASAN_SYMBOLIZER_PATH"] = llvmsym
            self.log.info("ASan using symbolizer at %s" % llvmsym)
        else:
            self.log.info("Failed to find ASan symbolizer at %s" % llvmsym)

        return env

    def run_tests(self, programs, xre_path, symbols_path=None, interactive=False):
        """
        Run a set of C++ unit test programs.

        Arguments:
        * programs: An iterable containing paths to test programs.
        * xre_path: A path to a directory containing a XUL Runtime Environment.
        * symbols_path: A path to a directory containing Breakpad-formatted
                        symbol files for producing stack traces on crash.

        Returns True if all test programs exited with a zero status, False
        otherwise.
        """
        self.xre_path = xre_path
        self.log = mozlog.get_default_logger()
        self.log.suite_start(programs)
        env = self.build_environment()
        pass_count = 0
        fail_count = 0
        for prog in programs:
            single_result = self.run_one_test(prog, env, symbols_path, interactive)
            if single_result:
                pass_count += 1
            else:
                fail_count += 1
        self.log.suite_end()

        
        self.log.info("Result summary:")
        self.log.info("cppunittests INFO | Passed: %d" % pass_count)
        self.log.info("cppunittests INFO | Failed: %d" % fail_count)
        return fail_count == 0

class CPPUnittestOptions(OptionParser):
    def __init__(self):
        OptionParser.__init__(self)
        self.add_option("--xre-path",
                        action = "store", type = "string", dest = "xre_path",
                        default = None,
                        help = "absolute path to directory containing XRE (probably xulrunner)")
        self.add_option("--symbols-path",
                        action = "store", type = "string", dest = "symbols_path",
                        default = None,
                        help = "absolute path to directory containing breakpad symbols, or the URL of a zip file containing symbols")
        self.add_option("--skip-manifest",
                        action = "store", type = "string", dest = "manifest_file",
                        default = None,
                        help = "absolute path to a manifest file")

def extract_unittests_from_args(args, environ):
    """Extract unittests from args, expanding directories as needed"""
    mp = manifestparser.TestManifest(strict=True)
    tests = []
    for p in args:
        if os.path.isdir(p):
            try:
                mp.read(os.path.join(p, 'cppunittest.ini'))
            except IOError:
                tests.extend([os.path.abspath(os.path.join(p, x)) for x in os.listdir(p)])
        else:
            tests.append(os.path.abspath(p))

    
    
    if mozinfo.isWin:
        tests.extend([test['path'] + '.exe' for test in mp.active_tests(exists=False, disabled=False, **environ)])
    else:
        tests.extend([test['path'] for test in mp.active_tests(exists=False, disabled=False, **environ)])

    
    tests = [test for test in tests if os.path.isfile(test)]

    return tests

def update_mozinfo():
    """walk up directories to find mozinfo.json update the info"""
    path = SCRIPT_DIR
    dirs = set()
    while path != os.path.expanduser('~'):
        if path in dirs:
            break
        dirs.add(path)
        path = os.path.split(path)[0]
    mozinfo.find_and_update_from_json(*dirs)

def main():
    parser = CPPUnittestOptions()
    mozlog.commandline.add_logging_group(parser)
    options, args = parser.parse_args()
    if not args:
        print >>sys.stderr, """Usage: %s <test binary> [<test binary>...]""" % sys.argv[0]
        sys.exit(1)
    if not options.xre_path:
        print >>sys.stderr, """Error: --xre-path is required"""
        sys.exit(1)

    log = mozlog.commandline.setup_logging("cppunittests", options,
                                           {"tbpl": sys.stdout})

    update_mozinfo()
    progs = extract_unittests_from_args(args, mozinfo.info)
    options.xre_path = os.path.abspath(options.xre_path)
    if mozinfo.isMac:
        options.xre_path = os.path.join(os.path.dirname(options.xre_path), 'Resources')
    tester = CPPUnitTests()

    try:
        result = tester.run_tests(progs, options.xre_path, options.symbols_path)
    except Exception as e:
        log.error(str(e))
        result = False

    sys.exit(0 if result else 1)

if __name__ == '__main__':
    main()

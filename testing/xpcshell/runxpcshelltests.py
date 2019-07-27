





import copy
import json
import math
import mozdebug
import os
import os.path
import random
import re
import shutil
import signal
import socket
import sys
import time
import traceback
import xml.dom.minidom
from collections import deque
from distutils import dir_util
from multiprocessing import cpu_count
from optparse import OptionParser
from subprocess import Popen, PIPE, STDOUT
from tempfile import mkdtemp, gettempdir
from threading import Timer, Thread, Event, RLock

try:
    import psutil
    HAVE_PSUTIL = True
except ImportError:
    HAVE_PSUTIL = False

from automation import Automation, getGlobalLog, resetGlobalLog
from automationutils import *





LOG_MUTEX = RLock()

HARNESS_TIMEOUT = 5 * 60


NUM_THREADS = int(cpu_count() * 4)

FAILURE_ACTIONS = set(['test_unexpected_fail',
                       'test_unexpected_pass',
                       'javascript_error'])
ACTION_STRINGS = {
    "test_unexpected_fail": "TEST-UNEXPECTED-FAIL",
    "test_known_fail": "TEST-KNOWN-FAIL",
    "test_unexpected_pass": "TEST-UNEXPECTED-PASS",
    "javascript_error": "TEST-UNEXPECTED-FAIL",
    "test_pass": "TEST-PASS",
    "test_info": "TEST-INFO"
}




here = os.path.dirname(__file__)
mozbase = os.path.realpath(os.path.join(os.path.dirname(here), 'mozbase'))

if os.path.isdir(mozbase):
    for package in os.listdir(mozbase):
        sys.path.append(os.path.join(mozbase, package))

import manifestparser
import mozcrash
import mozinfo








_cleanup_encoding_re = re.compile(u'[\x00-\x08\x0b\x0c\x0e-\x1f\x7f-\x9f\\\\]')
def _cleanup_encoding_repl(m):
    c = m.group(0)
    return '\\\\' if c == '\\' else '\\x{0:02X}'.format(ord(c))
def cleanup_encoding(s):
    """S is either a byte or unicode string.  Either way it may
       contain control characters, unpaired surrogates, reserved code
       points, etc.  If it is a byte string, it is assumed to be
       UTF-8, but it may not be *correct* UTF-8.  Produce a byte
       string that can safely be dumped into a (generally UTF-8-coded)
       logfile."""
    if not isinstance(s, unicode):
        s = s.decode('utf-8', 'replace')
    if s.endswith('\n'):
        
        
        s = s[:-1]
    
    s = _cleanup_encoding_re.sub(_cleanup_encoding_repl, s)
    return s.encode('utf-8', 'backslashreplace')

""" Control-C handling """
gotSIGINT = False
def markGotSIGINT(signum, stackFrame):
    global gotSIGINT
    gotSIGINT = True

class XPCShellTestThread(Thread):
    def __init__(self, test_object, event, cleanup_dir_list, retry=True,
            tests_root_dir=None, app_dir_key=None, interactive=False,
            verbose=False, pStdout=None, pStderr=None, keep_going=False,
            log=None, **kwargs):
        Thread.__init__(self)
        self.daemon = True

        self.test_object = test_object
        self.cleanup_dir_list = cleanup_dir_list
        self.retry = retry

        self.appPath = kwargs.get('appPath')
        self.xrePath = kwargs.get('xrePath')
        self.testingModulesDir = kwargs.get('testingModulesDir')
        self.debuggerInfo = kwargs.get('debuggerInfo')
        self.pluginsPath = kwargs.get('pluginsPath')
        self.httpdManifest = kwargs.get('httpdManifest')
        self.httpdJSPath = kwargs.get('httpdJSPath')
        self.headJSPath = kwargs.get('headJSPath')
        self.testharnessdir = kwargs.get('testharnessdir')
        self.profileName = kwargs.get('profileName')
        self.singleFile = kwargs.get('singleFile')
        self.env = copy.deepcopy(kwargs.get('env'))
        self.symbolsPath = kwargs.get('symbolsPath')
        self.logfiles = kwargs.get('logfiles')
        self.xpcshell = kwargs.get('xpcshell')
        self.xpcsRunArgs = kwargs.get('xpcsRunArgs')
        self.failureManifest = kwargs.get('failureManifest')
        self.on_message = kwargs.get('on_message')

        self.tests_root_dir = tests_root_dir
        self.app_dir_key = app_dir_key
        self.interactive = interactive
        self.verbose = verbose
        self.pStdout = pStdout
        self.pStderr = pStderr
        self.keep_going = keep_going
        self.log = log

        
        
        self.passCount = 0
        self.todoCount = 0
        self.failCount = 0

        self.output_lines = []
        self.has_failure_output = False
        self.saw_proc_start = False
        self.saw_proc_end = False

        
        self.event = event
        self.done = False 

    def run(self):
        try:
            self.run_test()
        except Exception as e:
            self.exception = e
            self.traceback = traceback.format_exc()
        else:
            self.exception = None
            self.traceback = None
        if self.retry:
            self.log.info("TEST-INFO | %s | Test failed or timed out, will retry."
                          % self.test_object['name'])
        self.done = True
        self.event.set()

    def kill(self, proc):
        """
          Simple wrapper to kill a process.
          On a remote system, this is overloaded to handle remote process communication.
        """
        return proc.kill()

    def removeDir(self, dirname):
        """
          Simple wrapper to remove (recursively) a given directory.
          On a remote system, we need to overload this to work on the remote filesystem.
        """
        shutil.rmtree(dirname)

    def poll(self, proc):
        """
          Simple wrapper to check if a process has terminated.
          On a remote system, this is overloaded to handle remote process communication.
        """
        return proc.poll()

    def createLogFile(self, test_file, stdout):
        """
          For a given test file and stdout buffer, create a log file.
          On a remote system we have to fix the test name since it can contain directories.
        """
        with open(test_file + ".log", "w") as f:
            f.write(stdout)

    def getReturnCode(self, proc):
        """
          Simple wrapper to get the return code for a given process.
          On a remote system we overload this to work with the remote process management.
        """
        return proc.returncode

    def communicate(self, proc):
        """
          Simple wrapper to communicate with a process.
          On a remote system, this is overloaded to handle remote process communication.
        """
        
        
        
        if proc.stdout:
            while True:
                line = proc.stdout.readline()
                if not line:
                    break
                self.process_line(line)

            if self.saw_proc_start and not self.saw_proc_end:
                self.has_failure_output = True

        return proc.communicate()

    def launchProcess(self, cmd, stdout, stderr, env, cwd, timeout=None):
        """
          Simple wrapper to launch a process.
          On a remote system, this is more complex and we need to overload this function.
        """
        
        
        if HAVE_PSUTIL:
            popen_func = psutil.Popen
        else:
            popen_func = Popen
        proc = popen_func(cmd, stdout=stdout, stderr=stderr,
                    env=env, cwd=cwd)
        return proc

    def checkForCrashes(self,
                        dump_directory,
                        symbols_path,
                        test_name=None):
        """
          Simple wrapper to check for crashes.
          On a remote system, this is more complex and we need to overload this function.
        """
        return mozcrash.check_for_crashes(dump_directory, symbols_path, test_name=test_name)

    def logCommand(self, name, completeCmd, testdir):
        self.log.info("TEST-INFO | %s | full command: %r" % (name, completeCmd))
        self.log.info("TEST-INFO | %s | current directory: %r" % (name, testdir))
        
        
        changedEnv = (set("%s=%s" % i for i in self.env.iteritems())
                      - set("%s=%s" % i for i in os.environ.iteritems()))
        self.log.info("TEST-INFO | %s | environment: %s" % (name, list(changedEnv)))

    def testTimeout(self, test_file, proc):
        if not self.retry:
            self.log.error("TEST-UNEXPECTED-FAIL | %s | Test timed out" % test_file)
        self.done = True
        Automation().killAndGetStackNoScreenshot(proc.pid, self.appPath, self.debuggerInfo)

    def buildCmdTestFile(self, name):
        """
          Build the command line arguments for the test file.
          On a remote system, this may be overloaded to use a remote path structure.
        """
        return ['-e', 'const _TEST_FILE = ["%s"];' %
                  replaceBackSlashes(name)]

    def setupTempDir(self):
        tempDir = mkdtemp()
        self.env["XPCSHELL_TEST_TEMP_DIR"] = tempDir
        if self.interactive:
            self.log.info("TEST-INFO | temp dir is %s" % tempDir)
        return tempDir

    def setupPluginsDir(self):
        if not os.path.isdir(self.pluginsPath):
            return None

        pluginsDir = mkdtemp()
        
        
        
        dir_util.copy_tree(self.pluginsPath, pluginsDir)
        if self.interactive:
            self.log.info("TEST-INFO | plugins dir is %s" % pluginsDir)
        return pluginsDir

    def setupProfileDir(self):
        """
          Create a temporary folder for the profile and set appropriate environment variables.
          When running check-interactive and check-one, the directory is well-defined and
          retained for inspection once the tests complete.

          On a remote system, this may be overloaded to use a remote path structure.
        """
        if self.interactive or self.singleFile:
            profileDir = os.path.join(gettempdir(), self.profileName, "xpcshellprofile")
            try:
                
                self.removeDir(profileDir)
            except:
                pass
            os.makedirs(profileDir)
        else:
            profileDir = mkdtemp()
        self.env["XPCSHELL_TEST_PROFILE_DIR"] = profileDir
        if self.interactive or self.singleFile:
            self.log.info("TEST-INFO | profile dir is %s" % profileDir)
        return profileDir

    def buildCmdHead(self, headfiles, tailfiles, xpcscmd):
        """
          Build the command line arguments for the head and tail files,
          along with the address of the webserver which some tests require.

          On a remote system, this is overloaded to resolve quoting issues over a secondary command line.
        """
        cmdH = ", ".join(['"' + replaceBackSlashes(f) + '"'
                       for f in headfiles])
        cmdT = ", ".join(['"' + replaceBackSlashes(f) + '"'
                       for f in tailfiles])
        return xpcscmd + \
                ['-e', 'const _SERVER_ADDR = "localhost"',
                 '-e', 'const _HEAD_FILES = [%s];' % cmdH,
                 '-e', 'const _TAIL_FILES = [%s];' % cmdT]

    def getHeadAndTailFiles(self, test_object):
        """Obtain the list of head and tail files.

        Returns a 2-tuple. The first element is a list of head files. The second
        is a list of tail files.
        """
        def sanitize_list(s, kind):
            for f in s.strip().split(' '):
                f = f.strip()
                if len(f) < 1:
                    continue

                path = os.path.normpath(os.path.join(test_object['here'], f))
                if not os.path.exists(path):
                    raise Exception('%s file does not exist: %s' % (kind, path))

                if not os.path.isfile(path):
                    raise Exception('%s file is not a file: %s' % (kind, path))

                yield path

        headlist = test_object['head'] if 'head' in test_object else ''
        taillist = test_object['tail'] if 'tail' in test_object else ''
        return (list(sanitize_list(headlist, 'head')),
                list(sanitize_list(taillist, 'tail')))

    def buildXpcsCmd(self, testdir):
        """
          Load the root head.js file as the first file in our test path, before other head, test, and tail files.
          On a remote system, we overload this to add additional command line arguments, so this gets overloaded.
        """
        
        
        if not self.appPath:
            self.appPath = self.xrePath

        self.xpcsCmd = [
            self.xpcshell,
            '-g', self.xrePath,
            '-a', self.appPath,
            '-r', self.httpdManifest,
            '-m',
            '-s',
            '-e', 'const _HTTPD_JS_PATH = "%s";' % self.httpdJSPath,
            '-e', 'const _HEAD_JS_PATH = "%s";' % self.headJSPath
        ]

        if self.testingModulesDir:
            
            sanitized = self.testingModulesDir.replace('\\', '\\\\')
            self.xpcsCmd.extend([
                '-e',
                'const _TESTING_MODULES_DIR = "%s";' % sanitized
            ])

        self.xpcsCmd.extend(['-f', os.path.join(self.testharnessdir, 'head.js')])

        if self.debuggerInfo:
            self.xpcsCmd = [self.debuggerInfo.path] + self.debuggerInfo.args + self.xpcsCmd

        
        
        
        if not self.pluginsPath:
            self.pluginsPath = os.path.join(self.appPath, 'plugins')

        self.pluginsDir = self.setupPluginsDir()
        if self.pluginsDir:
            self.xpcsCmd.extend(['-p', self.pluginsDir])

    def cleanupDir(self, directory, name, xunit_result):
        if not os.path.exists(directory):
            return

        TRY_LIMIT = 25 
                       
        try_count = 0
        while try_count < TRY_LIMIT:
            try:
                self.removeDir(directory)
            except OSError:
                self.log.info("TEST-INFO | Failed to remove directory: %s. Waiting." % directory)
                
                
                time.sleep(1)
                try_count += 1
            else:
                
                return

        
        self.cleanup_dir_list.append(directory)

    def clean_temp_dirs(self, name, stdout):
        
        
        if self.profileDir and not self.interactive and not self.singleFile:
            self.cleanupDir(self.profileDir, name, self.xunit_result)

        self.cleanupDir(self.tempDir, name, self.xunit_result)

        if self.pluginsDir:
            self.cleanupDir(self.pluginsDir, name, self.xunit_result)

    def message_from_line(self, line):
        """ Given a line of raw output, convert to a string message. """
        if isinstance(line, basestring):
            
            if line:
                if 'TEST-UNEXPECTED-' in line:
                    self.has_failure_output = True
            return line

        msg = ['%s: ' % line['process'] if 'process' in line else '']

        
        
        
        
        if '_message' in line:
            msg.append(line['_message'])
            if 'diagnostic' in line:
                msg.append('\nDiagnostic: %s' % line['diagnostic'])
        else:
            msg.append('%s | %s | %s' % (ACTION_STRINGS[line['action']],
                                         line.get('source_file', 'undefined'),
                                         line.get('diagnostic', 'undefined')))

        msg.append('\n%s' % line['stack'] if 'stack' in line else '')
        return ''.join(msg)

    def parse_output(self, output):
        """Parses process output for structured messages and saves output as it is
        read. Sets self.has_failure_output in case of evidence of a failure"""
        for line_string in output.splitlines():
            self.process_line(line_string)

        if self.saw_proc_start and not self.saw_proc_end:
            self.has_failure_output = True

    def report_message(self, line):
        """ Reports a message to a consumer, both as a strucutured and
        human-readable log message. """

        message = cleanup_encoding(self.message_from_line(line))
        if message.endswith('\n'):
            
            
            message = message[:-1]

        if self.on_message:
            self.on_message(line, message)
        else:
            self.output_lines.append(message)

    def process_line(self, line_string):
        """ Parses a single line of output, determining its significance and
        reporting a message.
        """
        try:
            line_object = json.loads(line_string)
            if not isinstance(line_object, dict):
                self.report_message(line_string)
                return
        except ValueError:
            self.report_message(line_string)
            return

        if 'action' not in line_object:
            
            
            self.report_message(line_string)
            return

        action = line_object['action']
        self.report_message(line_object)

        if action in FAILURE_ACTIONS:
            self.has_failure_output = True
        elif action == 'child_test_start':
            self.saw_proc_start = True
        elif action == 'child_test_end':
            self.saw_proc_end = True

    def log_output(self, output):
        """Prints given output line-by-line to avoid overflowing buffers."""
        self.log.info(">>>>>>>")
        if output:
            if isinstance(output, basestring):
                output = output.splitlines()
            for part in output:
                
                for line in part.splitlines():
                    try:
                        line = line.decode('utf-8')
                    except UnicodeDecodeError:
                        self.log.info("TEST-INFO | %s | Detected non UTF-8 output."\
                                      " Please modify the test to only print UTF-8." %
                                      self.test_object['name'])
                        
                        line = line.decode('utf-8', 'replace')
                    self.log.info(line)
        self.log.info("<<<<<<<")

    def run_test(self):
        """Run an individual xpcshell test."""
        global gotSIGINT

        name = self.test_object['path']

        self.xunit_result = {'name': self.test_object['name'], 'classname': 'xpcshell'}

        
        
        
        if self.tests_root_dir is not None:
            self.tests_root_dir = os.path.normpath(self.tests_root_dir)
            if os.path.normpath(self.test_object['here']).find(self.tests_root_dir) != 0:
                raise Exception('tests_root_dir is not a parent path of %s' %
                    self.test_object['here'])
            relpath = self.test_object['here'][len(self.tests_root_dir):].lstrip('/\\')
            self.xunit_result['classname'] = relpath.replace('/', '.').replace('\\', '.')

        
        if 'disabled' in self.test_object:
            self.log.info('TEST-INFO | skipping %s | %s' %
                (name, self.test_object['disabled']))

            self.xunit_result['skipped'] = True
            self.retry = False

            self.keep_going = True
            return

        
        expected = self.test_object['expected'] == 'pass'

        
        
        if self.app_dir_key and self.app_dir_key in self.test_object:
            rel_app_dir = self.test_object[self.app_dir_key]
            rel_app_dir = os.path.join(self.xrePath, rel_app_dir)
            self.appPath = os.path.abspath(rel_app_dir)
        else:
            self.appPath = None

        test_dir = os.path.dirname(name)
        self.buildXpcsCmd(test_dir)
        head_files, tail_files = self.getHeadAndTailFiles(self.test_object)
        cmdH = self.buildCmdHead(head_files, tail_files, self.xpcsCmd)

        
        
        self.profileDir = self.setupProfileDir()
        self.tempDir = self.setupTempDir()

        
        cmdT = self.buildCmdTestFile(name)

        args = self.xpcsRunArgs[:]
        if 'debug' in self.test_object:
            args.insert(0, '-d')

        completeCmd = cmdH + cmdT + args

        testTimeoutInterval = HARNESS_TIMEOUT
        
        if 'requesttimeoutfactor' in self.test_object:
            testTimeoutInterval *= int(self.test_object['requesttimeoutfactor'])

        testTimer = None
        if not self.interactive and not self.debuggerInfo:
            testTimer = Timer(testTimeoutInterval, lambda: self.testTimeout(name, proc))
            testTimer.start()

        proc = None
        stdout = None
        stderr = None

        try:
            self.log.info("TEST-INFO | %s | running test ..." % name)
            if self.verbose:
                self.logCommand(name, completeCmd, test_dir)

            startTime = time.time()
            proc = self.launchProcess(completeCmd,
                stdout=self.pStdout, stderr=self.pStderr, env=self.env, cwd=test_dir, timeout=testTimeoutInterval)

            if self.interactive:
                self.log.info("TEST-INFO | %s | Process ID: %d" % (name, proc.pid))

            stdout, stderr = self.communicate(proc)

            if self.interactive:
                
                self.keep_going = True
                return

            if testTimer:
                testTimer.cancel()

            if stdout:
                self.parse_output(stdout)
            result = not (self.has_failure_output or
                          (self.getReturnCode(proc) != 0))

            if result != expected:
                if self.retry:
                    self.clean_temp_dirs(name, stdout)
                    return

                failureType = "TEST-UNEXPECTED-%s" % ("FAIL" if expected else "PASS")
                message = "%s | %s | test failed (with xpcshell return code: %d)" % (
                              failureType, name, self.getReturnCode(proc))
                if self.output_lines:
                    message += ", see following log:"

                with LOG_MUTEX:
                    self.log.error(message)
                    self.log_output(self.output_lines)

                self.failCount += 1
                self.xunit_result["passed"] = False

                self.xunit_result["failure"] = {
                  "type": failureType,
                  "message": message,
                  "text": stdout
                }

                if self.failureManifest:
                    with open(self.failureManifest, 'a') as f:
                        f.write('[%s]\n' % self.test_object['path'])
                        for k, v in self.test_object.items():
                            f.write('%s = %s\n' % (k, v))

            else:
                now = time.time()
                timeTaken = (now - startTime) * 1000
                self.xunit_result["time"] = now - startTime

                with LOG_MUTEX:
                    self.log.info("TEST-%s | %s | test passed (time: %.3fms)" % ("PASS" if expected else "KNOWN-FAIL", name, timeTaken))
                    if self.verbose:
                        self.log_output(self.output_lines)

                self.xunit_result["passed"] = True
                self.retry = False

                if expected:
                    self.passCount = 1
                else:
                    self.todoCount = 1
                    self.xunit_result["todo"] = True

            if self.checkForCrashes(self.tempDir, self.symbolsPath, test_name=name):
                if self.retry:
                    self.clean_temp_dirs(name, stdout)
                    return

                message = "PROCESS-CRASH | %s | application crashed" % name
                self.failCount = 1
                self.xunit_result["passed"] = False
                self.xunit_result["failure"] = {
                    "type": "PROCESS-CRASH",
                    "message": message,
                    "text": stdout
                }

            if self.logfiles and stdout:
                self.createLogFile(name, stdout)

        finally:
            
            
            if proc and self.poll(proc) is None:
                self.kill(proc)

                if self.retry:
                    self.clean_temp_dirs(name, stdout)
                    return

                with LOG_MUTEX:
                    message = "TEST-UNEXPECTED-FAIL | %s | Process still running after test!" % name
                    self.log.error(message)
                    self.log_output(self.output_lines)

                self.failCount = 1
                self.xunit_result["passed"] = False
                self.xunit_result["failure"] = {
                  "type": "TEST-UNEXPECTED-FAIL",
                  "message": message,
                  "text": stdout
                }

            self.clean_temp_dirs(name, stdout)

        if gotSIGINT:
            self.xunit_result["passed"] = False
            self.xunit_result["time"] = "0.0"
            self.xunit_result["failure"] = {
                "type": "SIGINT",
                "message": "Received SIGINT",
                "text": "Received SIGINT (control-C) during test execution."
            }

            self.log.error("TEST-UNEXPECTED-FAIL | Received SIGINT (control-C) during test execution")
            if self.keep_going:
                gotSIGINT = False
            else:
                self.keep_going = False
                return

        self.keep_going = True

class XPCShellTests(object):

    log = getGlobalLog()

    def __init__(self, log=None):
        """ Init logging and node status """
        if log:
            resetGlobalLog(log)

        
        
        log_funs = ['debug', 'info', 'warning', 'error', 'critical', 'log']
        for fun_name in log_funs:
            unwrapped = getattr(self.log, fun_name, None)
            if unwrapped:
                def wrap(fn):
                    def wrapped(*args, **kwargs):
                        with LOG_MUTEX:
                            fn(*args, **kwargs)
                    return wrapped
                setattr(self.log, fun_name, wrap(unwrapped))

        self.nodeProc = {}

    def buildTestList(self):
        """
          read the xpcshell.ini manifest and set self.alltests to be
          an array of test objects.

          if we are chunking tests, it will be done here as well
        """
        if isinstance(self.manifest, manifestparser.TestManifest):
            mp = self.manifest
        else:
            mp = manifestparser.TestManifest(strict=False)
            if self.manifest is None:
                for testdir in self.testdirs:
                    if testdir:
                        mp.read(os.path.join(testdir, 'xpcshell.ini'))
            else:
                mp.read(self.manifest)

        self.buildTestPath()

        try:
            self.alltests = mp.active_tests(**mozinfo.info)
        except TypeError:
            sys.stderr.write("*** offending mozinfo.info: %s\n" % repr(mozinfo.info))
            raise

        if self.singleFile is None and self.totalChunks > 1:
            self.chunkTests()

    def chunkTests(self):
        """
          Split the list of tests up into [totalChunks] pieces and filter the
          self.alltests based on thisChunk, so we only run a subset.
        """
        totalTests = len(self.alltests)
        testsPerChunk = math.ceil(totalTests / float(self.totalChunks))
        start = int(round((self.thisChunk-1) * testsPerChunk))
        end = int(start + testsPerChunk)
        if end > totalTests:
            end = totalTests
        self.log.info("Running tests %d-%d/%d", start+1, end, totalTests)
        self.alltests = self.alltests[start:end]

    def setAbsPath(self):
        """
          Set the absolute path for xpcshell, httpdjspath and xrepath.
          These 3 variables depend on input from the command line and we need to allow for absolute paths.
          This function is overloaded for a remote solution as os.path* won't work remotely.
        """
        self.testharnessdir = os.path.dirname(os.path.abspath(__file__))
        self.headJSPath = self.testharnessdir.replace("\\", "/") + "/head.js"
        self.xpcshell = os.path.abspath(self.xpcshell)

        
        self.httpdJSPath = os.path.join(os.path.dirname(self.xpcshell), 'components', 'httpd.js')
        self.httpdJSPath = replaceBackSlashes(self.httpdJSPath)

        self.httpdManifest = os.path.join(os.path.dirname(self.xpcshell), 'components', 'httpd.manifest')
        self.httpdManifest = replaceBackSlashes(self.httpdManifest)

        if self.xrePath is None:
            self.xrePath = os.path.dirname(self.xpcshell)
        else:
            self.xrePath = os.path.abspath(self.xrePath)

        if self.mozInfo is None:
            self.mozInfo = os.path.join(self.testharnessdir, "mozinfo.json")

    def buildCoreEnvironment(self):
        """
          Add environment variables likely to be used across all platforms, including remote systems.
        """
        
        self.env["XPCOM_DEBUG_BREAK"] = "stack-and-abort"
        
        if not self.debuggerInfo:
            self.env["MOZ_CRASHREPORTER"] = "1"
        
        self.env["MOZ_CRASHREPORTER_NO_REPORT"] = "1"
        
        
        self.env["NS_TRACE_MALLOC_DISABLE_STACKS"] = "1"
        
        self.env["MOZ_DISABLE_NONLOCAL_CONNECTIONS"] = "1"

    def buildEnvironment(self):
        """
          Create and returns a dictionary of self.env to include all the appropriate env variables and values.
          On a remote system, we overload this to set different values and are missing things like os.environ and PATH.
        """
        self.env = dict(os.environ)
        self.buildCoreEnvironment()
        if sys.platform == 'win32':
            self.env["PATH"] = self.env["PATH"] + ";" + self.xrePath
        elif sys.platform in ('os2emx', 'os2knix'):
            os.environ["BEGINLIBPATH"] = self.xrePath + ";" + self.env["BEGINLIBPATH"]
            os.environ["LIBPATHSTRICT"] = "T"
        elif sys.platform == 'osx' or sys.platform == "darwin":
            self.env["DYLD_LIBRARY_PATH"] = self.xrePath
        else: 
            if not "LD_LIBRARY_PATH" in self.env or self.env["LD_LIBRARY_PATH"] is None:
                self.env["LD_LIBRARY_PATH"] = self.xrePath
            else:
                self.env["LD_LIBRARY_PATH"] = ":".join([self.xrePath, self.env["LD_LIBRARY_PATH"]])

        if "asan" in self.mozInfo and self.mozInfo["asan"]:
            
            llvmsym = os.path.join(self.xrePath, "llvm-symbolizer")
            if os.path.isfile(llvmsym):
                self.env["ASAN_SYMBOLIZER_PATH"] = llvmsym
                self.log.info("INFO | runxpcshelltests.py | ASan using symbolizer at %s", llvmsym)
            else:
                self.log.info("TEST-UNEXPECTED-FAIL | runxpcshelltests.py | Failed to find ASan symbolizer at %s", llvmsym)

        return self.env

    def getPipes(self):
        """
          Determine the value of the stdout and stderr for the test.
          Return value is a list (pStdout, pStderr).
        """
        if self.interactive:
            pStdout = None
            pStderr = None
        else:
            if (self.debuggerInfo and self.debuggerInfo.interactive):
                pStdout = None
                pStderr = None
            else:
                if sys.platform == 'os2emx':
                    pStdout = None
                else:
                    pStdout = PIPE
                pStderr = STDOUT
        return pStdout, pStderr

    def buildTestPath(self):
        """
          If we specifiy a testpath, set the self.testPath variable to be the given directory or file.

          |testPath| will be the optional path only, or |None|.
          |singleFile| will be the optional test only, or |None|.
        """
        self.singleFile = None
        if self.testPath is not None:
            if self.testPath.endswith('.js'):
            
                if self.testPath.find('/') == -1:
                    
                    self.singleFile = self.testPath
                else:
                    
                    
                    self.testPath = self.testPath.rsplit('/', 1)
                    self.singleFile = self.testPath[1]
                    self.testPath = self.testPath[0]
            else:
                
                
                self.testPath = self.testPath.rstrip("/")

    def verifyDirPath(self, dirname):
        """
          Simple wrapper to get the absolute path for a given directory name.
          On a remote system, we need to overload this to work on the remote filesystem.
        """
        return os.path.abspath(dirname)

    def trySetupNode(self):
        """
          Run node for SPDY tests, if available, and updates mozinfo as appropriate.
        """
        nodeMozInfo = {'hasNode': False} 
        nodeBin = None

        
        
        localPath = os.getenv('MOZ_NODE_PATH', None)
        if localPath and os.path.exists(localPath) and os.path.isfile(localPath):
            nodeBin = localPath

        if nodeBin:
            self.log.info('Found node at %s' % (nodeBin,))

            def startServer(name, serverJs):
                if os.path.exists(serverJs):
                    
                    self.log.info('Found %s at %s' % (name, serverJs))
                    try:
                        
                        
                        process = Popen([nodeBin, serverJs], stdin=PIPE, stdout=PIPE,
                                stderr=STDOUT, env=self.env, cwd=os.getcwd())
                        self.nodeProc[name] = process

                        
                        
                        msg = process.stdout.readline()
                        if 'server listening' in msg:
                            nodeMozInfo['hasNode'] = True
                    except OSError, e:
                        
                        self.log.error('Could not run %s server: %s' % (name, str(e)))

            myDir = os.path.split(os.path.abspath(__file__))[0]
            startServer('moz-spdy', os.path.join(myDir, 'moz-spdy', 'moz-spdy.js'))
            startServer('moz-http2', os.path.join(myDir, 'moz-http2', 'moz-http2.js'))

        mozinfo.update(nodeMozInfo)

    def shutdownNode(self):
        """
          Shut down our node process, if it exists
        """
        for name, proc in self.nodeProc.iteritems():
            self.log.info('Node %s server shutting down ...' % name)
            proc.terminate()

    def writeXunitResults(self, results, name=None, filename=None, fh=None):
        """
          Write Xunit XML from results.

          The function receives an iterable of results dicts. Each dict must have
          the following keys:

            classname - The "class" name of the test.
            name - The simple name of the test.

          In addition, it must have one of the following saying how the test
          executed:

            passed - Boolean indicating whether the test passed. False if it
              failed.
            skipped - True if the test was skipped.

          The following keys are optional:

            time - Execution time of the test in decimal seconds.
            failure - Dict describing test failure. Requires keys:
              type - String type of failure.
              message - String describing basic failure.
              text - Verbose string describing failure.

          Arguments:

          |name|, Name of the test suite. Many tools expect Java class dot notation
            e.g. dom.simple.foo. A directory with '/' converted to '.' is a good
            choice.
          |fh|, File handle to write XML to.
          |filename|, File name to write XML to.
          |results|, Iterable of tuples describing the results.
        """
        if filename is None and fh is None:
            raise Exception("One of filename or fh must be defined.")

        if name is None:
            name = "xpcshell"
        else:
            assert isinstance(name, basestring)

        if filename is not None:
            fh = open(filename, 'wb')

        doc = xml.dom.minidom.Document()
        testsuite = doc.createElement("testsuite")
        testsuite.setAttribute("name", name)
        doc.appendChild(testsuite)

        total = 0
        passed = 0
        failed = 0
        skipped = 0

        for result in results:
            total += 1

            if result.get("skipped", None):
                skipped += 1
            elif result["passed"]:
                passed += 1
            else:
                failed += 1

            testcase = doc.createElement("testcase")
            testcase.setAttribute("classname", result["classname"])
            testcase.setAttribute("name", result["name"])

            if "time" in result:
                testcase.setAttribute("time", str(result["time"]))
            else:
                
                testcase.setAttribute("time", "0")

            if "failure" in result:
                failure = doc.createElement("failure")
                failure.setAttribute("type", str(result["failure"]["type"]))
                failure.setAttribute("message", result["failure"]["message"])

                
                
                
                cdata = result["failure"]["text"]
                if not isinstance(cdata, str):
                    cdata = ""

                cdata = cdata.replace("]]>", "]] >")
                text = doc.createCDATASection(cdata)
                failure.appendChild(text)
                testcase.appendChild(failure)

            if result.get("skipped", None):
                e = doc.createElement("skipped")
                testcase.appendChild(e)

            testsuite.appendChild(testcase)

        testsuite.setAttribute("tests", str(total))
        testsuite.setAttribute("failures", str(failed))
        testsuite.setAttribute("skip", str(skipped))

        doc.writexml(fh, addindent="  ", newl="\n", encoding="utf-8")

    def post_to_autolog(self, results, name):
        from moztest.results import TestContext, TestResult, TestResultCollection
        from moztest.output.autolog import AutologOutput

        context = TestContext(
            testgroup='b2g xpcshell testsuite',
            operating_system='android',
            arch='emulator',
            harness='xpcshell',
            hostname=socket.gethostname(),
            tree='b2g',
            buildtype='opt',
            )

        collection = TestResultCollection('b2g emulator testsuite')

        for result in results:
            duration = result.get('time', 0)

            if 'skipped' in result:
                outcome = 'SKIPPED'
            elif 'todo' in result:
                outcome = 'KNOWN-FAIL'
            elif result['passed']:
                outcome = 'PASS'
            else:
                outcome = 'UNEXPECTED-FAIL'

            output = None
            if 'failure' in result:
                output = result['failure']['text']

            t = TestResult(name=result['name'], test_class=name,
                           time_start=0, context=context)
            t.finish(result=outcome, time_end=duration, output=output)

            collection.append(t)
            collection.time_taken += duration

        out = AutologOutput()
        out.post(out.make_testgroups(collection))

    def buildXpcsRunArgs(self):
        """
          Add arguments to run the test or make it interactive.
        """
        if self.interactive:
            self.xpcsRunArgs = [
            '-e', 'print("To start the test, type |_execute_test();|.");',
            '-i']
        else:
            self.xpcsRunArgs = ['-e', '_execute_test(); quit(0);']

    def addTestResults(self, test):
        self.passCount += test.passCount
        self.failCount += test.failCount
        self.todoCount += test.todoCount
        self.xunitResults.append(test.xunit_result)

    def runTests(self, xpcshell, xrePath=None, appPath=None, symbolsPath=None,
                 manifest=None, testdirs=None, testPath=None, mobileArgs=None,
                 interactive=False, verbose=False, keepGoing=False, logfiles=True,
                 thisChunk=1, totalChunks=1, debugger=None,
                 debuggerArgs=None, debuggerInteractive=False,
                 profileName=None, mozInfo=None, sequential=False, shuffle=False,
                 testsRootDir=None, xunitFilename=None, xunitName=None,
                 testingModulesDir=None, autolog=False, pluginsPath=None,
                 testClass=XPCShellTestThread, failureManifest=None,
                 on_message=None, **otherOptions):
        """Run xpcshell tests.

        |xpcshell|, is the xpcshell executable to use to run the tests.
        |xrePath|, if provided, is the path to the XRE to use.
        |appPath|, if provided, is the path to an application directory.
        |symbolsPath|, if provided is the path to a directory containing
          breakpad symbols for processing crashes in tests.
        |manifest|, if provided, is a file containing a list of
          test directories to run.
        |testdirs|, if provided, is a list of absolute paths of test directories.
          No-manifest only option.
        |testPath|, if provided, indicates a single path and/or test to run.
        |pluginsPath|, if provided, custom plugins directory to be returned from
          the xpcshell dir svc provider for NS_APP_PLUGINS_DIR_LIST.
        |interactive|, if set to True, indicates to provide an xpcshell prompt
          instead of automatically executing the test.
        |verbose|, if set to True, will cause stdout/stderr from tests to
          be printed always
        |logfiles|, if set to False, indicates not to save output to log files.
          Non-interactive only option.
        |debugger|, if set, specifies the name of the debugger that will be used
          to launch xpcshell.
        |debuggerArgs|, if set, specifies arguments to use with the debugger.
        |debuggerInteractive|, if set, allows the debugger to be run in interactive
          mode.
        |profileName|, if set, specifies the name of the application for the profile
          directory if running only a subset of tests.
        |mozInfo|, if set, specifies specifies build configuration information, either as a filename containing JSON, or a dict.
        |shuffle|, if True, execute tests in random order.
        |testsRootDir|, absolute path to root directory of all tests. This is used
          by xUnit generation to determine the package name of the tests.
        |xunitFilename|, if set, specifies the filename to which to write xUnit XML
          results.
        |xunitName|, if outputting an xUnit XML file, the str value to use for the
          testsuite name.
        |testingModulesDir|, if provided, specifies where JS modules reside.
          xpcshell will register a resource handler mapping this path.
        |otherOptions| may be present for the convenience of subclasses
        """

        global gotSIGINT

        if testdirs is None:
            testdirs = []

        if xunitFilename is not None or xunitName is not None:
            if not isinstance(testsRootDir, basestring):
                raise Exception("testsRootDir must be a str when outputting xUnit.")

            if not os.path.isabs(testsRootDir):
                testsRootDir = os.path.abspath(testsRootDir)

            if not os.path.exists(testsRootDir):
                raise Exception("testsRootDir path does not exists: %s" %
                        testsRootDir)

        
        
        
        
        
        if not testingModulesDir:
            ourDir = os.path.dirname(__file__)
            possible = os.path.join(ourDir, os.path.pardir, 'modules')

            if os.path.isdir(possible):
                testingModulesDir = possible

        if testingModulesDir:
            
            
            
            testingModulesDir = os.path.normpath(testingModulesDir)

            if not os.path.isabs(testingModulesDir):
                testingModulesDir = os.path.abspath(testingModulesDir)

            if not testingModulesDir.endswith(os.path.sep):
                testingModulesDir += os.path.sep

        self.debuggerInfo = None

        if debugger:
            
            
            if debuggerArgs:
                debuggerArgs = debuggerArgs.split();

            self.debuggerInfo = mozdebug.get_debugger_info(debugger, debuggerArgs, debuggerInteractive)

        self.xpcshell = xpcshell
        self.xrePath = xrePath
        self.appPath = appPath
        self.symbolsPath = symbolsPath
        self.manifest = manifest
        self.testdirs = testdirs
        self.testPath = testPath
        self.interactive = interactive
        self.verbose = verbose
        self.keepGoing = keepGoing
        self.logfiles = logfiles
        self.on_message = on_message
        self.totalChunks = totalChunks
        self.thisChunk = thisChunk
        self.profileName = profileName or "xpcshell"
        self.mozInfo = mozInfo
        self.testingModulesDir = testingModulesDir
        self.pluginsPath = pluginsPath
        self.sequential = sequential

        if not testdirs and not manifest:
            
            self.log.error("Error: No test dirs or test manifest specified!")
            return False

        self.testCount = 0
        self.passCount = 0
        self.failCount = 0
        self.todoCount = 0

        self.setAbsPath()
        self.buildXpcsRunArgs()

        self.event = Event()

        
        if not isinstance(self.mozInfo, dict):
            mozInfoFile = self.mozInfo
            if not os.path.isfile(mozInfoFile):
                self.log.error("Error: couldn't find mozinfo.json at '%s'. Perhaps you need to use --build-info-json?" % mozInfoFile)
                return False
            self.mozInfo = json.load(open(mozInfoFile))

        
        
        
        fixedInfo = {}
        for k, v in self.mozInfo.items():
            if isinstance(k, unicode):
                k = k.encode('ascii')
            fixedInfo[k] = v
        self.mozInfo = fixedInfo

        mozinfo.update(self.mozInfo)

        
        self.buildEnvironment()

        
        
        
        
        appDirKey = None
        if "appname" in self.mozInfo:
            appDirKey = self.mozInfo["appname"] + "-appdir"

        
        
        self.trySetupNode()

        pStdout, pStderr = self.getPipes()

        self.buildTestList()
        if self.singleFile:
            self.sequential = True

        if shuffle:
            random.shuffle(self.alltests)

        self.xunitResults = []
        self.cleanup_dir_list = []
        self.try_again_list = []

        kwargs = {
            'appPath': self.appPath,
            'xrePath': self.xrePath,
            'testingModulesDir': self.testingModulesDir,
            'debuggerInfo': self.debuggerInfo,
            'pluginsPath': self.pluginsPath,
            'httpdManifest': self.httpdManifest,
            'httpdJSPath': self.httpdJSPath,
            'headJSPath': self.headJSPath,
            'testharnessdir': self.testharnessdir,
            'profileName': self.profileName,
            'singleFile': self.singleFile,
            'env': self.env, 
            'symbolsPath': self.symbolsPath,
            'logfiles': self.logfiles,
            'xpcshell': self.xpcshell,
            'xpcsRunArgs': self.xpcsRunArgs,
            'failureManifest': failureManifest,
            'on_message': self.on_message,
        }

        if self.sequential:
            
            
            signal.signal(signal.SIGINT, markGotSIGINT)

        if self.debuggerInfo:
            
            self.sequential = True

            
            if self.debuggerInfo.interactive:
                signal.signal(signal.SIGINT, lambda signum, frame: None)

        
        tests_queue = deque()
        
        sequential_tests = []
        for test_object in self.alltests:
            name = test_object['path']
            if self.singleFile and not name.endswith(self.singleFile):
                continue

            if self.testPath and name.find(self.testPath) == -1:
                continue

            self.testCount += 1

            test = testClass(test_object, self.event, self.cleanup_dir_list,
                    tests_root_dir=testsRootDir, app_dir_key=appDirKey,
                    interactive=interactive, verbose=verbose, pStdout=pStdout,
                    pStderr=pStderr, keep_going=keepGoing, log=self.log,
                    mobileArgs=mobileArgs, **kwargs)
            if 'run-sequentially' in test_object or self.sequential:
                sequential_tests.append(test)
            else:
                tests_queue.append(test)

        if self.sequential:
            self.log.info("INFO | Running tests sequentially.")
        else:
            self.log.info("INFO | Using at most %d threads." % NUM_THREADS)

        
        
        running_tests = set()
        keep_going = True
        exceptions = []
        tracebacks = []
        while tests_queue or running_tests:
            
            
            if not keep_going and not running_tests:
                break

            
            while keep_going and tests_queue and (len(running_tests) < NUM_THREADS):
                test = tests_queue.popleft()
                running_tests.add(test)
                test.start()

            
            

            
            self.event.wait(1)
            self.event.clear()

            
            done_tests = set()
            for test in running_tests:
                if test.done:
                    done_tests.add(test)
                    test.join(1) 
                    
                    
                    if test.retry or test.is_alive():
                        
                        self.try_again_list.append(test.test_object)
                        continue
                    
                    if test.exception:
                        exceptions.append(test.exception)
                        tracebacks.append(test.traceback)
                        
                        
                        keep_going = False
                    keep_going = keep_going and test.keep_going
                    self.addTestResults(test)

            
            running_tests.difference_update(done_tests)

        if keep_going:
            
            for test in sequential_tests:
                if not keep_going:
                    self.log.error("TEST-UNEXPECTED-FAIL | Received SIGINT (control-C), so stopped run. " \
                                   "(Use --keep-going to keep running tests after killing one with SIGINT)")
                    break
                
                test.retry = False
                test.start()
                test.join()
                self.addTestResults(test)
                
                if test.exception:
                    exceptions.append(test.exception)
                    tracebacks.append(test.traceback)
                    break
                keep_going = test.keep_going

        
        if self.try_again_list:
            self.log.info("Retrying tests that failed when run in parallel.")
        for test_object in self.try_again_list:
            test = testClass(test_object, self.event, self.cleanup_dir_list,
                    retry=False, tests_root_dir=testsRootDir,
                    app_dir_key=appDirKey, interactive=interactive,
                    verbose=verbose, pStdout=pStdout, pStderr=pStderr,
                    keep_going=keepGoing, log=self.log, mobileArgs=mobileArgs,
                    **kwargs)
            test.start()
            test.join()
            self.addTestResults(test)
            
            if test.exception:
                exceptions.append(test.exception)
                tracebacks.append(test.traceback)
                break
            keep_going = test.keep_going

        
        signal.signal(signal.SIGINT, signal.SIG_DFL)

        self.shutdownNode()
        
        
        
        
        for directory in self.cleanup_dir_list:
            try:
                shutil.rmtree(directory)
            except:
                self.log.info("INFO | %s could not be cleaned up." % directory)

        if exceptions:
            self.log.info("INFO | Following exceptions were raised:")
            for t in tracebacks:
                self.log.error(t)
            raise exceptions[0]

        if self.testCount == 0:
            self.log.error("TEST-UNEXPECTED-FAIL | runxpcshelltests.py | No tests run. Did you pass an invalid --test-path?")
            self.failCount = 1

        self.log.info("INFO | Result summary:")
        self.log.info("INFO | Passed: %d" % self.passCount)
        self.log.info("INFO | Failed: %d" % self.failCount)
        self.log.info("INFO | Todo: %d" % self.todoCount)
        self.log.info("INFO | Retried: %d" % len(self.try_again_list))

        if autolog:
            self.post_to_autolog(self.xunitResults, xunitName)

        if xunitFilename is not None:
            self.writeXunitResults(filename=xunitFilename, results=self.xunitResults,
                                   name=xunitName)

        if gotSIGINT and not keepGoing:
            self.log.error("TEST-UNEXPECTED-FAIL | Received SIGINT (control-C), so stopped run. " \
                           "(Use --keep-going to keep running tests after killing one with SIGINT)")
            return False

        return self.failCount == 0

class XPCShellOptions(OptionParser):
    def __init__(self):
        """Process command line arguments and call runTests() to do the real work."""
        OptionParser.__init__(self)

        addCommonOptions(self)
        self.add_option("--app-path",
                        type="string", dest="appPath", default=None,
                        help="application directory (as opposed to XRE directory)")
        self.add_option("--autolog",
                        action="store_true", dest="autolog", default=False,
                        help="post to autolog")
        self.add_option("--interactive",
                        action="store_true", dest="interactive", default=False,
                        help="don't automatically run tests, drop to an xpcshell prompt")
        self.add_option("--verbose",
                        action="store_true", dest="verbose", default=False,
                        help="always print stdout and stderr from tests")
        self.add_option("--keep-going",
                        action="store_true", dest="keepGoing", default=False,
                        help="continue running tests after test killed with control-C (SIGINT)")
        self.add_option("--logfiles",
                        action="store_true", dest="logfiles", default=True,
                        help="create log files (default, only used to override --no-logfiles)")
        self.add_option("--manifest",
                        type="string", dest="manifest", default=None,
                        help="Manifest of test directories to use")
        self.add_option("--no-logfiles",
                        action="store_false", dest="logfiles",
                        help="don't create log files")
        self.add_option("--sequential",
                        action="store_true", dest="sequential", default=False,
                        help="Run all tests sequentially")
        self.add_option("--test-path",
                        type="string", dest="testPath", default=None,
                        help="single path and/or test filename to test")
        self.add_option("--tests-root-dir",
                        type="string", dest="testsRootDir", default=None,
                        help="absolute path to directory where all tests are located. this is typically $(objdir)/_tests")
        self.add_option("--testing-modules-dir",
                        dest="testingModulesDir", default=None,
                        help="Directory where testing modules are located.")
        self.add_option("--test-plugin-path",
                        type="string", dest="pluginsPath", default=None,
                        help="Path to the location of a plugins directory containing the test plugin or plugins required for tests. "
                             "By default xpcshell's dir svc provider returns gre/plugins. Use test-plugin-path to add a directory "
                             "to return for NS_APP_PLUGINS_DIR_LIST when queried.")
        self.add_option("--total-chunks",
                        type = "int", dest = "totalChunks", default=1,
                        help = "how many chunks to split the tests up into")
        self.add_option("--this-chunk",
                        type = "int", dest = "thisChunk", default=1,
                        help = "which chunk to run between 1 and --total-chunks")
        self.add_option("--profile-name",
                        type = "string", dest="profileName", default=None,
                        help="name of application profile being tested")
        self.add_option("--build-info-json",
                        type = "string", dest="mozInfo", default=None,
                        help="path to a mozinfo.json including information about the build configuration. defaults to looking for mozinfo.json next to the script.")
        self.add_option("--shuffle",
                        action="store_true", dest="shuffle", default=False,
                        help="Execute tests in random order")
        self.add_option("--xunit-file", dest="xunitFilename",
                        help="path to file where xUnit results will be written.")
        self.add_option("--xunit-suite-name", dest="xunitName",
                        help="name to record for this xUnit test suite. Many "
                             "tools expect Java class notation, e.g. "
                             "dom.basic.foo")
        self.add_option("--failure-manifest", dest="failureManifest",
                        action="store",
                        help="path to file where failure manifest will be written.")

def main():
    parser = XPCShellOptions()
    options, args = parser.parse_args()

    if len(args) < 2 and options.manifest is None or \
       (len(args) < 1 and options.manifest is not None):
        print >>sys.stderr, """Usage: %s <path to xpcshell> <test dirs>
              or: %s --manifest=test.manifest <path to xpcshell>""" % (sys.argv[0],
                                                              sys.argv[0])
        sys.exit(1)

    xpcsh = XPCShellTests()

    if options.interactive and not options.testPath:
        print >>sys.stderr, "Error: You must specify a test filename in interactive mode!"
        sys.exit(1)

    if not xpcsh.runTests(args[0], testdirs=args[1:], **options.__dict__):
        sys.exit(1)

if __name__ == '__main__':
    main()

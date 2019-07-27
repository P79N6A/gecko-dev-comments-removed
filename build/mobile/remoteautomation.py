



import glob
import time
import re
import os
import tempfile
import shutil
import subprocess
import sys

from automation import Automation
from devicemanager import DMError
from mozlog.structured import get_default_logger
import mozcrash


fennecLogcatFilters = [ "The character encoding of the HTML document was not declared",
                        "Use of Mutation Events is deprecated. Use MutationObserver instead.",
                        "Unexpected value from nativeGetEnabledTags: 0" ]

class RemoteAutomation(Automation):
    _devicemanager = None

    def __init__(self, deviceManager, appName = '', remoteLog = None,
                 processArgs=None):
        self._devicemanager = deviceManager
        self._appName = appName
        self._remoteProfile = None
        self._remoteLog = remoteLog
        self._processArgs = processArgs or {};

        
        self._product = "fennec"
        self.lastTestSeen = "remoteautomation.py"
        Automation.__init__(self)

    def setDeviceManager(self, deviceManager):
        self._devicemanager = deviceManager

    def setAppName(self, appName):
        self._appName = appName

    def setRemoteProfile(self, remoteProfile):
        self._remoteProfile = remoteProfile

    def setProduct(self, product):
        self._product = product

    def setRemoteLog(self, logfile):
        self._remoteLog = logfile

    
    def environment(self, env=None, xrePath=None, crashreporter=True, debugger=False, dmdPath=None, lsanPath=None):
        
        
        if env is None:
            env = {}

        if dmdPath:
            env['MOZ_REPLACE_MALLOC_LIB'] = os.path.join(dmdPath, 'libdmd.so')

        
        
        
        if 'MOZ_HIDE_RESULTS_TABLE' in os.environ:
            env['MOZ_HIDE_RESULTS_TABLE'] = os.environ['MOZ_HIDE_RESULTS_TABLE']

        if crashreporter and not debugger:
            env['MOZ_CRASHREPORTER_NO_REPORT'] = '1'
            env['MOZ_CRASHREPORTER'] = '1'
        else:
            env['MOZ_CRASHREPORTER_DISABLE'] = '1'

        
        
        
        
        env.setdefault('MOZ_DISABLE_NONLOCAL_CONNECTIONS', '1')

        return env

    def waitForFinish(self, proc, utilityPath, timeout, maxTime, startTime, debuggerInfo, symbolsPath):
        """ Wait for tests to finish.
            If maxTime seconds elapse or no output is detected for timeout
            seconds, kill the process and fail the test.
        """
        
        status = proc.wait(timeout = maxTime, noOutputTimeout = timeout)
        self.lastTestSeen = proc.getLastTestSeen

        topActivity = self._devicemanager.getTopActivity()
        if topActivity == proc.procName:
            proc.kill(True)
        if status == 1:
            if maxTime:
                print "TEST-UNEXPECTED-FAIL | %s | application ran for longer than " \
                      "allowed maximum time of %s seconds" % (self.lastTestSeen, maxTime)
            else:
                print "TEST-UNEXPECTED-FAIL | %s | application ran for longer than " \
                      "allowed maximum time" % (self.lastTestSeen)
        if status == 2:
            print "TEST-UNEXPECTED-FAIL | %s | application timed out after %d seconds with no output" \
                % (self.lastTestSeen, int(timeout))

        return status

    def deleteANRs(self):
        
        
        traces = "/data/anr/traces.txt"
        try:
            self._devicemanager.shellCheckOutput(['echo', '', '>', traces], root=True)
            self._devicemanager.shellCheckOutput(['chmod', '666', traces], root=True)
        except DMError:
            print "Error deleting %s" % traces
            pass

    def checkForANRs(self):
        traces = "/data/anr/traces.txt"
        if self._devicemanager.fileExists(traces):
            try:
                t = self._devicemanager.pullFile(traces)
                print "Contents of %s:" % traces
                print t
                
                self.deleteANRs()
            except DMError:
                print "Error pulling %s" % traces
            except IOError:
                print "Error pulling %s" % traces
        else:
            print "%s not found" % traces

    def deleteTombstones(self):
        
        remoteDir = "/data/tombstones"
        try:
            self._devicemanager.shellCheckOutput(['rm', '-r', remoteDir], root=True)
        except DMError:
            
            pass

    def checkForTombstones(self):
        
        remoteDir = "/data/tombstones"
        blobberUploadDir = os.environ.get('MOZ_UPLOAD_DIR', None)
        if blobberUploadDir:
            if not os.path.exists(blobberUploadDir):
                os.mkdir(blobberUploadDir)
            if self._devicemanager.dirExists(remoteDir):
                
                try:
                    self._devicemanager.shellCheckOutput(['chmod', '777', remoteDir], root=True)
                    self._devicemanager.shellCheckOutput(['chmod', '666', os.path.join(remoteDir, '*')], root=True)
                    self._devicemanager.getDirectory(remoteDir, blobberUploadDir, False)
                except DMError:
                    
                    pass
                self.deleteTombstones()
                
                
                for f in glob.glob(os.path.join(blobberUploadDir, "tombstone_??")):
                    
                    
                    
                    for i in xrange(1, sys.maxint):
                        newname = "%s.%d.txt" % (f, i)
                        if not os.path.exists(newname):
                            os.rename(f, newname)
                            break
            else:
                print "%s does not exist; tombstone check skipped" % remoteDir
        else:
            print "MOZ_UPLOAD_DIR not defined; tombstone check skipped"

    def checkForCrashes(self, directory, symbolsPath):
        self.checkForANRs()
        self.checkForTombstones()

        try:
            logcat = self._devicemanager.getLogcat(filterOutRegexps=fennecLogcatFilters)
        except DMError:
            print "getLogcat threw DMError; re-trying just once..."
            time.sleep(1)
            logcat = self._devicemanager.getLogcat(filterOutRegexps=fennecLogcatFilters)

        javaException = mozcrash.check_for_java_exception(logcat)
        if javaException:
            return True

        
        
        if not self.CRASHREPORTER:
            return False

        try:
            dumpDir = tempfile.mkdtemp()
            remoteCrashDir = self._remoteProfile + '/minidumps/'
            if not self._devicemanager.dirExists(remoteCrashDir):
                
                
                
                
                print "Automation Error: No crash directory (%s) found on remote device" % remoteCrashDir
                
                return True
            self._devicemanager.getDirectory(remoteCrashDir, dumpDir)

            logger = get_default_logger()
            if logger is not None:
                crashed = mozcrash.log_crashes(logger, dumpDir, symbolsPath, test=self.lastTestSeen)
            else:
                crashed = Automation.checkForCrashes(self, dumpDir, symbolsPath)

        finally:
            try:
                shutil.rmtree(dumpDir)
            except:
                print "WARNING: unable to remove directory: %s" % dumpDir
        return crashed

    def buildCommandLine(self, app, debuggerInfo, profileDir, testURL, extraArgs):
        
        if (self._remoteProfile):
            profileDir = self._remoteProfile

        
        
        if app == "am" and extraArgs[0] == "instrument":
            return app, extraArgs

        cmd, args = Automation.buildCommandLine(self, app, debuggerInfo, profileDir, testURL, extraArgs)
        
        try:
            args.remove('-foreground')
        except:
            pass


        return app, args

    def Process(self, cmd, stdout = None, stderr = None, env = None, cwd = None):
        if stdout == None or stdout == -1 or stdout == subprocess.PIPE:
            stdout = self._remoteLog

        return self.RProcess(self._devicemanager, cmd, stdout, stderr, env, cwd, self._appName,
                             **self._processArgs)

    
    class RProcess(object):
        
        dm = None
        def __init__(self, dm, cmd, stdout=None, stderr=None, env=None, cwd=None, app=None,
                     messageLogger=None):
            self.dm = dm
            self.stdoutlen = 0
            self.lastTestSeen = "remoteautomation.py"
            self.proc = dm.launchProcess(cmd, stdout, cwd, env, True)
            self.messageLogger = messageLogger

            if (self.proc is None):
                if cmd[0] == 'am':
                    self.proc = stdout
                else:
                    raise Exception("unable to launch process")
            self.procName = cmd[0].split('/')[-1]
            if cmd[0] == 'am' and cmd[1] == "instrument":
                self.procName = app
                print "Robocop process name: "+self.procName

            
            self.timeout = 3600
            
            time.sleep(1)

            
            self.logBuffer = ""

        @property
        def pid(self):
            pid = self.dm.processExist(self.procName)
            
            
            
            
            if pid is None:
                return 0
            return pid

        def read_stdout(self):
            """ Fetch the full remote log file using devicemanager and return just
                the new log entries since the last call (as a list of messages or lines).
            """
            if not self.dm.fileExists(self.proc):
                return []
            try:
                newLogContent = self.dm.pullFile(self.proc, self.stdoutlen)
            except DMError:
                
                
                
                return []
            if not newLogContent:
                return []

            self.stdoutlen += len(newLogContent)

            if self.messageLogger is None:
                testStartFilenames = re.findall(r"TEST-START \| ([^\s]*)", newLogContent)
                if testStartFilenames:
                    self.lastTestSeen = testStartFilenames[-1]
                print newLogContent
                return [newLogContent]

            self.logBuffer += newLogContent
            lines = self.logBuffer.split('\n')
            if not lines:
                return

            
            self.logBuffer = lines[-1]
            del lines[-1]
            messages = []
            for line in lines:
                
                
                parsed_messages = self.messageLogger.write(line)
                for message in parsed_messages:
                    if message['action'] == 'test_start':
                        self.lastTestSeen = message['test']
                messages += parsed_messages
            return messages

        @property
        def getLastTestSeen(self):
            return self.lastTestSeen

        
        
        
        
        
        
        def wait(self, timeout = None, noOutputTimeout = None):
            timer = 0
            noOutputTimer = 0
            interval = 20

            if timeout == None:
                timeout = self.timeout

            status = 0
            while (self.dm.getTopActivity() == self.procName):
                
                if timer % 60 == 0:
                    messages = self.read_stdout()
                    if messages:
                        noOutputTimer = 0

                time.sleep(interval)
                timer += interval
                noOutputTimer += interval
                if (timer > timeout):
                    status = 1
                    break
                if (noOutputTimeout and noOutputTimer > noOutputTimeout):
                    status = 2
                    break

            
            self.read_stdout()

            return status

        def kill(self, stagedShutdown = False):
            if stagedShutdown:
                
                self.dm.killProcess(self.procName, 3)
                time.sleep(3)
                
                self.dm.killProcess(self.procName, 6)
                
                retries = 0
                while retries < 3:
                    pid = self.dm.processExist(self.procName)
                    if pid and pid > 0:
                        print "%s still alive after SIGABRT: waiting..." % self.procName
                        time.sleep(5)
                    else:
                        return
                    retries += 1
                self.dm.killProcess(self.procName, 9)
                pid = self.dm.processExist(self.procName)
                if pid and pid > 0:
                    self.dm.killProcess(self.procName)
            else:
                self.dm.killProcess(self.procName)

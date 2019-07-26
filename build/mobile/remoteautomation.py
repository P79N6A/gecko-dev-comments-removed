



import time
import re
import os
import automationutils
import tempfile
import shutil
import subprocess

from automation import Automation
from devicemanager import NetworkTools, DMError


fennecLogcatFilters = [ "The character encoding of the HTML document was not declared",
                           "Use of Mutation Events is deprecated. Use MutationObserver instead." ]

class RemoteAutomation(Automation):
    _devicemanager = None

    def __init__(self, deviceManager, appName = '', remoteLog = None):
        self._devicemanager = deviceManager
        self._appName = appName
        self._remoteProfile = None
        self._remoteLog = remoteLog

        
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

    
    def environment(self, env = None, xrePath = None, crashreporter = True):
        
        
        if env is None:
            env = {}

        
        
        
        if 'MOZ_HIDE_RESULTS_TABLE' in os.environ:
            env['MOZ_HIDE_RESULTS_TABLE'] = os.environ['MOZ_HIDE_RESULTS_TABLE']

        if crashreporter:
            env['MOZ_CRASHREPORTER_NO_REPORT'] = '1'
            env['MOZ_CRASHREPORTER'] = '1'
        else:
            env['MOZ_CRASHREPORTER_DISABLE'] = '1'

        return env

    def waitForFinish(self, proc, utilityPath, timeout, maxTime, startTime, debuggerInfo, symbolsPath):
        """ Wait for tests to finish (as evidenced by the process exiting),
            or for maxTime elapse, in which case kill the process regardless.
        """
        
        status = proc.wait(timeout = maxTime)
        self.lastTestSeen = proc.getLastTestSeen

        if (status == 1 and self._devicemanager.processExist(proc.procName)):
            
            if maxTime:
                print "TEST-UNEXPECTED-FAIL | %s | application ran for longer than " \
                      "allowed maximum time of %s seconds" % (self.lastTestSeen, maxTime)
            else:
                print "TEST-UNEXPECTED-FAIL | %s | application ran for longer than " \
                      "allowed maximum time" % (self.lastTestSeen)
            proc.kill()

        return status

    def checkForJavaException(self, logcat):
        found_exception = False
        for i, line in enumerate(logcat):
            if "REPORTING UNCAUGHT EXCEPTION" in line:
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                found_exception = True
                logre = re.compile(r".*\):\s(.*)")
                m = logre.search(logcat[i+1])
                if m and m.group(1):
                    top_frame = m.group(1)
                m = logre.search(logcat[i+2])
                if m and m.group(1):
                    top_frame = top_frame + m.group(1)
                print "PROCESS-CRASH | java-exception | %s" % top_frame
                break
        return found_exception

    def checkForCrashes(self, directory, symbolsPath):
        logcat = self._devicemanager.getLogcat(filterOutRegexps=fennecLogcatFilters)
        javaException = self.checkForJavaException(logcat)
        if javaException:
            return True
        try:
            remoteCrashDir = self._remoteProfile + '/minidumps/'
            if not self._devicemanager.dirExists(remoteCrashDir):
                
                
                
                print "Automation Error: No crash directory (%s) found on remote device" % remoteCrashDir
                
                return True
            dumpDir = tempfile.mkdtemp()
            self._devicemanager.getDirectory(remoteCrashDir, dumpDir)
            crashed = automationutils.checkForCrashes(dumpDir, symbolsPath,
                                            self.lastTestSeen)
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

    def getLanIp(self):
        nettools = NetworkTools()
        return nettools.getLanIp()

    def Process(self, cmd, stdout = None, stderr = None, env = None, cwd = None):
        if stdout == None or stdout == -1 or stdout == subprocess.PIPE:
          stdout = self._remoteLog

        return self.RProcess(self._devicemanager, cmd, stdout, stderr, env, cwd)

    
    class RProcess(object):
        
        dm = None
        def __init__(self, dm, cmd, stdout = None, stderr = None, env = None, cwd = None):
            self.dm = dm
            self.stdoutlen = 0
            self.lastTestSeen = "remoteautomation.py"
            self.proc = dm.launchProcess(cmd, stdout, cwd, env, True)
            if (self.proc is None):
              if cmd[0] == 'am':
                self.proc = stdout
              else:
                raise Exception("unable to launch process")
            exepath = cmd[0]
            name = exepath.split('/')[-1]
            self.procName = name
            
            
            
            
            if cmd[0] == 'am' and cmd[1] == "instrument":
              try:
                i = cmd.index("class")
              except ValueError:
                
                i = -1
              if (i > 0):
                classname = cmd[i+1]
                parts = classname.split('.')
                try:
                  i = parts.index("tests")
                except ValueError:
                  
                  i = -1
                if (i > 0):
                  self.procName = '.'.join(parts[0:i])
                  print "Robocop derived process name: "+self.procName

            
            self.timeout = 3600
            
            time.sleep(1)

        @property
        def pid(self):
            pid = self.dm.processExist(self.procName)
            
            
            
            
            if pid is None:
                return 0
            return pid

        @property
        def stdout(self):
            """ Fetch the full remote log file using devicemanager and return just
                the new log entries since the last call (as a multi-line string).
            """
            if self.dm.fileExists(self.proc):
                try:
                    t = self.dm.pullFile(self.proc)
                except DMError:
                    
                    
                    
                    return ''
                newLogContent = t[self.stdoutlen:]
                self.stdoutlen = len(t)
                
                
                
                testStartFilenames = re.findall(r"TEST-START \| ([^\s]*)", newLogContent)
                if testStartFilenames:
                    self.lastTestSeen = testStartFilenames[-1]
                return newLogContent.strip('\n').strip()
            else:
                return ''

        @property
        def getLastTestSeen(self):
            return self.lastTestSeen

        def wait(self, timeout = None):
            timer = 0
            interval = 5

            if timeout == None:
                timeout = self.timeout

            while (self.dm.processExist(self.procName)):
                t = self.stdout
                if t != '': print t
                time.sleep(interval)
                timer += interval
                if (timer > timeout):
                    break

            
            print self.stdout

            if (timer >= timeout):
                return 1
            return 0

        def kill(self):
            self.dm.killProcess(self.procName)

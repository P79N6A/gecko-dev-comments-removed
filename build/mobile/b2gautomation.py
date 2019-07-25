



import automationutils
import threading
import os
import Queue
import re
import socket
import shutil
import sys
import tempfile
import time

from automation import Automation
from devicemanager import DeviceManager, NetworkTools
from mozprocess import ProcessHandlerMixin


class StdOutProc(ProcessHandlerMixin):
    """Process handler for b2g which puts all output in a Queue.
    """

    def __init__(self, cmd, queue, **kwargs):
        self.queue = queue
        kwargs.setdefault('processOutputLine', []).append(self.handle_output)
        ProcessHandlerMixin.__init__(self, cmd, **kwargs)

    def handle_output(self, line):
        self.queue.put_nowait(line)


class B2GRemoteAutomation(Automation):
    _devicemanager = None

    def __init__(self, deviceManager, appName='', remoteLog=None,
                 marionette=None, context_chrome=True):
        self._devicemanager = deviceManager
        self._appName = appName
        self._remoteProfile = None
        self._remoteLog = remoteLog
        self.marionette = marionette
        self.context_chrome = context_chrome
        self._is_emulator = False

        
        self._product = "b2g"
        
        self.logFinish = 'INFO SimpleTest FINISHED' 
        Automation.__init__(self)

    def setEmulator(self, is_emulator):
        self._is_emulator = is_emulator

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

    
    def environment(self, env=None, xrePath=None, crashreporter=True):
        
        
        if env is None:
            env = {}

        
        env['MOZ_HIDE_RESULTS_TABLE'] = '1'
        return env

    def waitForNet(self): 
        active = False
        time_out = 0
        while not active and time_out < 40:
            data = self._devicemanager.runCmd(['shell', '/system/bin/netcfg']).stdout.readlines()
            data.pop(0)
            for line in data:
                if (re.search(r'UP\s+(?:[0-9]{1,3}\.){3}[0-9]{1,3}', line)):
                    active = True
                    break
            time_out += 1
            time.sleep(1)
        return active

    def checkForCrashes(self, directory, symbolsPath):
        
        
        dumpDir = tempfile.mkdtemp()
        self._devicemanager.getDirectory(self._remoteProfile + '/minidumps/', dumpDir)
        automationutils.checkForCrashes(dumpDir, symbolsPath, self.lastTestSeen)
        try:
          shutil.rmtree(dumpDir)
        except:
          print "WARNING: unable to remove directory: %s" % (dumpDir)

    def initializeProfile(self, profileDir, extraPrefs = [], useServerLocations = False):
        
        extraPrefs.extend(["browser.manifestURL='dummy (bug 772307)'"])
        return Automation.initializeProfile(self, profileDir, extraPrefs, useServerLocations)

    def buildCommandLine(self, app, debuggerInfo, profileDir, testURL, extraArgs):
        
        if (self._remoteProfile):
            profileDir = self._remoteProfile

        cmd, args = Automation.buildCommandLine(self, app, debuggerInfo, profileDir, testURL, extraArgs)

        return app, args

    def getLanIp(self):
        nettools = NetworkTools()
        return nettools.getLanIp()

    def waitForFinish(self, proc, utilityPath, timeout, maxTime, startTime,
                      debuggerInfo, symbolsPath):
        """ Wait for tests to finish (as evidenced by a signature string
            in logcat), or for a given amount of time to elapse with no
            output.
        """
        timeout = timeout or 120

        didTimeout = False

        done = time.time() + timeout
        while True:
            currentlog = proc.stdout
            if currentlog:
                done = time.time() + timeout
                print currentlog
                if hasattr(self, 'logFinish') and self.logFinish in currentlog:
                    return 0
            else:
                if time.time() > done:
                    self.log.info("TEST-UNEXPECTED-FAIL | %s | application timed "
                                  "out after %d seconds with no output",
                                  self.lastTestSeen, int(timeout))
                    return 1

    def getDeviceStatus(self, serial=None):
        
        
        
        serial = serial or self._devicemanager.deviceSerial
        status = 'unknown'

        for line in self._devicemanager.runCmd(['devices']).stdout.readlines():
            result =  re.match('(.*?)\t(.*)', line)
            if result:
                thisSerial = result.group(1)
                if not serial or thisSerial == serial:
                    serial = thisSerial
                    status = result.group(2)

        return (serial, status)

    def restartB2G(self):
        
        time.sleep(5)
        self._devicemanager.checkCmd(['shell', 'stop', 'b2g'])
        
        time.sleep(10)
        self._devicemanager.checkCmd(['shell', 'start', 'b2g'])
        if self._is_emulator:
            self.marionette.emulator.wait_for_port()

    def rebootDevice(self):
        
        serial, status = self.getDeviceStatus()

        
        self._devicemanager.runCmd(['shell', '/system/bin/reboot'])

        
        
        time.sleep(10)

        
        print 'waiting for device to come back online after reboot'
        start = time.time()
        rserial, rstatus = self.getDeviceStatus(serial)
        while rstatus != 'device':
            if time.time() - start > 120:
                
                raise Exception("Device %s (status: %s) not back online after reboot" % (serial, rstatus))
            time.sleep(5)
            rserial, rstatus = self.getDeviceStatus(serial)
        print 'device:', serial, 'status:', rstatus

    def Process(self, cmd, stdout=None, stderr=None, env=None, cwd=None):
        
        
        
        
        
        
        

        
        
        
        
        if not self._is_emulator:
            self.rebootDevice()
            time.sleep(5)
            
            if not self.waitForNet():
                raise Exception("network did not come up, please configure the network" + 
                                " prior to running before running the automation framework")

        
        self._devicemanager.runCmd(['shell', 'stop', 'b2g'])
        time.sleep(5)

        
        instance = self.B2GInstance(self._devicemanager)

        time.sleep(5)

        
        
        if not self._is_emulator:
            self._devicemanager.checkCmd(['forward',
                                          'tcp:%s' % self.marionette.port,
                                          'tcp:%s' % self.marionette.port])

        if self._is_emulator:
            self.marionette.emulator.wait_for_port()
        else:
            time.sleep(5)

        
        session = self.marionette.start_session()
        if 'b2g' not in session:
            raise Exception("bad session value %s returned by start_session" % session)

        if self.context_chrome:
            self.marionette.set_context(self.marionette.CONTEXT_CHROME)

        
        if hasattr(self, 'testURL'):
            
            
            
            self.marionette.execute_script("document.getElementById('homescreen').src='%s';" % self.testURL)
        
        elif hasattr(self, 'testScript'):
            if os.path.isfile(self.testScript):
                script = open(self.testScript, 'r')
                self.marionette.execute_script(script.read())
                script.close()
            else:
                
                self.marionette.execute_script(self.testScript)
        else:
            
            pass

        return instance

    
    class B2GInstance(object):
        """Represents a B2G instance running on a device, and exposes
           some process-like methods/properties that are expected by the
           automation.
        """

        def __init__(self, dm):
            self.dm = dm
            self.stdout_proc = None
            self.queue = Queue.Queue()

            
            
            
            
            cmd = [self.dm.adbPath]
            if self.dm.deviceSerial:
                cmd.extend(['-s', self.dm.deviceSerial])
            cmd.append('shell')
            cmd.append('/system/bin/b2g.sh')
            proc = threading.Thread(target=self._save_stdout_proc, args=(cmd, self.queue))
            proc.daemon = True
            proc.start()

        def _save_stdout_proc(self, cmd, queue):
            self.stdout_proc = StdOutProc(cmd, queue)
            self.stdout_proc.run()
            if hasattr(self.stdout_proc, 'processOutput'):
                self.stdout_proc.processOutput()
            self.stdout_proc.waitForFinish()
            self.stdout_proc = None

        @property
        def pid(self):
            
            return 0

        @property
        def stdout(self):
            
            
            lines = []
            while True:
                try:
                    lines.append(self.queue.get_nowait())
                except Queue.Empty:
                    break
            return '\n'.join(lines)

        def wait(self, timeout = None):
            
            raise Exception("'wait' called on B2GInstance")

        def kill(self):
            
            raise Exception("'kill' called on B2GInstance")


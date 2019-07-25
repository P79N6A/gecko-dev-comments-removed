



import automationutils
import os
import re
import socket
import shutil
import sys
import tempfile
import time

from automation import Automation
from devicemanager import DeviceManager, NetworkTools

class B2GRemoteAutomation(Automation):
    _devicemanager = None

    def __init__(self, deviceManager, appName='', remoteLog=None,
                 marionette=None):
        self._devicemanager = deviceManager
        self._appName = appName
        self._remoteProfile = None
        self._remoteLog = remoteLog
        self.marionette = marionette

        
        self._product = "b2g"
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

    
    def environment(self, env=None, xrePath=None, crashreporter=True):
        
        
        if env is None:
            env = {}

        return env

    def checkForCrashes(self, directory, symbolsPath):
        
        
        dumpDir = tempfile.mkdtemp()
        self._devicemanager.getDirectory(self._remoteProfile + '/minidumps/', dumpDir)
        automationutils.checkForCrashes(dumpDir, symbolsPath, self.lastTestSeen)
        try:
          shutil.rmtree(dumpDir)
        except:
          print "WARNING: unable to remove directory: %s" % (dumpDir)

    def buildCommandLine(self, app, debuggerInfo, profileDir, testURL, extraArgs):
        
        if (self._remoteProfile):
            profileDir = self._remoteProfile

        cmd, args = Automation.buildCommandLine(self, app, debuggerInfo, profileDir, testURL, extraArgs)

        return app, args

    def getLanIp(self):
        nettools = NetworkTools()
        return nettools.getLanIp()

    def waitForFinish(self, proc, utilityPath, timeout, maxTime, startTime,
                      debuggerInfo, symbolsPath, logger):
        """ Wait for mochitest to finish (as evidenced by a signature string
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
                if 'INFO SimpleTest FINISHED' in currentlog:
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

    def rebootDevice(self):
        
        serial, status = self.getDeviceStatus()

        
        self._devicemanager.checkCmd(['reboot'])

        
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
        
        
        
        
        
        
        

        instance = self.B2GInstance(self._devicemanager)

        
        
        
        
        self.rebootDevice()

        
        
        
        time.sleep(20)

        
        
        self._devicemanager.checkCmd(['forward',
                                      'tcp:%s' % self.marionette.port,
                                      'tcp:%s' % self.marionette.port])

        
        session = self.marionette.start_session()
        if 'b2g' not in session:
            raise Exception("bad session value %s returned by start_session" % session)

        
        self.marionette.execute_script("window.location.href='%s';" % self.testURL)

        return instance

    
    class B2GInstance(object):
        """Represents a B2G instance running on a device, and exposes
           some process-like methods/properties that are expected by the
           automation.
        """

        def __init__(self, dm):
            self.dm = dm
            self.lastloglines = []

        @property
        def pid(self):
            
            return 0

        @property
        def stdout(self):
            
            
            
            
            t = self.dm.runCmd(['logcat', '-t', '50']).stdout.read()
            if t == None: return ''

            t = t.strip('\n').strip()
            loglines = t.split('\n')
            line_index = 0

            
            
            
            log_index = 20 if len(loglines) > 50 else 0

            for index, line in enumerate(loglines[log_index:]):
                line_index = index + log_index + 1
                try:
                    self.lastloglines.index(line)
                except ValueError:
                    break

            newoutput = '\n'.join(loglines[line_index:])
            self.lastloglines = loglines

            return newoutput

        def wait(self, timeout = None):
            
            raise Exception("'wait' called on B2GInstance")

        def kill(self):
            
            raise Exception("'kill' called on B2GInstance")


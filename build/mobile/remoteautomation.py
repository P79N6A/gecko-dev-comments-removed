





































import time
import sys
import os
import socket

from automation import Automation
from devicemanager import DeviceManager

class RemoteAutomation(Automation):
    _devicemanager = None
    
    def __init__(self, deviceManager, appName = '', remoteLog = None):
        self._devicemanager = deviceManager
        self._appName = appName
        self._remoteProfile = None
        self._remoteLog = remoteLog

        
        self._product = "fennec"
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

    def waitForFinish(self, proc, utilityPath, timeout, maxTime, startTime, debuggerInfo, symbolsDir):
        
        status = proc.wait(timeout = maxTime)

        print proc.stdout

        if (status == 1 and self._devicemanager.processExist(proc.procName)):
            
            proc.kill()

        return status

    def buildCommandLine(self, app, debuggerInfo, profileDir, testURL, extraArgs):
        
        if (self._remoteProfile):
            profileDir = self._remoteProfile

        cmd, args = Automation.buildCommandLine(self, app, debuggerInfo, profileDir, testURL, extraArgs)
        
        try:
            args.remove('-foreground')
        except:
            pass


        return app, args

    
    def getInterfaceIp(self, ifname):
        if os.name != "nt":
            import fcntl
            import struct
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            return socket.inet_ntoa(fcntl.ioctl(
                                    s.fileno(),
                                    0x8915,  
                                    struct.pack('256s', ifname[:15])
                                    )[20:24])
        else:
            return None

    def getLanIp(self):
        ip = socket.gethostbyname(socket.gethostname())
        if ip.startswith("127.") and os.name != "nt":
            interfaces = ["eth0","eth1","eth2","wlan0","wlan1","wifi0","ath0","ath1","ppp0"]
            for ifname in interfaces:
                try:
                    ip = self.getInterfaceIp(ifname)
                    break;
                except IOError:
                    pass
        return ip

    def Process(self, cmd, stdout = None, stderr = None, env = None, cwd = '.'):
        if stdout == None or stdout == -1:
          stdout = self._remoteLog

        return self.RProcess(self._devicemanager, cmd, stdout, stderr, env, cwd)

    
    class RProcess(object):
        
        dm = None
        def __init__(self, dm, cmd, stdout = None, stderr = None, env = None, cwd = '.'):
            self.dm = dm
            self.proc = dm.launchProcess(cmd, stdout)
            exepath = cmd[0]
            name = exepath.split('/')[-1]
            self.procName = name

            
            self.timeout = 3600
            time.sleep(15)

        @property
        def pid(self):
            hexpid = self.dm.processExist(self.procName)
            if (hexpid == '' or hexpid == None):
                hexpid = "0x0"
            return int(hexpid, 0)
    
        @property
        def stdout(self):
            return self.dm.getFile(self.proc)
 
        def wait(self, timeout = None):
            timer = 0
            interval = 5

            if timeout == None:
                timeout = self.timeout

            while (self.dm.processExist(self.procName)):
                time.sleep(interval)
                timer += interval
                if (timer > timeout):
                    break

            if (timer >= timeout):
                return 1
            return 0
 
        def kill(self):
            self.dm.killProcess(self.procName)









































import time
import hashlib
import socket
import os

class FileError(Exception):
  " Signifies an error which occurs while doing a file operation."

  def __init__(self, msg = ''):
    self.msg = msg

  def __str__(self):
    return self.msg

class DMError(Exception):
  "generic devicemanager exception."

  def __init__(self, msg= ''):
    self.msg = msg

  def __str__(self):
    return self.msg


class DeviceManager:
  
  
  
  
  def pushFile(self, localname, destname):
    assert 0 == 1
    return False

  
  
  
  
  def mkDir(self, name):
      assert 0 == 1
      return None

  
  
  
  
  
  def mkDirs(self, filename):
      assert 0 == 1
      return None

  
  
  
  
  
  def pushDir(self, localDir, remoteDir):
    assert 0 == 1
    return None

  
  
  
  
  def dirExists(self, dirname):
    assert 0 == 1
    return False

  
  
  
  
  
  
  def fileExists(self, filepath):
    assert 0 == 1
    return False

  
  
  
  
  
  def listFiles(self, rootdir):
    assert 0 == 1
    return []

  
  
  
  
  def removeFile(self, filename):
    assert 0 == 1
    return False

  
  
  
  
  
  def removeDir(self, remoteDir):
    assert 0 == 1
    return None

  
  
  
  
  def getProcessList(self):
    assert 0 == 1
    return []

  
  
  
  
  def fireProcess(self, appname, failIfRunning=False):
    assert 0 == 1
    return None

  
  
  
  
  def launchProcess(self, cmd, outputFile = "process.txt", cwd = '', env = '', failIfRunning=False):
    assert 0 == 1
    return None

  
  
  
  
  
  
  def communicate(self, process, timeout = 600, interval = 5):
    timed_out = True
    if (timeout > 0):
      total_time = 0
      while total_time < timeout:
        time.sleep(interval)
        if self.processExist(process) == None:
          timed_out = False
          break
        total_time += interval

    if (timed_out == True):
      return [None, None]

    return [self.getFile(process, "temp.txt"), None]

  
  
  
  
  
  def processExist(self, appname):
    pid = None

    
    parts = filter(lambda x: x != '', appname.split(' '))
    appname = ' '.join(parts)

    
    
    parts = appname.split('"')
    if (len(parts) > 2):
      appname = ' '.join(parts[2:]).strip()
  
    pieces = appname.split(' ')
    parts = pieces[0].split('/')
    app = parts[-1]
    procre = re.compile('.*' + app + '.*')

    procList = self.getProcessList()
    if (procList == []):
      return None
      
    for proc in procList:
      if (procre.match(proc[1])):
        pid = proc[0]
        break
    return pid

  
  
  
  
  def killProcess(self, appname):
    assert 0 == 1
    return None

  
  
  
  
  def catFile(self, remoteFile):
    assert 0 == 1
    return None

  
  
  
  
  def pullFile(self, remoteFile):
    assert 0 == 1
    return None

  
  
  
  
  
  def getFile(self, remoteFile, localFile = ''):
    assert 0 == 1
    return None

  
  
  
  
  
  
  
  
  def getDirectory(self, remoteDir, localDir, checkDir=True):
    assert 0 == 1
    return None

  
  
  
  
  
  def isDir(self, remotePath):
    assert 0 == 1
    return False

  
  
  
  
  
  def validateFile(self, remoteFile, localFile):
    assert 0 == 1
    return False

  
  
  
  
  
  def getRemoteHash(self, filename):
    assert 0 == 1
    return None

  
  
  
  
  
  def getLocalHash(self, filename):
    file = open(filename, 'rb')
    if (file == None):
      return None

    try:
      mdsum = hashlib.md5()
    except:
      return None

    while 1:
      data = file.read(1024)
      if not data:
        break
      mdsum.update(data)

    file.close()
    hexval = mdsum.hexdigest()
    if (self.debug >= 3): print "local hash returned: '" + hexval + "'"
    return hexval
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  def getDeviceRoot(self):
    assert 0 == 1
    return None

  
  
  
  
  
  
  
  def getAppRoot(self):
    devroot = self.getDeviceRoot()
    if (devroot == None):
      return None

    if (self.dirExists(devroot + '/fennec')):
      return devroot + '/fennec'
    elif (self.dirExists(devroot + '/firefox')):
      return devroot + '/firefox'
    elif (self.dirExsts('/data/data/org.mozilla.fennec')):
      return 'org.mozilla.fennec'
    elif (self.dirExists('/data/data/org.mozilla.firefox')):
      return 'org.mozilla.firefox'
    elif (self.dirExists('/data/data/org.mozilla.fennec_aurora')):
      return 'org.mozilla.fennec_aurora'
    elif (self.dirExists('/data/data/org.mozilla.firefox_beta')):
      return 'org.mozilla.firefox_beta'

    
    return None

  
  
  
  
  
  
  def getTestRoot(self, type):
    devroot = self.getDeviceRoot()
    if (devroot == None):
      return None

    if (re.search('xpcshell', type, re.I)):
      self.testRoot = devroot + '/xpcshell'
    elif (re.search('?(i)reftest', type)):
      self.testRoot = devroot + '/reftest'
    elif (re.search('?(i)mochitest', type)):
      self.testRoot = devroot + '/mochitest'
    return self.testRoot

  
  
  def signal(self, processID, signalType, signalAction):
    
    pass

  
  def getReturnCode(self, processID):
    
    return 0

  
  
  
  
  def unpackFile(self, filename):
    return None

  
  
  
  
  def reboot(self, ipAddr=None, port=30000):
    assert 0 == 1
    return None

  
  
  
  
  
  def validateDir(self, localDir, remoteDir):
    if (self.debug >= 2): print "validating directory: " + localDir + " to " + remoteDir
    for root, dirs, files in os.walk(localDir):
      parts = root.split(localDir)
      for file in files:
        remoteRoot = remoteDir + '/' + parts[1]
        remoteRoot = remoteRoot.replace('/', '/')
        if (parts[1] == ""): remoteRoot = remoteDir
        remoteName = remoteRoot + '/' + file
        if (self.validateFile(remoteName, os.path.join(root, file)) <> True):
            return False
    return True

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  def getInfo(self, directive=None):
    assert 0 == 1
    return {}

  
  
  
  
  def installApp(self, appBundlePath, destPath=None):
    assert 0 == 1
    return None

  
  
  
  
  def uninstallAppAndReboot(self, appName, installPath=None):
    assert 0 == 1
    return None

  
  
  
  
  def updateApp(self, appBundlePath, processName=None, destPath=None, ipAddr=None, port=30000):
    assert 0 == 1
    return None

  
  
  
  
  def getCurrentTime(self):
    assert 0 == 1
    return None


class NetworkTools:
  def __init__(self):
    pass

  
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

  
  def findOpenPort(self, ip, seed):
    try:
      s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
      connected = False
      if isinstance(seed, basestring):
        seed = int(seed)
      maxportnum = seed + 5000 
      while not connected:
        try:
          s.bind((ip, seed))
          connected = True
          s.close()
          break
        except:          
          if seed > maxportnum:
            print "Could not find open port after checking 5000 ports"
          raise
        seed += 1
    except:
      print "Socket error trying to find open port"
        
    return seed


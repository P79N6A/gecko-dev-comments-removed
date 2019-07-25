import subprocess
from devicemanager import DeviceManager, DMError
import re
import os
import sys

class DeviceManagerADB(DeviceManager):

  def __init__(self, host = None, port = 20701, retrylimit = 5, packageName = None):
    self.host = host
    self.port = port
    self.retrylimit = retrylimit
    self.retries = 0
    self._sock = None
    self.useRunAs = False
    self.packageName = None
    if packageName == None:
      if os.getenv('USER'):
        packageName = 'org.mozilla.fennec_' + os.getenv('USER')
      else:
        packageName = 'org.mozilla.fennec_'
    self.Init(packageName)

  def Init(self, packageName):
    
    
    try:
      self.verifyADB()
      self.verifyRunAs(packageName)
    except:
      self.useRunAs = False
      self.packageName = None
    try:
      
      files = self.listFiles("/data/data")
      if (len(files) == 1):
        if (files[0].find("Permission denied") != -1):
          print "NOT running as root"
          raise Exception("not running as root")
    except:
      try:
        self.checkCmd(["root"])
      except:
        print "restarting as root failed"

  
  
  
  
  def pushFile(self, localname, destname):
    try:
      if (os.name == "nt"):
        destname = destname.replace('\\', '/')
      if (self.useRunAs):
        remoteTmpFile = self.tmpDir + "/" + os.path.basename(localname)
        self.checkCmd(["push", os.path.realpath(localname), remoteTmpFile])
        self.checkCmdAs(["shell", "cp", remoteTmpFile, destname])
        self.checkCmd(["shell", "rm", remoteTmpFile])
      else:
        self.checkCmd(["push", os.path.realpath(localname), destname])
      if (self.isDir(destname)):
        destname = destname + "/" + os.path.basename(localname)
      self.chmodDir(destname)
      return True
    except:
      return False

  
  
  
  
  def mkDir(self, name):
    try:
      self.checkCmdAs(["shell", "mkdir", name])
      self.chmodDir(name)
      return name
    except:
      return None

  
  
  
  
  
  def mkDirs(self, filename):
    parts = filename.split('/')
    name = ""
    for part in parts:
      if (part == parts[-1]): break
      if (part != ""):
        name += '/' + part
        if (not self.dirExists(name)):
          if (self.mkDir(name) == None):
            print "failed making directory: " + str(name)
            return None
    return name

  
  
  
  
  
  def pushDir(self, localDir, remoteDir):
    
    
    
    try:
      if (not self.dirExists(remoteDir)):
        self.mkDirs(remoteDir+"/x")
      for root, dirs, files in os.walk(localDir, followlinks='true'):
        relRoot = os.path.relpath(root, localDir)
        for file in files:
          localFile = os.path.join(root, file)
          remoteFile = remoteDir + "/"
          if (relRoot!="."):
            remoteFile = remoteFile + relRoot + "/"
          remoteFile = remoteFile + file
          self.pushFile(localFile, remoteFile)
        for dir in dirs:
          targetDir = remoteDir + "/"
          if (relRoot!="."):
            targetDir = targetDir + relRoot + "/"
          targetDir = targetDir + dir
          if (not self.dirExists(targetDir)):
            self.mkDir(targetDir)
      self.checkCmdAs(["shell", "chmod", "777", remoteDir])
      return True
    except:
      print "pushing " + localDir + " to " + remoteDir + " failed"
      return False

  
  
  
  
  def dirExists(self, dirname):
    return self.isDir(dirname)

  
  
  
  
  
  
  def fileExists(self, filepath):
    p = self.runCmd(["shell", "ls", "-a", filepath])
    data = p.stdout.readlines()
    if (len(data) == 1):
      if (data[0].rstrip() == filepath):
        return True
    return False

  def removeFile(self, filename):
    return self.runCmd(["shell", "rm", filename]).stdout.read()

  
  
  
  
  
  def removeSingleDir(self, remoteDir):
    return self.runCmd(["shell", "rmdir", remoteDir]).stdout.read()

  
  
  
  
  
  def removeDir(self, remoteDir):
      out = ""
      if (self.isDir(remoteDir)):
          files = self.listFiles(remoteDir.strip())
          for f in files:
              if (self.isDir(remoteDir.strip() + "/" + f.strip())):
                  out += self.removeDir(remoteDir.strip() + "/" + f.strip())
              else:
                  out += self.removeFile(remoteDir.strip() + "/" + f.strip())
          out += self.removeSingleDir(remoteDir.strip())
      else:
          out += self.removeFile(remoteDir.strip())
      return out

  def isDir(self, remotePath):
      p = self.runCmd(["shell", "ls", "-a", remotePath])
      data = p.stdout.readlines()
      if (len(data) == 0):
          return True
      if (len(data) == 1):
          if (data[0].rstrip() == remotePath):
              return False
          if (data[0].find("No such file or directory") != -1):
              return False
          if (data[0].find("Not a directory") != -1):
              return False
      return True

  def listFiles(self, rootdir):
      p = self.runCmd(["shell", "ls", "-a", rootdir])
      data = p.stdout.readlines()
      if (len(data) == 1):
          if (data[0] == rootdir):
              return []
          if (data[0].find("No such file or directory") != -1):
              return []
          if (data[0].find("Not a directory") != -1):
              return []
      return data

  
  
  
  
  def getProcessList(self):
    p = self.runCmd(["shell", "ps"])
      
    p.stdout.readline()
    proc = p.stdout.readline()
    ret = []
    while (proc):
      els = proc.split()
      ret.append(list([els[1], els[len(els) - 1], els[0]]))
      proc =  p.stdout.readline()
    return ret

  
  
  
  
  def fireProcess(self, appname, failIfRunning=False):
    
    parts = appname.split('"');
    if (len(parts) > 2):
      parts = parts[2:]
    return self.launchProcess(parts, failIfRunning)

  
  
  
  
  def launchProcess(self, cmd, outputFile = "process.txt", cwd = '', env = '', failIfRunning=False):
    acmd = ["shell", "am","start"]
    cmd = ' '.join(cmd).strip()
    i = cmd.find(" ")
    acmd.append("-n")
    acmd.append(cmd[0:i] + "/.App")
    acmd.append("--es")
    acmd.append("args")
    acmd.append(cmd[i:])
    print acmd
    self.checkCmd(acmd)
    return outputFile;

  
  
  
  
  def killProcess(self, appname):
    procs = self.getProcessList()
    for proc in procs:
      if (proc[1] == appname):
        p = self.runCmd(["shell", "ps"])
        return p.stdout.read()
      return None

  
  
  
  
  def catFile(self, remoteFile):
    
    
    return self.getFile(remoteFile)

  
  
  
  
  def pullFile(self, remoteFile):
    
    return self.getFile(remoteFile)

  
  
  
  
  
  def getFile(self, remoteFile, localFile = 'tmpfile_dm_adb'):
    
    
    try:
      self.runCmd(["pull",  remoteFile, localFile]).stdout.read()
      f = open(localFile)
      ret = f.read()
      f.close()
      return ret;      
    except:
      return None

  
  
  
  
  
  
  
  
  def getDirectory(self, remoteDir, localDir, checkDir=True):
    ret = []
    p = self.runCmd(["pull", remoteDir, localDir])
    p.stderr.readline()
    line = p.stderr.readline()
    while (line):
      els = line.split()
      f = els[len(els) - 1]
      i = f.find(localDir)
      if (i != -1):
        if (localDir[len(localDir) - 1] != '/'):
          i = i + 1
        f = f[i + len(localDir):]
      i = f.find("/")
      if (i > 0):
        f = f[0:i]
      ret.append(f)
      line =  p.stderr.readline()
    
    ret.pop(len(ret) - 1)
    return ret



  
  
  
  
  
  def validateFile(self, remoteFile, localFile):
    return self.getRemoteHash(remoteFile) == self.getLocalHash(localFile)

  
  
  
  
  
  def getRemoteHash(self, filename):
    data = p = self.runCmd(["shell", "ls", "-l", filename]).stdout.read()
    return data.split()[3]

  def getLocalHash(self, filename):
    data = p = subprocess.Popen(["ls", "-l", filename], stdout=subprocess.PIPE).stdout.read()
    return data.split()[4]

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  def getDeviceRoot(self):
    
    
    testRoot = "/data/local/tests"
    if (self.dirExists(testRoot)):
      return testRoot
    root = "/mnt/sdcard"
    if (not self.dirExists(root)):
      root = "/data/local"
    testRoot = root + "/tests"
    if (not self.dirExists(testRoot)):
      self.mkDir(testRoot)
    return testRoot

  
  
  
  
  
  
  
  def getAppRoot(self):
    devroot = self.getDeviceRoot()
    if (devroot == None):
      return None

    if (self.dirExists(devroot + '/fennec')):
      return devroot + '/fennec'
    elif (self.dirExists(devroot + '/firefox')):
      return devroot + '/firefox'
    elif (self.packageName and self.dirExists('/data/data/' + self.packageName)):
      return '/data/data/' + self.packageName

    
    print "devicemanagerADB: getAppRoot failed"
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


  
  
  
  
  def reboot(self, wait = False):
    ret = self.runCmd(["reboot"]).stdout.read()
    if (not wait):
      return "Success"
    countdown = 40
    while (countdown > 0):
      countdown
      try:
        self.checkCmd(["wait-for-device", "shell", "ls", "/sbin"])
        return ret
      except:
        try:
          self.checkCmd(["root"])
        except:
          time.sleep(1)
          print "couldn't get root"
    return "Success"

  
  
  
  
  def updateApp(self, appBundlePath, processName=None, destPath=None, ipAddr=None, port=30000):
    return self.runCmd(["install", "-r", appBundlePath]).stdout.read()

  
  
  
  
  def getCurrentTime(self):
    timestr = self.runCmd(["shell", "date", "+%s"]).stdout.read().strip()
    if (not timestr or not timestr.isdigit()):
        return None
    return str(int(timestr)*1000)

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  def getInfo(self, directive="all"):
    ret = {}
    if (directive == "id" or directive == "all"):
      ret["id"] = self.runCmd(["get-serialno"]).stdout.read()
    if (directive == "os" or directive == "all"):
      ret["os"] = self.runCmd(["shell", "getprop", "ro.build.display.id"]).stdout.read()
    if (directive == "uptime" or directive == "all"):
      utime = self.runCmd(["shell", "uptime"]).stdout.read()
      if (not utime):
        raise DMError("error getting uptime")
      utime = utime[9:]
      hours = utime[0:utime.find(":")]
      utime = utime[utime[1:].find(":") + 2:]
      minutes = utime[0:utime.find(":")]
      utime = utime[utime[1:].find(":") +  2:]
      seconds = utime[0:utime.find(",")]
      ret["uptime"] = ["0 days " + hours + " hours " + minutes + " minutes " + seconds + " seconds"]
    if (directive == "process" or directive == "all"):
      ret["process"] = self.runCmd(["shell", "ps"]).stdout.read()
    if (directive == "systime" or directive == "all"):
      ret["systime"] = self.runCmd(["shell", "date"]).stdout.read()
    print ret
    return ret

  def runCmd(self, args):
    args.insert(0, "adb")
    return subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

  def checkCmd(self, args):
    args.insert(0, "adb")
    return subprocess.check_call(args)

  def checkCmdAs(self, args):
    if (self.useRunAs):
      args.insert(1, "run-as")
      args.insert(2, self.packageName)
    return self.checkCmd(args)

  def chmodDir(self, remoteDir):
    if (self.isDir(remoteDir)):
      files = self.listFiles(remoteDir.strip())
      for f in files:
        if (self.isDir(remoteDir.strip() + "/" + f.strip())):
          self.chmodDir(remoteDir.strip() + "/" + f.strip())
        else:
          self.checkCmdAs(["shell", "chmod", "777", remoteDir.strip()])
          print "chmod " + remoteDir.strip()
      self.checkCmdAs(["shell", "chmod", "777", remoteDir])
      print "chmod " + remoteDir
    else:
      self.checkCmdAs(["shell", "chmod", "777", remoteDir.strip()])
      print "chmod " + remoteDir.strip()

  def verifyADB(self):
    
    try:
      self.runCmd(["version"])
    except Exception as (ex):
      print "unable to execute ADB: ensure Android SDK is installed and adb is in your $PATH"
    
  def isCpAvailable(self):
    
    
    data = self.runCmd(["shell", "cp"]).stdout.read()
    if (re.search('Usage', data)):
      return True
    else:
      print "unable to execute 'cp' on device; consider installing busybox from Android Market"
      return False

  def verifyRunAs(self, packageName):
    
    
    
    
    
    
    
    self.useRunAs = False
    devroot = self.getDeviceRoot()
    if (packageName and self.isCpAvailable() and devroot):
      self.tmpDir = devroot + "/tmp"
      if (not self.dirExists(self.tmpDir)):
        self.mkDir(self.tmpDir)
      self.checkCmd(["shell", "run-as", packageName, "mkdir", devroot + "/sanity"])
      self.checkCmd(["push", os.path.abspath(sys.argv[0]), self.tmpDir + "/tmpfile"])
      self.checkCmd(["shell", "run-as", packageName, "cp", self.tmpDir + "/tmpfile", devroot + "/sanity"])
      if (self.fileExists(devroot + "/sanity/tmpfile")):
        print "will execute commands via run-as " + packageName
        self.packageName = packageName
        self.useRunAs = True
      self.checkCmd(["shell", "rm", devroot + "/tmp/tmpfile"])
      self.checkCmd(["shell", "run-as", packageName, "rm", "-r", devroot + "/sanity"])
      

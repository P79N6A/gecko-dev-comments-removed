import subprocess
from devicemanager import DeviceManager, DMError
import re

class DeviceManagerADB(DeviceManager):

  def __init__(self, host = None, port = 20701, retrylimit = 5):
    self.host = host
    self.port = port
    self.retrylimit = retrylimit
    self.retries = 0
    self._sock = None
    self.getDeviceRoot()
    try:
      
      self.checkCmd(["shell", "ls", "/sbin"])
    except:
      try:
        self.checkCmd(["root"])
      except:
        print "restarting as root failed"

  
  
  
  
  def pushFile(self, localname, destname):
    try:
      self.checkCmd(["push", localname, destname])
      self.chmodDir(destname)
      return True
    except:
      return False

  
  
  
  
  def mkDir(self, name):
    try:
      self.checkCmd(["shell", "mkdir", name])
      return name
    except:
      return None

  
  
  
  
  
  def mkDirs(self, filename):
    self.checkCmd(["shell", "mkdir", "-p ", name])
    return filename

  
  
  
  
  
  def pushDir(self, localDir, remoteDir):
    try:
      self.checkCmd(["push", localDir, remoteDir])
      self.chmodDir(remoteDir)
      return True
    except:
      print "pushing " + localDir + " to " + remoteDir + " failed"
      return False

  
  
  
  
  def dirExists(self, dirname):
    try:
      self.checkCmd(["shell", "ls", dirname])
      return True
    except:
      return False

  
  
  
  
  
  
  def fileExists(self, filepath):
    self.checkCmd(["shell", "ls", filepath])
    return True

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
                  out += self.removeFile(remoteDir.strip())
          out += self.removeSingleDir(remoteDir)
      else:
          out += self.removeFile(remoteDir.strip())
      return out

  def isDir(self, remotePath):
      p = self.runCmd(["shell", "ls", remotePath])
      data = p.stdout.readlines()
      if (len(data) == 0):
          return True
      if (len(data) == 1):
          if (data[0] == remotePath):
              return False
          if (data[0].find("No such file or directory") != -1):
              return False
          if (data[0].find("Not a directory") != -1):
              return False
      return True

  def listFiles(self, rootdir):
      p = self.runCmd(["shell", "ls", rootdir])
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
    return self.runCmd(["shell", appname]).pid

  
  
  
  
  def launchProcess(self, cmd, outputFile = "process.txt", cwd = '', env = '', failIfRunning=False):
    acmd = ["shell", "am","start"]
    cmd = ' '.join(cmd)
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
      self.checkCmd(["pull",  remoteFile, localFile])
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
    if (not self.dirExists("/data/local/tests")):
      self.mkDir("/data/local/tests")
    return "/data/local/tests"

  
  
  
  
  
  
  
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

  def chmodDir(self, remoteDir):
    print "called chmodDir"
    if (self.isDir(remoteDir)):
      files = self.listFiles(remoteDir.strip())
      for f in files:
        if (self.isDir(remoteDir.strip() + "/" + f.strip())):
          self.chmodDir(remoteDir.strip() + "/" + f.strip())
        else:
          self.checkCmd(["shell", "chmod", "777", remoteDir.strip()])
          print "chmod " + remoteDir.strip()
      self.checkCmd(["shell", "chmod", "777", remoteDir])
      print "chmod " + remoteDir
    else:
      self.checkCmd(["shell", "chmod", "777", remoteDir.strip()])
      print "chmod " + remoteDir

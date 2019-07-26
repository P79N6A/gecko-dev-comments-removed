



import subprocess
from devicemanager import DeviceManager, DMError, _pop_last_line
import re
import os
import sys
import tempfile

class DeviceManagerADB(DeviceManager):

  def __init__(self, host=None, port=20701, retrylimit=5, packageName='fennec',
               adbPath='adb', deviceSerial=None, deviceRoot=None):
    self.host = host
    self.port = port
    self.retrylimit = retrylimit
    self.retries = 0
    self._sock = None
    self.useRunAs = False
    self.haveRoot = False
    self.useDDCopy = False
    self.useZip = False
    self.packageName = None
    self.tempDir = None
    self.deviceRoot = deviceRoot

    
    self.adbPath = adbPath

    
    
    self.deviceSerial = deviceSerial

    if packageName == 'fennec':
      if os.getenv('USER'):
        self.packageName = 'org.mozilla.fennec_' + os.getenv('USER')
      else:
        self.packageName = 'org.mozilla.fennec_'
    elif packageName:
      self.packageName = packageName

    
    self.verifyADB()

    
    if self.host:
      self.connectRemoteADB()

    
    self.verifyDevice()

    
    self.setupDeviceRoot()

    
    try:
      self.verifyRunAs()
    except DMError:
      pass

    
    useRunAsTmp = self.useRunAs
    self.useRunAs = False
    try:
      self.verifyRoot()
    except DMError, e:
      try:
        self.checkCmd(["root"])
        
        
        
        self.verifyRoot()
      except DMError:
        if useRunAsTmp:
          print "restarting as root failed, but run-as available"
        else:
          print "restarting as root failed"
    self.useRunAs = useRunAsTmp

    
    
    try:
      self.verifyZip()
    except DMError:
      pass

  def __del__(self):
    if self.host:
      self.disconnectRemoteADB()

  
  
  
  
  def shell(self, cmd, outputfile, env=None, cwd=None):
    
    

    
    
    
    cmdline = "%s; echo $?" % self._escapedCommandLine(cmd)

    
    if cwd:
      cmdline = "cd %s; %s" % (cwd, cmdline)
    if env:
      envstr = '; '.join(map(lambda x: 'export %s=%s' % (x[0], x[1]), env.iteritems()))
      cmdline = envstr + "; " + cmdline

    
    args=[self.adbPath]
    if self.deviceSerial:
        args.extend(['-s', self.deviceSerial])
    args.extend(["shell", cmdline])
    proc = subprocess.Popen(args,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (stdout, stderr) = proc.communicate()
    outputfile.write(stdout.rstrip('\n'))

    lastline = _pop_last_line(outputfile)
    if lastline:
      m = re.search('([0-9]+)', lastline)
      if m:
        return_code = m.group(1)
        outputfile.seek(-2, 2)
        outputfile.truncate() 
        return int(return_code)

    return None

  def connectRemoteADB(self):
    self.checkCmd(["connect", self.host + ":" + str(self.port)])

  def disconnectRemoteADB(self):
    self.checkCmd(["disconnect", self.host + ":" + str(self.port)])

  
  
  
  
  def pushFile(self, localname, destname):
    try:
      if (os.name == "nt"):
        destname = destname.replace('\\', '/')
      if (self.useRunAs):
        remoteTmpFile = self.getTempDir() + "/" + os.path.basename(localname)
        self.checkCmd(["push", os.path.realpath(localname), remoteTmpFile])
        if self.useDDCopy:
          self.checkCmdAs(["shell", "dd", "if=" + remoteTmpFile, "of=" + destname])
        else:
          self.checkCmdAs(["shell", "cp", remoteTmpFile, destname])
        self.checkCmd(["shell", "rm", remoteTmpFile])
      else:
        self.checkCmd(["push", os.path.realpath(localname), destname])
      if (self.isDir(destname)):
        destname = destname + "/" + os.path.basename(localname)
      return True
    except:
      return False

  
  
  
  
  def mkDir(self, name):
    try:
      result = self.runCmdAs(["shell", "mkdir", name]).stdout.read()
      if 'read-only file system' in result.lower():
        return None
      if 'file exists' in result.lower():
        return name
      return name
    except:
      return None

  
  
  
  
  
  def pushDir(self, localDir, remoteDir):
    
    
    
    
    try:
      if (not self.dirExists(remoteDir)):
        self.mkDirs(remoteDir+"/x")
      if (self.useZip):
        try:
          localZip = tempfile.mktemp()+".zip"
          remoteZip = remoteDir + "/adbdmtmp.zip"
          subprocess.check_output(["zip", "-r", localZip, '.'], cwd=localDir)
          self.pushFile(localZip, remoteZip)
          os.remove(localZip)
          data = self.runCmdAs(["shell", "unzip", "-o", remoteZip, "-d", remoteDir]).stdout.read()
          self.checkCmdAs(["shell", "rm", remoteZip])
          if (re.search("unzip: exiting", data) or re.search("Operation not permitted", data)):
            raise Exception("unzip failed, or permissions error")
        except:
          print "zip/unzip failure: falling back to normal push"
          self.useZip = False
          self.pushDir(localDir, remoteDir)
      else:
        for root, dirs, files in os.walk(localDir, followlinks=True):
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
      return remoteDir
    except:
      print "pushing " + localDir + " to " + remoteDir + " failed"
      return None

  
  
  
  
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
      data[:] = [item.rstrip('\r\n') for item in data]
      if (len(data) == 1):
          if (data[0] == rootdir):
              return []
          if (data[0].find("No such file or directory") != -1):
              return []
          if (data[0].find("Not a directory") != -1):
              return []
          if (data[0].find("Permission denied") != -1):
              return []
          if (data[0].find("opendir failed") != -1):
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
    if cmd[0] == "am":
      self.checkCmd(["shell"] + cmd)
      return outputFile

    acmd = ["shell", "am", "start", "-W"]
    cmd = ' '.join(cmd).strip()
    i = cmd.find(" ")
    
    re_url = re.compile('^[http|file|chrome|about].*')
    last = cmd.rfind(" ")
    uri = ""
    args = ""
    if re_url.match(cmd[last:].strip()):
      args = cmd[i:last].strip()
      uri = cmd[last:].strip()
    else:
      args = cmd[i:].strip()
    acmd.append("-n")
    acmd.append(cmd[0:i] + "/.App")
    if args != "":
      acmd.append("--es")
      acmd.append("args")
      acmd.append(args)
    if env != '' and env != None:
      envCnt = 0
      
      for envkey, envval in env.iteritems():
        acmd.append("--es")
        acmd.append("env" + str(envCnt))
        acmd.append(envkey + "=" + envval);
        envCnt += 1
    if uri != "":
      acmd.append("-d")
      acmd.append(''.join(['\'',uri, '\'']));
    print acmd
    self.checkCmd(acmd)
    return outputFile

  
  
  
  
  def killProcess(self, appname, forceKill=False):
    procs = self.getProcessList()
    didKillProcess = False
    for (pid, name, user) in procs:
      if name == appname:
         args = ["shell", "kill"]
         if forceKill:
           args.append("-9")
         args.append(pid)
         p = self.runCmdAs(args)
         didKillProcess = True

    return didKillProcess

  
  
  
  
  def catFile(self, remoteFile):
    
    
    return self.getFile(remoteFile)

  
  
  
  
  def pullFile(self, remoteFile):
    
    return self.getFile(remoteFile)

  
  
  
  
  
  def getFile(self, remoteFile, localFile = 'tmpfile_dm_adb'):
    
    
    try:

      
      outerr = self.runCmd(["pull",  remoteFile, localFile]).communicate()

      
      if outerr[1]:
        errl = outerr[1].splitlines()
        if (len(errl) == 1):
          if (((errl[0].find("Permission denied") != -1)
            or (errl[0].find("does not exist") != -1))
            and self.useRunAs):
            
            
            
            remoteTmpFile = self.getTempDir() + "/" + os.path.basename(remoteFile)
            self.checkCmdAs(["shell", "dd", "if=" + remoteFile, "of=" + remoteTmpFile])
            self.checkCmdAs(["shell", "chmod", "777", remoteTmpFile])
            self.runCmd(["pull",  remoteTmpFile, localFile]).stdout.read()
            
            self.checkCmdAs(["shell", "rm", remoteTmpFile])

      f = open(localFile)
      ret = f.read()
      f.close()
      return ret
    except:
      return None

  
  
  
  
  
  
  
  
  def getDirectory(self, remoteDir, localDir, checkDir=True):
    ret = []
    p = self.runCmd(["pull", remoteDir, localDir])
    p.stdout.readline()
    line = p.stdout.readline()
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
      line =  p.stdout.readline()
    
    if (len(ret) > 0):
      ret.pop()
    return ret



  
  
  
  
  
  def validateFile(self, remoteFile, localFile):
    return self.getRemoteHash(remoteFile) == self.getLocalHash(localFile)

  
  
  
  
  
  def getRemoteHash(self, filename):
    data = p = self.runCmd(["shell", "ls", "-l", filename]).stdout.read()
    return data.split()[3]

  def getLocalHash(self, filename):
    data = p = subprocess.Popen(["ls", "-l", filename], stdout=subprocess.PIPE).stdout.read()
    return data.split()[4]

  
  def setupDeviceRoot(self):
    
    if self.deviceRoot:
      if not self.dirExists(self.deviceRoot):
        if not self.mkDir(self.deviceRoot):
          raise DMError("Unable to create device root %s" % self.deviceRoot)
      return

    
    
    testRoot = "/data/local/tests"
    if (self.dirExists(testRoot)):
      self.deviceRoot = testRoot
      return

    for (basePath, subPath) in [('/mnt/sdcard', 'tests'),
                                ('/data/local', 'tests')]:
      if self.dirExists(basePath):
        testRoot = os.path.join(basePath, subPath)
        if self.mkDir(testRoot):
          self.deviceRoot = testRoot
          return

    raise DMError("Unable to set up device root as /mnt/sdcard/tests "
                  "or /data/local/tests")

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  def getDeviceRoot(self):
    return self.deviceRoot

  
  
  
  
  
  
  
  def getTempDir(self):
    
    
    if self.tempDir == None:
      self.tempDir = self.getDeviceRoot() + "/tmp"
      if (not self.dirExists(self.tempDir)):
        return self.mkDir(self.tempDir)

    return self.tempDir

  
  
  
  
  
  
  
  def getAppRoot(self, packageName):
    devroot = self.getDeviceRoot()
    if (devroot == None):
      return None

    if (packageName and self.dirExists('/data/data/' + packageName)):
      self.packageName = packageName
      return '/data/data/' + packageName
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
    
    
    finalArgs = [self.adbPath]
    if self.deviceSerial:
      finalArgs.extend(['-s', self.deviceSerial])
    if (not self.haveRoot and self.useRunAs and args[0] == "shell" and args[1] != "run-as"):
      args.insert(1, "run-as")
      args.insert(2, self.packageName)
    finalArgs.extend(args)
    return subprocess.Popen(finalArgs, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

  def runCmdAs(self, args):
    if self.useRunAs:
      args.insert(1, "run-as")
      args.insert(2, self.packageName)
    return self.runCmd(args)

  def checkCmd(self, args):
    
    
    finalArgs = [self.adbPath]
    if self.deviceSerial:
      finalArgs.extend(['-s', self.deviceSerial])
    if (not self.haveRoot and self.useRunAs and args[0] == "shell" and args[1] != "run-as"):
      args.insert(1, "run-as")
      args.insert(2, self.packageName)
    finalArgs.extend(args)
    return subprocess.check_call(finalArgs)

  def checkCmdAs(self, args):
    if (self.useRunAs):
      args.insert(1, "run-as")
      args.insert(2, self.packageName)
    return self.checkCmd(args)

  
  
  
  
  def chmodDir(self, remoteDir):
    if (self.isDir(remoteDir)):
      files = self.listFiles(remoteDir.strip())
      for f in files:
        remoteEntry = remoteDir.strip() + "/" + f.strip()
        if (self.isDir(remoteEntry)):
          self.chmodDir(remoteEntry)
        else:
          self.checkCmdAs(["shell", "chmod", "777", remoteEntry])
          print "chmod " + remoteEntry
      self.checkCmdAs(["shell", "chmod", "777", remoteDir])
      print "chmod " + remoteDir
    else:
      self.checkCmdAs(["shell", "chmod", "777", remoteDir.strip()])
      print "chmod " + remoteDir.strip()
    return True

  def verifyADB(self):
    
    if self.adbPath != 'adb':
      if not os.access(self.adbPath, os.X_OK):
        raise DMError("invalid adb path, or adb not executable: %s", self.adbPath)

    try:
      self.checkCmd(["version"])
    except os.error, err:
      raise DMError("unable to execute ADB (%s): ensure Android SDK is installed and adb is in your $PATH" % err)
    except subprocess.CalledProcessError:
      raise DMError("unable to execute ADB: ensure Android SDK is installed and adb is in your $PATH")

  def verifyDevice(self):
    
    if self.deviceSerial:
      deviceStatus = None
      proc = subprocess.Popen([self.adbPath, "devices"],
                              stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT)
      for line in proc.stdout:
        m = re.match('(.+)?\s+(.+)$', line)
        if m:
          if self.deviceSerial == m.group(1):
            deviceStatus = m.group(2)
      if deviceStatus == None:
        raise DMError("device not found: %s" % self.deviceSerial)
      elif deviceStatus != "device":
        raise DMError("bad status for device %s: %s" % (self.deviceSerial,
                                                        deviceStatus))

    
    try:
      self.checkCmd(["shell", "echo"])
    except subprocess.CalledProcessError:
      raise DMError("unable to connect to device: is it plugged in?")

  def verifyRoot(self):
    
    files = self.listFiles("/data/data")
    if (len(files) == 0):
      print "NOT running as root"
      raise DMError("not running as root")

    self.haveRoot = True

  def isCpAvailable(self):
    
    
    data = self.runCmd(["shell", "cp"]).stdout.read()
    if (re.search('Usage', data)):
      return True
    else:
      data = self.runCmd(["shell", "dd", "-"]).stdout.read()
      if (re.search('unknown operand', data)):
        print "'cp' not found, but 'dd' was found as a replacement"
        self.useDDCopy = True
        return True
      print "unable to execute 'cp' on device; consider installing busybox from Android Market"
      return False

  def verifyRunAs(self):
    
    
    
    
    
    
    
    self.useRunAs = False
    devroot = self.getDeviceRoot()
    if (self.packageName and self.isCpAvailable() and devroot):
      tmpDir = self.getTempDir()

      
      
      runAsOut = self.runCmd(["shell", "run-as", self.packageName, "mkdir", devroot + "/sanity"]).communicate()[0]
      if runAsOut.startswith("run-as:") and ("not debuggable" in runAsOut or
                                             "is unknown" in runAsOut):
        raise DMError("run-as failed sanity check")

      tmpfile = tempfile.NamedTemporaryFile()
      self.checkCmd(["push", tmpfile.name, tmpDir + "/tmpfile"])
      if self.useDDCopy:
        self.checkCmd(["shell", "run-as", self.packageName, "dd", "if=" + tmpDir + "/tmpfile", "of=" + devroot + "/sanity/tmpfile"])
      else:
        self.checkCmd(["shell", "run-as", self.packageName, "cp", tmpDir + "/tmpfile", devroot + "/sanity"])
      if (self.fileExists(devroot + "/sanity/tmpfile")):
        print "will execute commands via run-as " + self.packageName
        self.useRunAs = True
      self.checkCmd(["shell", "rm", devroot + "/tmp/tmpfile"])
      self.checkCmd(["shell", "run-as", self.packageName, "rm", "-r", devroot + "/sanity"])

  def isUnzipAvailable(self):
    data = self.runCmdAs(["shell", "unzip"]).stdout.read()
    if (re.search('Usage', data)):
      return True
    else:
      return False

  def isLocalZipAvailable(self):
    try:
      subprocess.check_call(["zip", "-?"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    except:
      return False
    return True

  def verifyZip(self):
    
    
    
    self.useZip = False
    if (self.isUnzipAvailable() and self.isLocalZipAvailable()):
      print "will use zip to push directories"
      self.useZip = True
    else:
      raise DMError("zip not available")

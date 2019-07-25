






































import socket
import SocketServer
import time, datetime
import os
import re
import hashlib
import subprocess
from threading import Thread
import traceback
import sys
from devicemanager import DeviceManager, DMError, FileError, NetworkTools

class DeviceManagerSUT(DeviceManager):
  host = ''
  port = 0
  debug = 2 
  retries = 0
  tempRoot = os.getcwd()
  base_prompt = '$>'
  base_prompt_re = '\$\>'
  prompt_sep = '\x00'
  prompt_regex = '.*(' + base_prompt_re + prompt_sep + ')'
  agentErrorRE = re.compile('^##AGENT-WARNING##.*')

  
  
  
  
  
  

  def __init__(self, host, port = 20701, retrylimit = 5):
    self.host = host
    self.port = port
    self.retrylimit = retrylimit
    self.retries = 0
    self._sock = None
    self.getDeviceRoot()

  def cmdNeedsResponse(self, cmd):
    """ Not all commands need a response from the agent:
        * if the cmd matches the pushRE then it is the first half of push
          and therefore we want to wait until the second half before looking
          for a response
        * rebt obviously doesn't get a response
        * uninstall performs a reboot to ensure starting in a clean state and
          so also doesn't look for a response
    """
    noResponseCmds = [re.compile('^push .*$'),
                      re.compile('^rebt'),
                      re.compile('^uninst .*$'),
                      re.compile('^pull .*$')]

    for c in noResponseCmds:
      if (c.match(cmd)):
        return False
    
    
    return True

  def shouldCmdCloseSocket(self, cmd):
    """ Some commands need to close the socket after they are sent:
    * push
    * rebt
    * uninst
    * quit
    """
    
    socketClosingCmds = [re.compile('^push .*$'),
                         re.compile('^quit.*'),
                         re.compile('^rebt.*'),
                         re.compile('^uninst .*$')]

    for c in socketClosingCmds:
      if (c.match(cmd)):
        return True

    return False

  
  def verifySendCMD(self, cmdline, newline = True):
    return self.sendCMD(cmdline, newline, False)


  
  
  
  
  
  
  
  
  def sendCMD(self, cmdline, newline = True, ignoreAgentErrors = True):
    done = False
    while (not done):
      retVal = self._doCMD(cmdline, newline)
      if (retVal is None):
        self.retries += 1
      else:
        self.retries = 0
        if ignoreAgentErrors == False:
          if (self.agentErrorRE.match(retVal)):
            raise DMError("error on the agent executing '%s'" % cmdline)
        return retVal

      if (self.retries >= self.retrylimit):
        done = True

    raise DMError("unable to connect to %s after %s attempts" % (self.host, self.retrylimit))        

  def _doCMD(self, cmdline, newline = True):
    promptre = re.compile(self.prompt_regex + '$')
    data = ""
    shouldCloseSocket = False
    recvGuard = 1000

    if (self._sock == None):
      try:
        if (self.debug >= 1):
          print "reconnecting socket"
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      except:
        self._sock = None
        if (self.debug >= 2):
          print "unable to create socket"
        return None
      
      try:
        self._sock.connect((self.host, int(self.port)))
        self._sock.recv(1024)
      except:
        self._sock.close()
        self._sock = None
        if (self.debug >= 2):
          print "unable to connect socket"
        return None
    
    for cmd in cmdline:
      if newline: cmd += '\r\n'
      
      try:
        numbytes = self._sock.send(cmd)
        if (numbytes != len(cmd)):
          print "ERROR: our cmd was " + str(len(cmd)) + " bytes and we only sent " + str(numbytes)
          return None
        if (self.debug >= 4): print "send cmd: " + str(cmd)
      except:
        self._sock.close()
        self._sock = None
        return None
      
      
      shouldCloseSocket = self.shouldCmdCloseSocket(cmd)

      
      if (self.cmdNeedsResponse(cmd)):
        found = False
        loopguard = 0

        while (found == False and (loopguard < recvGuard)):
          temp = ''
          if (self.debug >= 4): print "recv'ing..."

          
          try:
            temp = self._sock.recv(1024)
            if (self.debug >= 4): print "response: " + str(temp)
          except:
            self._sock.close()
            self._sock = None
            return None

          
          
          if (self.agentErrorRE.match(temp)):
            data = temp
            break

          lines = temp.split('\n')

          for line in lines:
            if (promptre.match(line)):
              found = True
          data += temp

          
          
          if (temp == ''):
            loopguard += 1

    if (shouldCloseSocket == True):
      try:
        self._sock.close()
        self._sock = None
      except:
        self._sock = None
        return None

    return data
  
  
  
  def stripPrompt(self, data):
    promptre = re.compile(self.prompt_regex + '.*')
    retVal = []
    lines = data.split('\n')
    for line in lines:
      try:
        while (promptre.match(line)):
          pieces = line.split(self.prompt_sep)
          index = pieces.index('$>')
          pieces.pop(index)
          line = self.prompt_sep.join(pieces)
      except(ValueError):
        pass
      retVal.append(line)

    return '\n'.join(retVal)
  

  
  
  
  
  def pushFile(self, localname, destname):
    if (os.name == "nt"):
      destname = destname.replace('\\', '/')

    if (self.debug >= 3): print "in push file with: " + localname + ", and: " + destname
    if (self.validateFile(destname, localname) == True):
      if (self.debug >= 3): print "files are validated"
      return True

    if self.mkDirs(destname) == None:
      print "unable to make dirs: " + destname
      return False

    if (self.debug >= 3): print "sending: push " + destname
    
    filesize = os.path.getsize(localname)
    f = open(localname, 'rb')
    data = f.read()
    f.close()

    try:
      retVal = self.verifySendCMD(['push ' + destname + ' ' + str(filesize) + '\r\n', data], newline = False)
    except(DMError):
      retVal = False
  
    if (self.debug >= 3): print "push returned: " + str(retVal)

    validated = False
    if (retVal):
      retline = self.stripPrompt(retVal).strip() 
      if (retline == None):
        
        validated = self.validateFile(destname, localname)
      else:
        
        localHash = self.getLocalHash(localname)
        if (str(localHash) == str(retline)):
          validated = True
    else:
      
      validated = self.validateFile(destname, localname)

    if (validated):
      if (self.debug >= 3): print "Push File Validated!"
      return True
    else:
      if (self.debug >= 2): print "Push File Failed to Validate!"
      return False
  
  
  
  
  
  def mkDir(self, name):
    if (self.dirExists(name)):
      return name
    else:
      try:
        retVal = self.verifySendCMD(['mkdr ' + name])
      except(DMError):
        retVal = None
      return retVal

  
  
  
  
  
  def mkDirs(self, filename):
    parts = filename.split('/')
    name = ""
    for part in parts:
      if (part == parts[-1]): break
      if (part != ""):
        name += '/' + part
        if (self.mkDir(name) == None):
          print "failed making directory: " + str(name)
          return None
    return name

  
  
  
  
  
  def pushDir(self, localDir, remoteDir):
    if (self.debug >= 2): print "pushing directory: %s to %s" % (localDir, remoteDir)
    for root, dirs, files in os.walk(localDir):
      parts = root.split(localDir)
      for file in files:
        remoteRoot = remoteDir + '/' + parts[1]
        remoteName = remoteRoot + '/' + file
        if (parts[1] == ""): remoteRoot = remoteDir
        if (self.pushFile(os.path.join(root, file), remoteName) == False):
          
          self.removeFile(remoteName)
          if (self.pushFile(os.path.join(root, file), remoteName) == False):
            return None
    return remoteDir

  
  
  
  
  def dirExists(self, dirname):
    match = ".*" + dirname + "$"
    dirre = re.compile(match)
    try:
      data = self.verifySendCMD(['cd ' + dirname, 'cwd'])
    except(DMError):
      return False

    retVal = self.stripPrompt(data)
    data = retVal.split('\n')
    found = False
    for d in data:
      if (dirre.match(d)): 
        found = True

    return found

  
  
  
  
  
  
  def fileExists(self, filepath):
    s = filepath.split('/')
    containingpath = '/'.join(s[:-1])
    listfiles = self.listFiles(containingpath)
    for f in listfiles:
      if (f == s[-1]):
        return True
    return False

  
  
  
  
  
  def listFiles(self, rootdir):
    rootdir = rootdir.rstrip('/')
    if (self.dirExists(rootdir) == False):
      return []
    try:
      data = self.verifySendCMD(['cd ' + rootdir, 'ls'])
    except(DMError):
      return []

    retVal = self.stripPrompt(data)
    files = filter(lambda x: x, retVal.split('\n'))
    if len(files) == 1 and files[0] == '<empty>':
      
      return []
    return files

  
  
  
  
  def removeFile(self, filename):
    if (self.debug>= 2): print "removing file: " + filename
    try:
      retVal = self.verifySendCMD(['rm ' + filename])
    except(DMError):
      return None

    return retVal
  
  
  
  
  
  
  def removeDir(self, remoteDir):
    try:
      retVal = self.verifySendCMD(['rmdr ' + remoteDir])
    except(DMError):
      return None

    return retVal

  
  
  
  
  def getProcessList(self):
    try:
      data = self.verifySendCMD(['ps'])
    except DMError:
      return []

    retVal = self.stripPrompt(data)
    lines = retVal.split('\n')
    files = []
    for line in lines:
      if (line.strip() != ''):
        pidproc = line.strip().split()
        if (len(pidproc) == 2):
          files += [[pidproc[0], pidproc[1]]]
        elif (len(pidproc) == 3):
          
          files += [[pidproc[1], pidproc[2], pidproc[0]]]     
    return files

  
  
  
  
  def fireProcess(self, appname, failIfRunning=False):
    if (not appname):
      if (self.debug >= 1): print "WARNING: fireProcess called with no command to run"
      return None

    if (self.debug >= 2): print "FIRE PROC: '" + appname + "'"

    if (self.processExist(appname) != None):
      print "WARNING: process %s appears to be running already\n" % appname
      if (failIfRunning):
        return None
    
    try:
      data = self.verifySendCMD(['exec ' + appname])
    except(DMError):
      return None

    
    timeslept = 0
    while (timeslept <= 30):
      process = self.processExist(appname)
      if (process is not None):
        break
      time.sleep(3)
      timeslept += 3

    if (self.debug >= 4): print "got pid: %s for process: %s" % (process, appname)
    return process

  
  
  
  
  def launchProcess(self, cmd, outputFile = "process.txt", cwd = '', env = '', failIfRunning=False):
    if not cmd:
      if (self.debug >= 1): print "WARNING: launchProcess called without command to run"
      return None

    cmdline = subprocess.list2cmdline(cmd)
    if (outputFile == "process.txt" or outputFile == None):
      outputFile = self.getDeviceRoot();
      if outputFile is None:
        return None
      outputFile += "/process.txt"
      cmdline += " > " + outputFile
    
    
    cmdline = '%s %s' % (self.formatEnvString(env), cmdline)

    if self.fireProcess(cmdline, failIfRunning) is None:
      return None
    return outputFile
  
  
  
  
  
  
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
    try:
      data = self.verifySendCMD(['kill ' + appname])
    except(DMError):
      return None

    return data

  
  
  
  
  def getTempDir(self):
    try:
      data = self.verifySendCMD(['tmpd'])
    except(DMError):
      return None

    return self.stripPrompt(data).strip('\n')

  
  
  
  
  def catFile(self, remoteFile):
    try:
      data = self.verifySendCMD(['cat ' + remoteFile])
    except(DMError):
      return None

    return self.stripPrompt(data)
  
  
  
  
  
  def pullFile(self, remoteFile):
    """Returns contents of remoteFile using the "pull" command.
    The "pull" command is different from other commands in that DeviceManager
    has to read a certain number of bytes instead of just reading to the
    next prompt.  This is more robust than the "cat" command, which will be
    confused if the prompt string exists within the file being catted.
    However it means we can't use the response-handling logic in sendCMD().
    """
    
    def err(error_msg):
        err_str = 'error returned from pull: %s' % error_msg
        print err_str
        self._sock = None
        raise FileError(err_str) 

    
    
    
    
    def uread(to_recv, error_msg):
      """ unbuffered read """
      try:
        data = self._sock.recv(to_recv)
        if not data:
          err(error_msg)
          return None
        return data
      except:
        err(error_msg)
        return None

    def read_until_char(c, buffer, error_msg):
      """ read until 'c' is found; buffer rest """
      while not '\n' in buffer:
        data = uread(1024, error_msg)
        if data == None:
          err(error_msg)
          return ('', '', '')
        buffer += data
      return buffer.partition(c)

    def read_exact(total_to_recv, buffer, error_msg):
      """ read exact number of 'total_to_recv' bytes """
      while len(buffer) < total_to_recv:
        to_recv = min(total_to_recv - len(buffer), 1024)
        data = uread(to_recv, error_msg)
        if data == None:
          return None
        buffer += data
      return buffer

    prompt = self.base_prompt + self.prompt_sep
    buffer = ''
    
    
    
    
    
    try:
      data = self.verifySendCMD(['pull ' + remoteFile])
    except(DMError):
      return None

    
    metadata, sep, buffer = read_until_char('\n', buffer, 'could not find metadata')
    if not metadata:
      return None
    if self.debug >= 3:
      print 'metadata: %s' % metadata

    filename, sep, filesizestr = metadata.partition(',')
    if sep == '':
      err('could not find file size in returned metadata')
      return None
    try:
        filesize = int(filesizestr)
    except ValueError:
      err('invalid file size in returned metadata')
      return None

    if filesize == -1:
      
      error_str, sep, buffer = read_until_char('\n', buffer, 'could not find error message')
      if not error_str:
        return None
      
      read_exact(len(prompt), buffer, 'could not find prompt')
      print 'DeviceManager: error pulling file: %s' % error_str
      return None

    
    total_to_recv = filesize + len(prompt)
    buffer = read_exact(total_to_recv, buffer, 'could not get all file data')
    if buffer == None:
      return None
    if buffer[-len(prompt):] != prompt:
      err('no prompt found after file data--DeviceManager may be out of sync with agent')
      return buffer
    return buffer[:-len(prompt)]

  
  
  
  
  
  def getFile(self, remoteFile, localFile = ''):
    if localFile == '':
      localFile = os.path.join(self.tempRoot, "temp.txt")
  
    retVal = self.pullFile(remoteFile)
    if (retVal is None):
      return None

    fhandle = open(localFile, 'wb')
    fhandle.write(retVal)
    fhandle.close()
    if not self.validateFile(remoteFile, localFile):
      print 'failed to validate file when downloading %s!' % remoteFile
      return None
    return retVal

  
  
  
  
  
  
  
  
  def getDirectory(self, remoteDir, localDir, checkDir=True):
    if (self.debug >= 2): print "getting files in '" + remoteDir + "'"
    if checkDir:
      try:
        is_dir = self.isDir(remoteDir)
      except FileError:
        return None
      if not is_dir:
        return None
        
    filelist = self.listFiles(remoteDir)
    if (self.debug >= 3): print filelist
    if not os.path.exists(localDir):
      os.makedirs(localDir)

    for f in filelist:
      if f == '.' or f == '..':
        continue
      remotePath = remoteDir + '/' + f
      localPath = os.path.join(localDir, f)
      try:
        is_dir = self.isDir(remotePath)
      except FileError:
        print 'isdir failed on file "%s"; continuing anyway...' % remotePath
        continue
      if is_dir:
        if (self.getDirectory(remotePath, localPath, False) == None):
          print 'failed to get directory "%s"' % remotePath
          return None
      else:
        
        
        
        
        if self.getFile(remotePath, localPath) == None:
          print 'failed to get file "%s"; continuing anyway...' % remotePath 
    return filelist

  
  
  
  
  
  def isDir(self, remotePath):
    try:
      data = self.verifySendCMD(['isdir ' + remotePath])
    except(DMError):
      
      
      
      return False
    retVal = self.stripPrompt(data).strip()
    if not retVal:
      raise FileError('isdir returned null')
    return retVal == 'TRUE'

  
  
  
  
  
  def validateFile(self, remoteFile, localFile):
    remoteHash = self.getRemoteHash(remoteFile)
    localHash = self.getLocalHash(localFile)

    if (remoteHash == None):
      return False

    if (remoteHash == localHash):
      return True

    return False
  
  
  
  
  
  
  def getRemoteHash(self, filename):
    try:
      data = self.verifySendCMD(['hash ' + filename])
    except(DMError):
      return None

    retVal = self.stripPrompt(data)
    if (retVal != None):
      retVal = retVal.strip('\n')
    if (self.debug >= 3): print "remote hash returned: '" + retVal + "'"
    return retVal
    
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  def getDeviceRoot(self):
    try:
      data = self.verifySendCMD(['testroot'])
    except:
      return None
  
    deviceRoot = self.stripPrompt(data).strip('\n') + '/tests'

    if (not self.dirExists(deviceRoot)):
      if (self.mkDir(deviceRoot) == None):
        return None

    return deviceRoot

  
  
  
  
  def unpackFile(self, filename):
    devroot = self.getDeviceRoot()
    if (devroot == None):
      return None

    dir = ''
    parts = filename.split('/')
    if (len(parts) > 1):
      if self.fileExists(filename):
        dir = '/'.join(parts[:-1])
    elif self.fileExists('/' + filename):
      dir = '/' + filename
    elif self.fileExists(devroot + '/' + filename):
      dir = devroot + '/' + filename
    else:
      return None

    try:
      data = self.verifySendCMD(['cd ' + dir, 'unzp ' + filename])
    except(DMError):
      return None

    return data

  
  
  
  
  def reboot(self, ipAddr=None, port=30000):
    cmd = 'rebt'   

    if (self.debug > 3): print "INFO: sending rebt command"
    callbacksvrstatus = None    

    if (ipAddr is not None):
    
      try:
        destname = '/data/data/com.mozilla.SUTAgentAndroid/files/update.info'
        data = "%s,%s\rrebooting\r" % (ipAddr, port)
        self.verifySendCMD(['push ' + destname + ' ' + str(len(data)) + '\r\n', data], newline = False)
      except(DMError):
        return None

      ip, port = self.getCallbackIpAndPort(ipAddr, port)
      cmd += " %s %s" % (ip, port)
      
      callbacksvr = callbackServer(ip, port, self.debug)

    try:
      status = self.verifySendCMD([cmd])
    except(DMError):
      return None

    if (ipAddr is not None):
      status = callbacksvr.disconnect()

    if (self.debug > 3): print "INFO: rebt- got status back: " + str(status)
    return status

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  def getInfo(self, directive=None):
    data = None
    result = {}
    collapseSpaces = re.compile('  +')

    directives = ['os', 'id','uptime','systime','screen','memory','process',
                  'disk','power']
    if (directive in directives):
      directives = [directive]

    for d in directives:
      data = self.verifySendCMD(['info ' + d])
      if (data is None):
        continue
      data = self.stripPrompt(data)
      data = collapseSpaces.sub(' ', data)
      result[d] = data.split('\n')

    
    for k, v in result.iteritems():
      result[k] = filter(lambda x: x != '', result[k])
    
    
    if 'process' in result:
      proclist = []
      for l in result['process']:
        if l:
          proclist.append(l.split('\t'))
      result['process'] = proclist

    if (self.debug >= 3): print "results: " + str(result)
    return result

  """
  Installs the application onto the device
  Application bundle - path to the application bundle on the device
  Destination - destination directory of where application should be
                installed to (optional)
  Returns None for success, or output if known failure
  """
  
  
  
  
  def installApp(self, appBundlePath, destPath=None):
    cmd = 'inst ' + appBundlePath
    if destPath:
      cmd += ' ' + destPath
    try:
      data = self.verifySendCMD([cmd])
    except(DMError):
      return None

    f = re.compile('Failure')
    for line in data.split():
      if (f.match(line)):
        return data
    return None

  """
  Uninstalls the named application from device and causes a reboot.
  Takes an optional argument of installation path - the path to where the application
  was installed.
  Returns True, but it doesn't mean anything other than the command was sent,
  the reboot happens and we don't know if this succeeds or not.
  """
  
  
  
  
  def uninstallAppAndReboot(self, appName, installPath=None):
    cmd = 'uninst ' + appName
    if installPath:
      cmd += ' ' + installPath
    try:
      data = self.verifySendCMD([cmd])
    except(DMError):
      return None

    if (self.debug > 3): print "uninstallAppAndReboot: " + str(data)
    return True

  """
  Updates the application on the device.
  Application bundle - path to the application bundle on the device
  Process name of application - used to end the process if the applicaiton is
                                currently running
  Destination - Destination directory to where the application should be
                installed (optional)
  ipAddr - IP address to await a callback ping to let us know that the device has updated
           properly - defaults to current IP.
  port - port to await a callback ping to let us know that the device has updated properly
         defaults to 30000, and counts up from there if it finds a conflict
  Returns True if succeeds, False if not
  """
  
  
  
  
  def updateApp(self, appBundlePath, processName=None, destPath=None, ipAddr=None, port=30000):
    status = None
    cmd = 'updt '
    if (processName == None):
      
      cmd += "'' " + appBundlePath
    else:
      cmd += processName + ' ' + appBundlePath

    if (destPath):
      cmd += " " + destPath

    if (ipAddr is not None):
      ip, port = self.getCallbackIpAndPort(ipAddr, port)
      cmd += " %s %s" % (ip, port)
      
      callbacksvr = callbackServer(ip, port, self.debug)

    if (self.debug >= 3): print "INFO: updateApp using command: " + str(cmd)

    try:
      status = self.verifySendCMD([cmd])
    except(DMError):
      return None

    if ipAddr is not None:
      status = callbacksvr.disconnect()

    if (self.debug >= 3): print "INFO: updateApp: got status back: " + str(status)

    return status

  """
    return the current time on the device
  """
  
  
  
  
  def getCurrentTime(self):
    try:
      data = self.verifySendCMD(['clok'])
    except(DMError):
      return None

    return self.stripPrompt(data).strip('\n')

  """
    Connect the ipaddress and port for a callback ping.  Defaults to current IP address
    And ports starting at 30000.
    NOTE: the detection for current IP address only works on Linux!
  """
  
  
  
  
  def unpackFile(self, filename):
    devroot = self.getDeviceRoot()
    if (devroot == None):
      return None

    dir = ''
    parts = filename.split('/')
    if (len(parts) > 1):
      if self.fileExists(filename):
        dir = '/'.join(parts[:-1])
    elif self.fileExists('/' + filename):
      dir = '/' + filename
    elif self.fileExists(devroot + '/' + filename):
      dir = devroot + '/' + filename
    else:
      return None

    try:
      data = self.verifySendCMD(['cd ' + dir, 'unzp ' + filename])
    except(DMError):
      return None

    return data

  def getCallbackIpAndPort(self, aIp, aPort):
    ip = aIp
    nettools = NetworkTools()
    if (ip == None):
      ip = nettools.getLanIp()
    if (aPort != None):
      port = nettools.findOpenPort(ip, aPort)
    else:
      port = nettools.findOpenPort(ip, 30000)
    return ip, port

  """
    Returns a properly formatted env string for the agent.
    Input - env, which is either None, '', or a dict
    Output - a quoted string of the form: '"envvar1=val1,envvar2=val2..."'
    If env is None or '' return '' (empty quoted string)
  """
  def formatEnvString(self, env):
    if (env == None or env == ''):
      return ''

    retVal = '"%s"' % ','.join(map(lambda x: '%s=%s' % (x[0], x[1]), env.iteritems()))
    if (retVal == '""'):
      return ''

    return retVal

  """
    adjust the screen resolution on the device, REBOOT REQUIRED
    NOTE: this only works on a tegra ATM
    success: True
    failure: False

    supported resolutions: 640x480, 800x600, 1024x768, 1152x864, 1200x1024, 1440x900, 1680x1050, 1920x1080
  """
  def adjustResolution(self, width=1680, height=1050, type='hdmi'):
    if self.getInfo('os')['os'][0].split()[0] != 'harmony-eng':
      if (self.debug >= 2): print "WARNING: unable to adjust screen resolution on non Tegra device"
      return False

    results = self.getInfo('screen')
    parts = results['screen'][0].split(':')
    if (self.debug >= 3): print "INFO: we have a current resolution of %s, %s" % (parts[1].split()[0], parts[2].split()[0])

    
    screentype = -1
    if (type == 'hdmi'):
      screentype = 5
    elif (type == 'vga' or type == 'crt'):
      screentype = 3
    else:
      return False

    
    if not (isinstance(width, int) and isinstance(height, int)):
      return False

    if (width < 100 or width > 9999):
      return False

    if (height < 100 or height > 9999):
      return False

    if (self.debug >= 3): print "INFO: adjusting screen resolution to %s, %s and rebooting" % (width, height)
    try:
      self.verifySendCMD(["exec setprop persist.tegra.dpy%s.mode.width %s" % (screentype, width)])
      self.verifySendCMD(["exec setprop persist.tegra.dpy%s.mode.height %s" % (screentype, height)])
    except(DMError):
      return False

    return True

gCallbackData = ''

class myServer(SocketServer.TCPServer):
  allow_reuse_address = True

class callbackServer():
  def __init__(self, ip, port, debuglevel):
    global gCallbackData
    if (debuglevel >= 1): print "DEBUG: gCallbackData is: %s on port: %s" % (gCallbackData, port)
    gCallbackData = ''
    self.ip = ip
    self.port = port
    self.connected = False
    self.debug = debuglevel
    if (self.debug >= 3): print "Creating server with " + str(ip) + ":" + str(port)
    self.server = myServer((ip, port), self.myhandler)
    self.server_thread = Thread(target=self.server.serve_forever) 
    self.server_thread.setDaemon(True)
    self.server_thread.start()

  def disconnect(self, step = 60, timeout = 600):
    t = 0
    if (self.debug >= 3): print "Calling disconnect on callback server"
    while t < timeout:
      if (gCallbackData):
        
        if (self.debug >= 3): print "Got data back from agent: " + str(gCallbackData)
        break
      else:
        if (self.debug >= 0): print '.',
      time.sleep(step)
      t += step

    try:
      if (self.debug >= 3): print "Shutting down server now"
      self.server.shutdown()
    except:
      if (self.debug >= 1): print "Unable to shutdown callback server - check for a connection on port: " + str(self.port)

    
    time.sleep(step)
    return gCallbackData

  class myhandler(SocketServer.BaseRequestHandler):
    def handle(self):
      global gCallbackData
      gCallbackData = self.request.recv(1024)
      
      self.request.send("OK")
  

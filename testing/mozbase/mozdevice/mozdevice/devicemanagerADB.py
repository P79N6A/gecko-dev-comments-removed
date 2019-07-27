



import re
import os
import shutil
import tempfile
import time
import traceback

from devicemanager import DeviceManager, DMError
from mozprocess import ProcessHandler
import mozfile
import mozlog


class DeviceManagerADB(DeviceManager):
    """
    Implementation of DeviceManager interface that uses the Android "adb"
    utility to communicate with the device. Normally used to communicate
    with a device that is directly connected with the host machine over a USB
    port.
    """

    _haveRootShell = False
    _haveSu = False
    _useZip = False
    _logcatNeedsRoot = False
    _pollingInterval = 0.01
    _packageName = None
    _tempDir = None
    connected = False
    default_timeout = 300

    def __init__(self, host=None, port=5555, retryLimit=5, packageName='fennec',
                 adbPath='adb', deviceSerial=None, deviceRoot=None,
                 logLevel=mozlog.ERROR, autoconnect=True, runAdbAsRoot=False, **kwargs):
        DeviceManager.__init__(self, logLevel=logLevel,
                               deviceRoot=deviceRoot)
        self.host = host
        self.port = port
        self.retryLimit = retryLimit

        
        self._adbPath = adbPath

        
        
        self._deviceSerial = deviceSerial

        
        
        self._runAdbAsRoot = runAdbAsRoot

        if packageName == 'fennec':
            if os.getenv('USER'):
                self._packageName = 'org.mozilla.fennec_' + os.getenv('USER')
            else:
                self._packageName = 'org.mozilla.fennec_'
        elif packageName:
            self._packageName = packageName

        
        self._verifyADB()

        if autoconnect:
            self.connect()

    def connect(self):
        if not self.connected:
            
            if self.host:
                self._connectRemoteADB()

            
            self._verifyDevice()

            
            
            
            
            self._checkForRoot()

            
            
            try:
                self._verifyZip()
            except DMError:
                pass

    def __del__(self):
        if self.host:
            self._disconnectRemoteADB()

    def shell(self, cmd, outputfile, env=None, cwd=None, timeout=None, root=False):
        
        

        
        if root and not self._haveRootShell and not self._haveSu:
            raise DMError("Shell command '%s' requested to run as root but root "
                          "is not available on this device. Root your device or "
                          "refactor the test/harness to not require root." %
                          self._escapedCommandLine(cmd))

        
        
        
        if root and not self._haveRootShell:
            cmdline = "su -c \"%s\"" % self._escapedCommandLine(cmd)
        else:
            cmdline = self._escapedCommandLine(cmd)
        cmdline += "; echo $?"

        
        if cwd:
            cmdline = "cd %s; %s" % (cwd, cmdline)
        if env:
            envstr = '; '.join(map(lambda x: 'export %s=%s' % (x[0], x[1]), env.iteritems()))
            cmdline = envstr + "; " + cmdline

        
        args=[self._adbPath]
        if self._deviceSerial:
            args.extend(['-s', self._deviceSerial])
        args.extend(["shell", cmdline])

        def _raise():
            raise DMError("Timeout exceeded for shell call")

        self._logger.debug("shell - command: %s" % ' '.join(args))
        proc = ProcessHandler(args, processOutputLine=self._log, onTimeout=_raise)

        if not timeout:
            
            timeout = self.default_timeout

        timeout = int(timeout)
        proc.run(timeout)
        proc.wait()
        output = proc.output

        if output:
            lastline = output[-1]
            if lastline:
                m = re.search('([0-9]+)', lastline)
                if m:
                    return_code = m.group(1)
                    for line in output:
                        outputfile.write(line + '\n')
                    outputfile.seek(-2, 2)
                    outputfile.truncate() 
                    return int(return_code)

        return None

    def forward(self, local, remote):
        """
        Forward socket connections.

        Forward specs are one of:
          tcp:<port>
          localabstract:<unix domain socket name>
          localreserved:<unix domain socket name>
          localfilesystem:<unix domain socket name>
          dev:<character device name>
          jdwp:<process pid> (remote only)
        """
        return self._checkCmd(['forward', local, remote])

    def remount(self):
        "Remounts the /system partition on the device read-write."
        return self._checkCmd(['remount'])

    def devices(self):
        "Return a list of connected devices as (serial, status) tuples."
        proc = self._runCmd(['devices'])
        proc.output.pop(0) 
        devices = []
        for line in proc.output:
            result = re.match('(.*?)\t(.*)', line)
            if result:
                devices.append((result.group(1), result.group(2)))
        return devices

    def _connectRemoteADB(self):
        self._checkCmd(["connect", self.host + ":" + str(self.port)])

    def _disconnectRemoteADB(self):
        self._checkCmd(["disconnect", self.host + ":" + str(self.port)])

    def pushFile(self, localname, destname, retryLimit=None, createDir=True):
        
        
        
        
        retryLimit = retryLimit or self.retryLimit
        if self.dirExists(destname):
            raise DMError("Attempted to push a file (%s) to a directory (%s)!" %
                          (localname, destname))
        if not os.access(localname, os.F_OK):
            raise DMError("File not found: %s" % localname)

        proc = self._runCmd(["push", os.path.realpath(localname), destname],
                retryLimit=retryLimit)
        if proc.returncode != 0:
            raise DMError("Error pushing file %s -> %s; output: %s" % (localname, destname, proc.output))

    def mkDir(self, name):
        result = self._runCmd(["shell", "mkdir", name]).output
        if len(result) and 'read-only file system' in result[0].lower():
            raise DMError("Error creating directory: read only file system")

    def pushDir(self, localDir, remoteDir, retryLimit=None, timeout=None):
        
        
        
        
        retryLimit = retryLimit or self.retryLimit
        if not self.dirExists(remoteDir):
            self.mkDirs(remoteDir+"/x")
        if self._useZip:
            try:
                localZip = tempfile.mktemp() + ".zip"
                remoteZip = remoteDir + "/adbdmtmp.zip"
                proc = ProcessHandler(["zip", "-r", localZip, '.'], cwd=localDir,
                              processOutputLine=self._log)
                proc.run()
                proc.wait()
                self.pushFile(localZip, remoteZip, retryLimit=retryLimit, createDir=False)
                mozfile.remove(localZip)
                data = self._runCmd(["shell", "unzip", "-o", remoteZip,
                                     "-d", remoteDir]).output[0]
                self._checkCmd(["shell", "rm", remoteZip],
                               retryLimit=retryLimit, timeout=timeout)
                if re.search("unzip: exiting", data) or re.search("Operation not permitted", data):
                    raise Exception("unzip failed, or permissions error")
            except:
                self._logger.warning(traceback.format_exc())
                self._logger.warning("zip/unzip failure: falling back to normal push")
                self._useZip = False
                self.pushDir(localDir, remoteDir, retryLimit=retryLimit)
        else:
            tmpDir = tempfile.mkdtemp()
            
            tmpDirTarget = os.path.join(tmpDir, "tmp")
            shutil.copytree(localDir, tmpDirTarget)
            self._checkCmd(["push", tmpDirTarget, remoteDir], retryLimit=retryLimit)
            mozfile.remove(tmpDir)

    def dirExists(self, remotePath):
        data = self._runCmd(["shell", "ls", "-a", remotePath + '/']).output

        if len(data) == 1:
            res = data[0]
            if "Not a directory" in res or "No such file or directory" in res:
                return False
        return True

    def fileExists(self, filepath):
        data = self._runCmd(["shell", "ls", "-a", filepath]).output
        if len(data) == 1:
            foundpath = data[0].decode('utf-8').rstrip()
            if foundpath == filepath:
                return True
        return False

    def removeFile(self, filename):
        if self.fileExists(filename):
            self._checkCmd(["shell", "rm", filename])

    def removeDir(self, remoteDir):
        if self.dirExists(remoteDir):
            self._checkCmd(["shell", "rm", "-r", remoteDir])
        else:
            self.removeFile(remoteDir.strip())

    def moveTree(self, source, destination):
        self._checkCmd(["shell", "mv", source, destination])

    def copyTree(self, source, destination):
        self._checkCmd(["shell", "dd", "if=%s" % source, "of=%s" % destination])

    def listFiles(self, rootdir):
        data = self._runCmd(["shell", "ls", "-a", rootdir]).output
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
        p = self._runCmd(["shell", "ps"])
        
        p.output.pop(0)
        ret = []
        for proc in p.output:
            els = proc.split()
            
            
            if els[1].isdigit():
                ret.append(list([int(els[1]), els[len(els) - 1], els[0]]))
            else:
                ret.append(list([int(els[0]), els[len(els) - 1], els[1]]))
        return ret

    def fireProcess(self, appname, failIfRunning=False):
        """
        Starts a process

        returns: pid

        DEPRECATED: Use shell() or launchApplication() for new code
        """
        
        parts = appname.split('"');
        if (len(parts) > 2):
            parts = parts[2:]
        return self.launchProcess(parts, failIfRunning)

    def launchProcess(self, cmd, outputFile = "process.txt", cwd = '', env = '', failIfRunning=False):
        """
        Launches a process, redirecting output to standard out

        WARNING: Does not work how you expect on Android! The application's
        own output will be flushed elsewhere.

        DEPRECATED: Use shell() or launchApplication() for new code
        """
        if cmd[0] == "am":
            self._checkCmd(["shell"] + cmd)
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
        self._logger.info(acmd)
        self._checkCmd(acmd)
        return outputFile

    def killProcess(self, appname, sig=None):
        procs = self.getProcessList()
        for (pid, name, user) in procs:
            if name == appname:
                args = ["shell", "kill"]
                if sig:
                    args.append("-%d" % sig)
                args.append(str(pid))
                p = self._runCmd(args)
                if p.returncode != 0:
                    raise DMError("Error killing process "
                                  "'%s': %s" % (appname, p.output))

    def _runPull(self, remoteFile, localFile):
        """
        Pulls remoteFile from device to host
        """
        try:
            self._runCmd(["pull",  remoteFile, localFile])
        except (OSError, ValueError):
            raise DMError("Error pulling remote file '%s' to '%s'" % (remoteFile, localFile))

    def pullFile(self, remoteFile, offset=None, length=None):
        
        localFile = tempfile.mkstemp()[1]
        self._runPull(remoteFile, localFile)

        f = open(localFile, 'r')

        
        
        if offset is not None and length is not None:
            f.seek(offset)
            ret = f.read(length)
        elif offset is not None:
            f.seek(offset)
            ret = f.read()
        else:
            ret = f.read()

        f.close()
        mozfile.remove(localFile)
        return ret

    def getFile(self, remoteFile, localFile):
        self._runPull(remoteFile, localFile)

    def getDirectory(self, remoteDir, localDir, checkDir=True):
        self._runCmd(["pull", remoteDir, localDir]).wait()

    def validateFile(self, remoteFile, localFile):
        md5Remote = self._getRemoteHash(remoteFile)
        md5Local = self._getLocalHash(localFile)
        if md5Remote is None or md5Local is None:
            return None
        return md5Remote == md5Local

    def _getRemoteHash(self, remoteFile):
        """
        Return the md5 sum of a file on the device
        """
        localFile = tempfile.mkstemp()[1]
        localFile = self._runPull(remoteFile, localFile)

        if localFile is None:
            return None

        md5 = self._getLocalHash(localFile)
        mozfile.remove(localFile)

        return md5

    def _setupDeviceRoot(self, deviceRoot):
        
        if deviceRoot:
            self.mkDir(deviceRoot)
            return deviceRoot

        
        paths = [('/storage/sdcard0', 'tests'),
                 ('/storage/sdcard1', 'tests'),
                 ('/sdcard', 'tests'),
                 ('/mnt/sdcard', 'tests'),
                 ('/data/local', 'tests')]
        for (basePath, subPath) in paths:
            if self.dirExists(basePath):
                root = os.path.join(basePath, subPath)
                try:
                    self.mkDir(root)
                    return root
                except:
                    pass

        raise DMError("Unable to set up device root using paths: [%s]"
                        % ", ".join(["'%s'" % os.path.join(b, s) for b, s in paths]))

    def getTempDir(self):
        
        
        if not self._tempDir:
            self._tempDir = "%s/tmp" % self.deviceRoot
            self.mkDir(self._tempDir)

        return self._tempDir

    def reboot(self, wait = False, **kwargs):
        self._checkCmd(["reboot"])
        if wait:
            self._checkCmd(["wait-for-device"])
            if self._runAdbAsRoot:
                self._adb_root()
            self._checkCmd(["shell", "ls", "/sbin"])

    def updateApp(self, appBundlePath, **kwargs):
        return self._runCmd(["install", "-r", appBundlePath]).output

    def getCurrentTime(self):
        timestr = str(self._runCmd(["shell", "date", "+%s"]).output[0])
        if (not timestr or not timestr.isdigit()):
            raise DMError("Unable to get current time using date (got: '%s')" % timestr)
        return int(timestr)*1000

    def getInfo(self, directive=None):
        ret = {}
        if (directive == "id" or directive == "all"):
            ret["id"] = self._runCmd(["get-serialno"]).output[0]
        if (directive == "os" or directive == "all"):
            ret["os"] = self._runCmd(["shell", "getprop", "ro.build.display.id"]).output[0]
        if (directive == "uptime" or directive == "all"):
            utime = self._runCmd(["shell", "uptime"]).output[0]
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
            ret["process"] = self._runCmd(["shell", "ps"]).output
        if (directive == "systime" or directive == "all"):
            ret["systime"] = self._runCmd(["shell", "date"]).output[0]
        self._logger.info(ret)
        return ret

    def uninstallApp(self, appName, installPath=None):
        status = self._runCmd(["uninstall", appName]).output[0].strip()
        if status != 'Success':
            raise DMError("uninstall failed for %s. Got: %s" % (appName, status))

    def uninstallAppAndReboot(self, appName, installPath=None):
        self.uninstallApp(appName)
        self.reboot()

    def _runCmd(self, args, retryLimit=None):
        """
        Runs a command using adb

        returns: instance of ProcessHandler
        """
        retryLimit = retryLimit or self.retryLimit
        finalArgs = [self._adbPath]
        if self._deviceSerial:
            finalArgs.extend(['-s', self._deviceSerial])
        finalArgs.extend(args)
        self._logger.debug("_runCmd - command: %s" % ' '.join(finalArgs))
        retries = 0
        while retries < retryLimit:
            proc = ProcessHandler(finalArgs, storeOutput=True,
                    processOutputLine=self._log)
            proc.run()
            proc.returncode = proc.wait()
            if proc.returncode == None:
                proc.kill()
                retries += 1
            else:
                return proc

    
    
    def _checkCmd(self, args, timeout=None, retryLimit=None):
        """
        Runs a command using adb and waits for the command to finish.
        If timeout is specified, the process is killed after <timeout> seconds.

        returns: returncode from process
        """
        retryLimit = retryLimit or self.retryLimit
        finalArgs = [self._adbPath]
        if self._deviceSerial:
            finalArgs.extend(['-s', self._deviceSerial])
        finalArgs.extend(args)
        self._logger.debug("_checkCmd - command: %s" % ' '.join(finalArgs))
        if not timeout:
            
            
            timeout = self.default_timeout

        timeout = int(timeout)
        retries = 0
        while retries < retryLimit:
            proc = ProcessHandler(finalArgs, processOutputLine=self._log)
            proc.run(timeout=timeout)
            ret_code = proc.wait()
            if ret_code == None:
                proc.kill()
                retries += 1
            else:
                return ret_code

        raise DMError("Timeout exceeded for _checkCmd call after %d retries." % retries)

    def chmodDir(self, remoteDir, mask="777"):
        if (self.dirExists(remoteDir)):
            files = self.listFiles(remoteDir.strip())
            for f in files:
                remoteEntry = remoteDir.strip() + "/" + f.strip()
                if (self.dirExists(remoteEntry)):
                    self.chmodDir(remoteEntry)
                else:
                    self._checkCmd(["shell", "chmod", mask, remoteEntry])
                    self._logger.info("chmod %s" % remoteEntry)
            self._checkCmd(["shell", "chmod", mask, remoteDir])
            self._logger.info("chmod %s" % remoteDir)
        else:
            self._checkCmd(["shell", "chmod", mask, remoteDir.strip()])
            self._logger.info("chmod %s" % remoteDir.strip())

    def _verifyADB(self):
        """
        Check to see if adb itself can be executed.
        """
        if self._adbPath != 'adb':
            if not os.access(self._adbPath, os.X_OK):
                raise DMError("invalid adb path, or adb not executable: %s" % self._adbPath)

        try:
            self._checkCmd(["version"])
        except os.error, err:
            raise DMError("unable to execute ADB (%s): ensure Android SDK is installed and adb is in your $PATH" % err)

    def _verifyDevice(self):
        
        if self._deviceSerial:
            deviceStatus = None
            for line in self._runCmd(["devices"]).output:
                m = re.match('(.+)?\s+(.+)$', line)
                if m:
                    if self._deviceSerial == m.group(1):
                        deviceStatus = m.group(2)
            if deviceStatus == None:
                raise DMError("device not found: %s" % self._deviceSerial)
            elif deviceStatus != "device":
                raise DMError("bad status for device %s: %s" % (self._deviceSerial, deviceStatus))

        
        if self._checkCmd(["shell", "echo"]) is None:
            raise DMError("unable to connect to device")

    def _checkForRoot(self):
        
        
        
        data = self._runCmd(["shell", "id"]).output[0]
        if data.find('uid=0(root)') >= 0:
            self._haveRootShell = True
            
            return

        
        
        proc = self._runCmd(["shell", "su", "-c", "id"])

        
        
        start_time = time.time()
        retcode = None
        while (time.time() - start_time) <= 15 and retcode is None:
            retcode = proc.poll()
        if retcode is None: 
            proc.kill()

        data = proc.output[0]
        if data.find('uid=0(root)') >= 0:
            self._haveSu = True

        if self._runAdbAsRoot:
            self._adb_root()

    def _isUnzipAvailable(self):
        data = self._runCmd(["shell", "unzip"]).output
        for line in data:
            if (re.search('Usage', line)):
                return True
        return False

    def _isLocalZipAvailable(self):
        try:
            self._checkCmd(["zip", "-?"])
        except:
            return False
        return True

    def _verifyZip(self):
        
        
        
        self._useZip = False
        if (self._isUnzipAvailable() and self._isLocalZipAvailable()):
            self._logger.info("will use zip to push directories")
            self._useZip = True
        else:
            raise DMError("zip not available")

    def _adb_root(self):
        """ Some devices require us to reboot adbd as root.
            This function takes care of it.
        """
        if self.processInfo("adbd")[2] != "root":
            self._checkCmd(["root"])
            self._checkCmd(["wait-for-device"])
            if self.processInfo("adbd")[2] != "root":
                raise DMError("We tried rebooting adbd as root, however, it failed.")

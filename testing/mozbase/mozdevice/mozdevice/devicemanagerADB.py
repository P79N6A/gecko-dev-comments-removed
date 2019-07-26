



import subprocess
from devicemanager import DeviceManager, DMError, _pop_last_line
import re
import os
import shutil
import tempfile
import time

class DeviceManagerADB(DeviceManager):
    """
    Implementation of DeviceManager interface that uses the Android "adb"
    utility to communicate with the device. Normally used to communicate
    with a device that is directly connected with the host machine over a USB
    port.
    """

    _haveRootShell = False
    _haveSu = False
    _useRunAs = False
    _useZip = False
    _logcatNeedsRoot = False
    _pollingInterval = 0.01
    _packageName = None
    _tempDir = None
    default_timeout = 300

    def __init__(self, host=None, port=5555, retryLimit=5, packageName='fennec',
                 adbPath='adb', deviceSerial=None, deviceRoot=None, **kwargs):
        self.host = host
        self.port = port
        self.retryLimit = retryLimit
        self.deviceRoot = deviceRoot

        
        self._adbPath = adbPath

        
        
        self._deviceSerial = deviceSerial

        if packageName == 'fennec':
            if os.getenv('USER'):
                self._packageName = 'org.mozilla.fennec_' + os.getenv('USER')
            else:
                self._packageName = 'org.mozilla.fennec_'
        elif packageName:
            self._packageName = packageName

        
        self._verifyADB()

        
        if self.host:
            self._connectRemoteADB()

        
        self._verifyDevice()

        
        self._setupDeviceRoot()

        
        
        
        
        self._checkForRoot()

        
        try:
            self._verifyRunAs()
        except DMError:
            pass

        
        
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

        procOut = tempfile.SpooledTemporaryFile()
        procErr = tempfile.SpooledTemporaryFile()
        proc = subprocess.Popen(args, stdout=procOut, stderr=procErr)

        if not timeout:
            
            timeout = self.default_timeout

        timeout = int(timeout)
        start_time = time.time()
        ret_code = proc.poll()
        while ((time.time() - start_time) <= timeout) and ret_code == None:
            time.sleep(self._pollingInterval)
            ret_code = proc.poll()
        if ret_code == None:
            proc.kill()
            raise DMError("Timeout exceeded for shell call")

        procOut.seek(0)
        outputfile.write(procOut.read().rstrip('\n'))
        procOut.close()
        procErr.close()

        lastline = _pop_last_line(outputfile)
        if lastline:
            m = re.search('([0-9]+)', lastline)
            if m:
                return_code = m.group(1)
                outputfile.seek(-2, 2)
                outputfile.truncate() 
                return int(return_code)

        return None

    def _connectRemoteADB(self):
        self._checkCmd(["connect", self.host + ":" + str(self.port)])

    def _disconnectRemoteADB(self):
        self._checkCmd(["disconnect", self.host + ":" + str(self.port)])

    def pushFile(self, localname, destname, retryLimit=None):
        
        
        
        
        retryLimit = retryLimit or self.retryLimit
        if self.dirExists(destname):
            raise DMError("Attempted to push a file (%s) to a directory (%s)!" %
                          (localname, destname))

        if self._useRunAs:
            remoteTmpFile = self.getTempDir() + "/" + os.path.basename(localname)
            self._checkCmd(["push", os.path.realpath(localname), remoteTmpFile],
                    retryLimit=retryLimit)
            self.shellCheckOutput(["dd", "if=" + remoteTmpFile, "of=" + destname])
            self.shellCheckOutput(["rm", remoteTmpFile])
        else:
            self._checkCmd(["push", os.path.realpath(localname), destname],
                    retryLimit=retryLimit)

    def mkDir(self, name):
        result = self._runCmdAs(["shell", "mkdir", name]).stdout.read()
        if 'read-only file system' in result.lower():
            raise DMError("Error creating directory: read only file system")

    def pushDir(self, localDir, remoteDir, retryLimit=None):
        
        
        
        
        retryLimit = retryLimit or self.retryLimit
        if not self.dirExists(remoteDir):
            self.mkDirs(remoteDir+"/x")
        if self._useZip:
            try:
                localZip = tempfile.mktemp() + ".zip"
                remoteZip = remoteDir + "/adbdmtmp.zip"
                subprocess.Popen(["zip", "-r", localZip, '.'], cwd=localDir,
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
                self.pushFile(localZip, remoteZip, retryLimit=retryLimit)
                os.remove(localZip)
                data = self._runCmdAs(["shell", "unzip", "-o", remoteZip,
                                       "-d", remoteDir]).stdout.read()
                self._checkCmdAs(["shell", "rm", remoteZip], retryLimit=retryLimit)
                if re.search("unzip: exiting", data) or re.search("Operation not permitted", data):
                    raise Exception("unzip failed, or permissions error")
            except:
                print "zip/unzip failure: falling back to normal push"
                self._useZip = False
                self.pushDir(localDir, remoteDir, retryLimit=retryLimit)
        else:
            tmpDir = tempfile.mkdtemp()
            
            tmpDirTarget = os.path.join(tmpDir, "tmp")
            shutil.copytree(localDir, tmpDirTarget)
            self._checkCmd(["push", tmpDirTarget, remoteDir], retryLimit=retryLimit)
            shutil.rmtree(tmpDir)

    def dirExists(self, remotePath):
        p = self._runCmd(["shell", "ls", "-a", remotePath + '/'])

        data = p.stdout.readlines()
        if len(data) == 1:
            res = data[0]
            if "Not a directory" in res or "No such file or directory" in res:
                return False
        return True

    def fileExists(self, filepath):
        p = self._runCmd(["shell", "ls", "-a", filepath])
        data = p.stdout.readlines()
        if (len(data) == 1):
            if (data[0].rstrip() == filepath):
                return True
        return False

    def removeFile(self, filename):
        if self.fileExists(filename):
            self._runCmd(["shell", "rm", filename])

    def removeDir(self, remoteDir):
        if (self.dirExists(remoteDir)):
            self._runCmd(["shell", "rm", "-r", remoteDir]).wait()
        else:
            self.removeFile(remoteDir.strip())

    def listFiles(self, rootdir):
        p = self._runCmd(["shell", "ls", "-a", rootdir])
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
        p = self._runCmd(["shell", "ps"])
            
        p.stdout.readline()
        proc = p.stdout.readline()
        ret = []
        while (proc):
            els = proc.split()
            
            if els[1].isdigit():
                ret.append(list([int(els[1]), els[len(els) - 1], els[0]]))
            else:
                ret.append(list([int(els[0]), els[len(els) - 1], els[1]]))
            proc =  p.stdout.readline()
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
        print acmd
        self._checkCmd(acmd)
        return outputFile

    def killProcess(self, appname, forceKill=False):
        procs = self.getProcessList()
        for (pid, name, user) in procs:
            if name == appname:
                args = ["shell", "kill"]
                if forceKill:
                    args.append("-9")
                args.append(str(pid))
                p = self._runCmdAs(args)
                p.communicate()
                if p.returncode != 0:
                    raise DMError("Error killing process "
                                  "'%s': %s" % (appname, p.stdout.read()))

    def _runPull(self, remoteFile, localFile):
        """
        Pulls remoteFile from device to host
        """
        try:
            
            outerr = self._runCmd(["pull",  remoteFile, localFile]).communicate()

            
            if outerr[1]:
                errl = outerr[1].splitlines()
                if (len(errl) == 1):
                    if (((errl[0].find("Permission denied") != -1)
                        or (errl[0].find("does not exist") != -1))
                        and self._useRunAs):
                        
                        
                        
                        remoteTmpFile = self.getTempDir() + "/" + os.path.basename(remoteFile)
                        self._checkCmdAs(["shell", "dd", "if=" + remoteFile, "of=" + remoteTmpFile])
                        self._checkCmdAs(["shell", "chmod", "777", remoteTmpFile])
                        self._runCmd(["pull",  remoteTmpFile, localFile]).stdout.read()
                        
                        self._checkCmdAs(["shell", "rm", remoteTmpFile])
        except (OSError, ValueError):
            raise DMError("Error pulling remote file '%s' to '%s'" % (remoteFile, localFile))

    def pullFile(self, remoteFile):
        
        localFile = tempfile.mkstemp()[1]
        self._runPull(remoteFile, localFile)

        f = open(localFile, 'r')
        ret = f.read()
        f.close()
        os.remove(localFile)
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
        os.remove(localFile)

        return md5

    def _setupDeviceRoot(self):
        """
        setup the device root and cache its value
        """
        
        if self.deviceRoot:
            if not self.dirExists(self.deviceRoot):
                try:
                    self.mkDir(self.deviceRoot)
                except:
                    print "Unable to create device root %s" % self.deviceRoot
                    raise
            return

        paths = [('/mnt/sdcard', 'tests'),
                 ('/data/local', 'tests')]
        for (basePath, subPath) in paths:
            if self.dirExists(basePath):
                testRoot = os.path.join(basePath, subPath)
                try:
                    self.mkDir(testRoot)
                    self.deviceRoot = testRoot
                    return
                except:
                    pass

        raise DMError("Unable to set up device root using paths: [%s]"
                        % ", ".join(["'%s'" % os.path.join(b, s) for b, s in paths]))

    def getDeviceRoot(self):
        return self.deviceRoot

    def getTempDir(self):
        
        
        if not self._tempDir:
            self._tempDir = self.getDeviceRoot() + "/tmp"
            self.mkDir(self._tempDir)

        return self._tempDir

    def getAppRoot(self, packageName):
        devroot = self.getDeviceRoot()
        if (devroot == None):
            return None

        if (packageName and self.dirExists('/data/data/' + packageName)):
            self._packageName = packageName
            return '/data/data/' + packageName
        elif (self._packageName and self.dirExists('/data/data/' + self._packageName)):
            return '/data/data/' + self._packageName

        
        raise DMError("Failed to get application root for: %s" % packageName)

    def reboot(self, wait = False, **kwargs):
        self._runCmd(["reboot"])
        if (not wait):
            return
        countdown = 40
        while (countdown > 0):
            self._checkCmd(["wait-for-device", "shell", "ls", "/sbin"])

    def updateApp(self, appBundlePath, **kwargs):
        return self._runCmd(["install", "-r", appBundlePath]).stdout.read()

    def getCurrentTime(self):
        timestr = self._runCmd(["shell", "date", "+%s"]).stdout.read().strip()
        if (not timestr or not timestr.isdigit()):
            raise DMError("Unable to get current time using date (got: '%s')" % timestr)
        return str(int(timestr)*1000)

    def getInfo(self, directive=None):
        ret = {}
        if (directive == "id" or directive == "all"):
            ret["id"] = self._runCmd(["get-serialno"]).stdout.read()
        if (directive == "os" or directive == "all"):
            ret["os"] = self._runCmd(["shell", "getprop", "ro.build.display.id"]).stdout.read()
        if (directive == "uptime" or directive == "all"):
            utime = self._runCmd(["shell", "uptime"]).stdout.read()
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
            ret["process"] = self._runCmd(["shell", "ps"]).stdout.read()
        if (directive == "systime" or directive == "all"):
            ret["systime"] = self._runCmd(["shell", "date"]).stdout.read()
        print ret
        return ret

    def uninstallApp(self, appName, installPath=None):
        data = self._runCmd(["uninstall", appName]).stdout.read().strip()
        status = data.split('\n')[0].strip()
        if status != 'Success':
            raise DMError("uninstall failed for %s. Got: %s" % (appName, status))

    def uninstallAppAndReboot(self, appName, installPath=None):
        self.uninstallApp(appName)
        self.reboot()
        return

    def _runCmd(self, args):
        """
        Runs a command using adb

        returns: returncode from subprocess.Popen
        """
        finalArgs = [self._adbPath]
        if self._deviceSerial:
            finalArgs.extend(['-s', self._deviceSerial])
        
        
        if not self._haveRootShell and self._useRunAs and \
                args[0] == "shell" and args[1] not in [ "run-as", "am" ]:
            args.insert(1, "run-as")
            args.insert(2, self._packageName)
        finalArgs.extend(args)
        return subprocess.Popen(finalArgs, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    def _runCmdAs(self, args):
        """
        Runs a command using adb
        If self._useRunAs is True, the command is run-as user specified in self._packageName

        returns: returncode from subprocess.Popen
        """
        if self._useRunAs:
            args.insert(1, "run-as")
            args.insert(2, self._packageName)
        return self._runCmd(args)

    
    
    def _checkCmd(self, args, timeout=None, retryLimit=None):
        """
        Runs a command using adb and waits for the command to finish.
        If timeout is specified, the process is killed after <timeout> seconds.

        returns: returncode from subprocess.Popen
        """
        retryLimit = retryLimit or self.retryLimit
        
        
        finalArgs = [self._adbPath]
        if self._deviceSerial:
            finalArgs.extend(['-s', self._deviceSerial])
        if not self._haveRootShell and self._useRunAs and \
                args[0] == "shell" and args[1] not in [ "run-as", "am" ]:
            args.insert(1, "run-as")
            args.insert(2, self._packageName)
        finalArgs.extend(args)
        if not timeout:
            
            timeout = self.default_timeout

        timeout = int(timeout)
        retries = 0
        while retries < retryLimit:
            proc = subprocess.Popen(finalArgs, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            start_time = time.time()
            ret_code = proc.poll()
            while ((time.time() - start_time) <= timeout) and ret_code == None:
                time.sleep(self._pollingInterval)
                ret_code = proc.poll()
            if ret_code == None:
                proc.kill()
                retries += 1
                continue
            return ret_code
        raise DMError("Timeout exceeded for _checkCmd call after %d retries." % retries)

    def _checkCmdAs(self, args, timeout=None, retryLimit=None):
        """
        Runs a command using adb and waits for command to finish
        If self._useRunAs is True, the command is run-as user specified in self._packageName
        If timeout is specified, the process is killed after <timeout> seconds

        returns: returncode from subprocess.Popen
        """
        retryLimit = retryLimit or self.retryLimit
        if (self._useRunAs):
            args.insert(1, "run-as")
            args.insert(2, self._packageName)
        return self._checkCmd(args, timeout, retryLimit=retryLimit)

    def chmodDir(self, remoteDir, mask="777"):
        if (self.dirExists(remoteDir)):
            files = self.listFiles(remoteDir.strip())
            for f in files:
                remoteEntry = remoteDir.strip() + "/" + f.strip()
                if (self.dirExists(remoteEntry)):
                    self.chmodDir(remoteEntry)
                else:
                    self._checkCmdAs(["shell", "chmod", mask, remoteEntry])
                    print "chmod " + remoteEntry
            self._checkCmdAs(["shell", "chmod", mask, remoteDir])
            print "chmod " + remoteDir
        else:
            self._checkCmdAs(["shell", "chmod", mask, remoteDir.strip()])
            print "chmod " + remoteDir.strip()

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
        except subprocess.CalledProcessError:
            raise DMError("unable to execute ADB: ensure Android SDK is installed and adb is in your $PATH")

    def _verifyDevice(self):
        
        if self._deviceSerial:
            deviceStatus = None
            proc = subprocess.Popen([self._adbPath, "devices"],
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT)
            for line in proc.stdout:
                m = re.match('(.+)?\s+(.+)$', line)
                if m:
                    if self._deviceSerial == m.group(1):
                        deviceStatus = m.group(2)
            if deviceStatus == None:
                raise DMError("device not found: %s" % self._deviceSerial)
            elif deviceStatus != "device":
                raise DMError("bad status for device %s: %s" % (self._deviceSerial, deviceStatus))

        
        ret = None
        try:
            ret = self._checkCmd(["shell", "echo"])
        except subprocess.CalledProcessError:
            raise DMError("unable to connect to device: is it plugged in?")
        if ret:
            raise DMError("unable to connect to device")

    def _verifyRunAs(self):
        
        
        
        
        
        
        
        self._useRunAs = False
        devroot = self.getDeviceRoot()
        if self._packageName and devroot:
            tmpDir = self.getTempDir()

            
            
            runAsOut = self._runCmd(["shell", "run-as", self._packageName, "mkdir", devroot + "/sanity"]).communicate()[0]
            if runAsOut.startswith("run-as:") and ("not debuggable" in runAsOut or "is unknown" in runAsOut):
                raise DMError("run-as failed sanity check")

            tmpfile = tempfile.NamedTemporaryFile()
            self._checkCmd(["push", tmpfile.name, tmpDir + "/tmpfile"])
            self._checkCmd(["shell", "run-as", self._packageName, "dd", "if=" + tmpDir + "/tmpfile", "of=" + devroot + "/sanity/tmpfile"])
            if (self.fileExists(devroot + "/sanity/tmpfile")):
                print "will execute commands via run-as " + self._packageName
                self._useRunAs = True
            self._checkCmd(["shell", "rm", devroot + "/tmp/tmpfile"])
            self._checkCmd(["shell", "run-as", self._packageName, "rm", "-r", devroot + "/sanity"])

    def _checkForRoot(self):
        
        
        
        proc = self._runCmd(["shell", "id"])
        data = proc.stdout.read()
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

        data = proc.stdout.read()
        if data.find('uid=0(root)') >= 0:
            self._haveSu = True

    def _isUnzipAvailable(self):
        data = self._runCmdAs(["shell", "unzip"]).stdout.read()
        if (re.search('Usage', data)):
            return True
        else:
            return False

    def _isLocalZipAvailable(self):
        try:
            subprocess.check_call(["zip", "-?"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        except:
            return False
        return True

    def _verifyZip(self):
        
        
        
        self._useZip = False
        if (self._isUnzipAvailable() and self._isLocalZipAvailable()):
            print "will use zip to push directories"
            self._useZip = True
        else:
            raise DMError("zip not available")

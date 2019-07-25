





































import re, sys, os, os.path, logging, shutil, signal
from glob import glob
from optparse import OptionParser
from subprocess import Popen, PIPE, STDOUT
from tempfile import mkdtemp

import runxpcshelltests as xpcshell
from automationutils import *
import devicemanager

class XPCShellRemote(xpcshell.XPCShellTests, object):

    def __init__(self, devmgr):
        self.device = devmgr
        self.testRoot = "/tests/xpcshell"
        xpcshell.XPCShellTests.__init__(self)
        self.profileDir = self.testRoot + '/profile'
        self.device.mkDir(self.profileDir)

    
    def getcwd(self):
        return "/tests/"
        
    def readManifest(self, manifest):
        """Given a manifest file containing a list of test directories,
        return a list of absolute paths to the directories contained within."""

        manifestdir = self.testRoot + '/tests'
        testdirs = []
        try:
            f = self.device.getFile(manifest, "temp.txt")
            for line in f.split():
                dir = line.rstrip()
                path = manifestdir + '/' + dir
                testdirs.append(path)
            f.close()
        except:
            pass 
        return testdirs

    def verifyFilePath(self, fileName):
        
        
        
        
        if (self.device.fileExists(fileName)):
            return fileName
        
        fileName = self.device.getAppRoot() + '/xulrunner/' + fileName.split('/')[-1]
        if (self.device.fileExists(fileName)):
            return fileName
        
        fileName = self.device.getAppRoot() + '/' + fileName.split('/')[-1]
        if (not self.device.fileExists(fileName)):
            raise devicemanager.FileError("No File found for: " + str(fileName))

        return fileName

    def verifyDirPath(self, fileName):
        
        
        
        
        if (self.device.dirExists(fileName)):
            return fileName
        
        fileName = self.device.getAppRoot() + '/' + fileName.split('/')[-1]
        if (self.device.dirExists(fileName)):
            return fileName
        
        fileName = self.device.getDeviceRoot() + '/' + fileName.split('/')[-1]
        if (not self.device.dirExists(fileName)):
            raise devicemanager.FileError("No Dir found for: " + str(fileName))
        return fileName

    def setAbsPath(self):
        
        self.testharnessdir = "/tests/xpcshell/"

        
        self.xpcshell = self.verifyFilePath(self.xpcshell)
        if self.xrePath is None:
            
            self.xrePath = '/'.join(self.xpcshell.split('/')[:-1])
        else:
            self.xrePath = self.verifyDirPath(self.xrePath)

        
        self.httpdJSPath = self.xrePath + '/components/httpd.js'

    def buildXpcsCmd(self, testdir):
        
        self.env["XPCSHELL_TEST_PROFILE_DIR"] = self.profileDir
        self.xpcsCmd = [self.xpcshell, '-g', self.xrePath, '-v', '170', '-j', '-s', \
                        "--environ:CWD='" + testdir + "'", \
                        "--environ:XPCSHELL_TEST_PROFILE_DIR='" + self.env["XPCSHELL_TEST_PROFILE_DIR"] + "'", \
                        '-e', 'const _HTTPD_JS_PATH = \'%s\';' % self.httpdJSPath,
                        '-f', self.testharnessdir + '/head.js']

        if self.debuggerInfo:
            self.xpcsCmd = [self.debuggerInfo["path"]] + self.debuggerInfo["args"] + self.xpcsCmd

    def getHeadFiles(self, testdir):
        
        testHeadFiles = []
        for f in self.device.listFiles(testdir):
            hdmtch = re.compile("head_.*\.js")
            if (hdmtch.match(f)):
                testHeadFiles += [(testdir + '/' + f).replace('/', '//')]
                
        return sorted(testHeadFiles)
                
    def getTailFiles(self, testdir):
        testTailFiles = []
        
        
        for f in self.device.listFiles(testdir):
            tlmtch = re.compile("tail_.*\.js")
            if (tlmtch.match(f)):
                testTailFiles += [(testdir + '/' + f).replace('/', '//')]
        return reversed(sorted(testTailFiles))
        
    def getTestFiles(self, testdir):
        testfiles = []
        
        for f in self.device.listFiles(testdir):
            tstmtch = re.compile("test_.*\.js")
            if (tstmtch.match(f)):
                testfiles += [(testdir + '/' + f).replace('/', '//')]
        
        for f in testfiles:
            if (self.singleFile == f.split('/')[-1]):
                return [(testdir + '/' + f).replace('/', '//')]
            else:
                pass
        return testfiles

    def setupProfileDir(self):
        self.device.removeDir(self.profileDir)
        self.device.mkDir(self.profileDir)
        self.env["XPCSHELL_TEST_PROFILE_DIR"] = self.profileDir
        return self.profileDir

    def setupLeakLogging(self):
        filename = "runxpcshelltests_leaks.log"
        
        
        leakLogFile = self.profileDir + '/' + filename
        self.env["XPCOM_MEM_LEAK_LOG"] = leakLogFile
        return leakLogFile

    def launchProcess(self, cmd, stdout, stderr, env, cwd):
        print "launching : " + " ".join(cmd)
        proc = self.device.launchProcess(cmd, cwd=cwd)
        return proc

    def setSignal(self, proc, sig1, sig2):
        self.device.signal(proc, sig1, sig2)

    def communicate(self, proc):
        return self.device.communicate(proc)

    def removeDir(self, dirname):
        self.device.removeDir(dirname)

    def getReturnCode(self, proc):
        return self.device.getReturnCode(proc)

    
    
    def createLogFile(self, test, stdout):
        try:
            f = None
            filename = test.replace('\\', '/').split('/')[-1] + ".log"
            f = open(filename, "w")
            f.write(stdout)

            if os.path.exists(self.leakLogFile):
                leaks = open(self.leakLogFile, "r")
                f.write(leaks.read())
                leaks.close()
        finally:
            if f <> None:
                f.close()

    
    def buildCmdHead(self, headfiles, tailfiles, xpcscmd):
        cmdH = ", ".join(['\'' + f.replace('\\', '/') + '\''
                       for f in headfiles])
        cmdT = ", ".join(['\'' + f.replace('\\', '/') + '\''
                       for f in tailfiles])
        cmdH = xpcscmd + \
                ['-e', 'const _HEAD_FILES = [%s];' % cmdH] + \
                ['-e', 'const _TAIL_FILES = [%s];' % cmdT]
        return cmdH

class RemoteXPCShellOptions(xpcshell.XPCShellOptions):

  def __init__(self):
    xpcshell.XPCShellOptions.__init__(self)
    self.add_option("--device",
                    type="string", dest="device", default='',
                    help="ip address for the device")


def main():

  parser = RemoteXPCShellOptions()
  options, args = parser.parse_args()

  if len(args) < 2 and options.manifest is None or \
     (len(args) < 1 and options.manifest is not None):
     print "len(args): " + str(len(args))
     print >>sys.stderr, """Usage: %s <path to xpcshell> <test dirs>
           or: %s --manifest=test.manifest <path to xpcshell>""" % (sys.argv[0],
                                                           sys.argv[0])
     sys.exit(1)

  if (options.device == ''):
    print >>sys.stderr, "Error: Please provide an ip address for the remote device with the --device option"
    sys.exit(1)


  dm = devicemanager.DeviceManager(options.device, 20701)
  xpcsh = XPCShellRemote(dm)
  debuggerInfo = getDebuggerInfo(xpcsh.oldcwd, options.debugger, options.debuggerArgs,
    options.debuggerInteractive);

  if options.interactive and not options.testPath:
    print >>sys.stderr, "Error: You must specify a test filename in interactive mode!"
    sys.exit(1)

  
  
  zipName = 'xpcshell.7z'
  try:
    Popen(['7z', 'a', zipName, '../xpcshell']).wait()
  except:
    print "to run these tests remotely, we require 7z to be installed and in your path"
    sys.exit(1)

  if dm.pushFile(zipName, '/tests/xpcshell.7z') == None:
     raise devicemanager.FileError("failed to copy xpcshell.7z to device")
  if dm.unpackFile('xpcshell.7z') == None:
     raise devicemanager.FileError("failed to unpack xpcshell.7z on the device")

  if not xpcsh.runTests(args[0],
                        xrePath=options.xrePath,
                        symbolsPath=options.symbolsPath,
                        manifest=options.manifest,
                        testdirs=args[1:],
                        testPath=options.testPath,
                        interactive=options.interactive,
                        logfiles=options.logfiles,
                        debuggerInfo=debuggerInfo):
    sys.exit(1)

if __name__ == '__main__':
  main()




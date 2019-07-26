



"""
Runs the Mochitest test harness.
"""

from __future__ import with_statement
import os
import sys
SCRIPT_DIR = os.path.abspath(os.path.realpath(os.path.dirname(__file__)))
sys.path.insert(0, SCRIPT_DIR);

import ctypes
import glob
import json
import mozcrash
import mozinfo
import mozprocess
import mozrunner
import optparse
import re
import shutil
import signal
import subprocess
import tempfile
import time
import traceback
import urllib2
import zipfile

from automationutils import environment, getDebuggerInfo, isURL, KeyValueParseError, parseKeyValue, processLeakLog, systemMemory, dumpScreen, ShutdownLeaks, printstatus
from datetime import datetime
from manifestparser import TestManifest
from mochitest_options import MochitestOptions
from mozprofile import Profile, Preferences
from mozprofile.permissions import ServerLocations
from urllib import quote_plus as encodeURIComponent



from mozrunner.utils import findInPath as which


import logging
log = logging.getLogger()
def resetGlobalLog():
  while log.handlers:
    log.removeHandler(log.handlers[0])
  handler = logging.StreamHandler(sys.stdout)
  log.setLevel(logging.INFO)
  log.addHandler(handler)
resetGlobalLog()









NSPR_LOG_MODULES = ""





def call(*args, **kwargs):
  """front-end function to mozprocess.ProcessHandler"""
  
  
  process = mozprocess.ProcessHandler(*args, **kwargs)
  process.run()
  return process.wait()

def killPid(pid):
  
  try:
    os.kill(pid, getattr(signal, "SIGKILL", signal.SIGTERM))
  except Exception, e:
    log.info("Failed to kill process %d: %s", pid, str(e))

if mozinfo.isWin:
  import ctypes, ctypes.wintypes, time, msvcrt

  def isPidAlive(pid):
    STILL_ACTIVE = 259
    PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
    pHandle = ctypes.windll.kernel32.OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, 0, pid)
    if not pHandle:
      return False
    pExitCode = ctypes.wintypes.DWORD()
    ctypes.windll.kernel32.GetExitCodeProcess(pHandle, ctypes.byref(pExitCode))
    ctypes.windll.kernel32.CloseHandle(pHandle)
    return pExitCode.value == STILL_ACTIVE

else:
  import errno

  def isPidAlive(pid):
    try:
      
      
      os.kill(pid, 0)

      
      
      wpid, wstatus = os.waitpid(pid, os.WNOHANG)
      return wpid == 0
    except OSError, err:
      
      
      if err.errno == errno.ESRCH or err.errno == errno.ECHILD:
        return False
      raise






class MochitestServer(object):
  "Web server used to serve Mochitests, for closer fidelity to the real web."

  def __init__(self, options):
    if isinstance(options, optparse.Values):
      options = vars(options)
    self._closeWhenDone = options['closeWhenDone']
    self._utilityPath = options['utilityPath']
    self._xrePath = options['xrePath']
    self._profileDir = options['profilePath']
    self.webServer = options['webServer']
    self.httpPort = options['httpPort']
    self.shutdownURL = "http://%(server)s:%(port)s/server/shutdown" % { "server" : self.webServer, "port" : self.httpPort }
    self.testPrefix = "'webapprt_'" if options.get('webapprtContent') else "undefined"

    if options.get('httpdPath'):
        self._httpdPath = options['httpdPath']
    else:
        self._httpdPath = SCRIPT_DIR
    self._httpdPath = os.path.abspath(self._httpdPath)

  def start(self):
    "Run the Mochitest server, returning the process ID of the server."

    
    env = environment(xrePath=self._xrePath)
    env["XPCOM_DEBUG_BREAK"] = "warn"

    
    
    
    
    env["ASAN_OPTIONS"] = "quarantine_size=1:redzone=32:malloc_context_size=5"

    if mozinfo.isWin:
      env["PATH"] = env["PATH"] + ";" + str(self._xrePath)

    args = ["-g", self._xrePath,
            "-v", "170",
            "-f", os.path.join(self._httpdPath, "httpd.js"),
            "-e", """const _PROFILE_PATH = '%(profile)s'; const _SERVER_PORT = '%(port)s'; const _SERVER_ADDR = '%(server)s'; const _TEST_PREFIX = %(testPrefix)s; const _DISPLAY_RESULTS = %(displayResults)s;""" %
                   {"profile" : self._profileDir.replace('\\', '\\\\'), "port" : self.httpPort, "server" : self.webServer,
                    "testPrefix" : self.testPrefix, "displayResults" : str(not self._closeWhenDone).lower() },
            "-f", os.path.join(SCRIPT_DIR, "server.js")]

    xpcshell = os.path.join(self._utilityPath,
                            "xpcshell" + mozinfo.info['bin_suffix'])
    command = [xpcshell] + args
    self._process = mozprocess.ProcessHandler(command, cwd=SCRIPT_DIR, env=env)
    self._process.run()
    log.info("%s : launching %s", self.__class__.__name__, command)
    pid = self._process.pid
    log.info("runtests.py | Server pid: %d", pid)

  def ensureReady(self, timeout):
    assert timeout >= 0

    aliveFile = os.path.join(self._profileDir, "server_alive.txt")
    i = 0
    while i < timeout:
      if os.path.exists(aliveFile):
        break
      time.sleep(1)
      i += 1
    else:
      log.error("TEST-UNEXPECTED-FAIL | runtests.py | Timed out while waiting for server startup.")
      self.stop()
      sys.exit(1)

  def stop(self):
    try:
      with urllib2.urlopen(self.shutdownURL) as c:
        c.read()

      
      
      
      rtncode = self._process.proc.poll()
      if rtncode is None:
        
        
        
        self._process.proc.terminate()
    except:
      self._process.kill()

class WebSocketServer(object):
  "Class which encapsulates the mod_pywebsocket server"

  def __init__(self, options, scriptdir, debuggerInfo=None):
    self.port = options.webSocketPort
    self._scriptdir = scriptdir
    self.debuggerInfo = debuggerInfo

  def start(self):
    
    
    
    
    
    
    
    
    scriptPath = 'pywebsocket_wrapper.py'
    script = os.path.join(self._scriptdir, scriptPath)

    cmd = [sys.executable, script]
    if self.debuggerInfo and self.debuggerInfo['interactive']:
        cmd += ['--interactive']
    cmd += ['-p', str(self.port), '-w', self._scriptdir, '-l',      \
           os.path.join(self._scriptdir, "websock.log"),            \
           '--log-level=debug', '--allow-handlers-outside-root-dir']
    
    self._process = mozprocess.ProcessHandler(cmd, cwd=SCRIPT_DIR)
    self._process.run()
    pid = self._process.pid
    log.info("runtests.py | Websocket server pid: %d", pid)

  def stop(self):
    self._process.kill()

class MochitestUtilsMixin(object):
  """
  Class containing some utility functions common to both local and remote
  mochitest runners
  """

  
  
  
  

  oldcwd = os.getcwd()
  jarDir = 'mochijar'

  
  TEST_PATH = "tests"
  CHROME_PATH = "redirect.html"
  urlOpts = []

  def __init__(self):
    self.update_mozinfo()
    self.server = None
    self.wsserver = None
    self.sslTunnel = None
    self._locations = None

  def update_mozinfo(self):
    """walk up directories to find mozinfo.json update the info"""
    

    path = SCRIPT_DIR
    dirs = set()
    while path != os.path.expanduser('~'):
        if path in dirs:
            break
        dirs.add(path)
        path = os.path.split(path)[0]

    mozinfo.find_and_update_from_json(*dirs)

  def getFullPath(self, path):
    " Get an absolute path relative to self.oldcwd."
    return os.path.normpath(os.path.join(self.oldcwd, os.path.expanduser(path)))

  def getLogFilePath(self, logFile):
    """ return the log file path relative to the device we are testing on, in most cases
        it will be the full path on the local system
    """
    return self.getFullPath(logFile)

  @property
  def locations(self):
    if self._locations is not None:
      return self._locations
    locations_file = os.path.join(SCRIPT_DIR, 'server-locations.txt')
    self._locations = ServerLocations(locations_file)
    return self._locations

  def buildURLOptions(self, options, env):
    """ Add test control options from the command line to the url

        URL parameters to test URL:

        autorun -- kick off tests automatically
        closeWhenDone -- closes the browser after the tests
        hideResultsTable -- hides the table of individual test results
        logFile -- logs test run to an absolute path
        totalChunks -- how many chunks to split tests into
        thisChunk -- which chunk to run
        startAt -- name of test to start at
        endAt -- name of test to end at
        timeout -- per-test timeout in seconds
        repeat -- How many times to repeat the test, ie: repeat=1 will run the test twice.
    """

    
    if options.logFile:
      options.logFile = self.getLogFilePath(options.logFile)

    
    if options.browserChrome or options.chrome or options.subsuite or \
       options.a11y or options.webapprtChrome:
      self.makeTestConfig(options)
    else:
      if options.autorun:
        self.urlOpts.append("autorun=1")
      if options.timeout:
        self.urlOpts.append("timeout=%d" % options.timeout)
      if options.closeWhenDone:
        self.urlOpts.append("closeWhenDone=1")
      if options.logFile:
        self.urlOpts.append("logFile=" + encodeURIComponent(options.logFile))
        self.urlOpts.append("fileLevel=" + encodeURIComponent(options.fileLevel))
      if options.consoleLevel:
        self.urlOpts.append("consoleLevel=" + encodeURIComponent(options.consoleLevel))
      if options.totalChunks:
        self.urlOpts.append("totalChunks=%d" % options.totalChunks)
        self.urlOpts.append("thisChunk=%d" % options.thisChunk)
      if options.chunkByDir:
        self.urlOpts.append("chunkByDir=%d" % options.chunkByDir)
      if options.startAt:
        self.urlOpts.append("startAt=%s" % options.startAt)
      if options.endAt:
        self.urlOpts.append("endAt=%s" % options.endAt)
      if options.shuffle:
        self.urlOpts.append("shuffle=1")
      if "MOZ_HIDE_RESULTS_TABLE" in env and env["MOZ_HIDE_RESULTS_TABLE"] == "1":
        self.urlOpts.append("hideResultsTable=1")
      if options.runUntilFailure:
        self.urlOpts.append("runUntilFailure=1")
      if options.repeat:
        self.urlOpts.append("repeat=%d" % options.repeat)
      if os.path.isfile(os.path.join(self.oldcwd, os.path.dirname(__file__), self.TEST_PATH, options.testPath)) and options.repeat > 0:
        self.urlOpts.append("testname=%s" % ("/").join([self.TEST_PATH, options.testPath]))
      if options.testManifest:
        self.urlOpts.append("testManifest=%s" % options.testManifest)
        if hasattr(options, 'runOnly') and options.runOnly:
          self.urlOpts.append("runOnly=true")
        else:
          self.urlOpts.append("runOnly=false")
      if options.manifestFile:
        self.urlOpts.append("manifestFile=%s" % options.manifestFile)
      if options.failureFile:
        self.urlOpts.append("failureFile=%s" % self.getFullPath(options.failureFile))
      if options.runSlower:
        self.urlOpts.append("runSlower=true")
      if options.debugOnFailure:
        self.urlOpts.append("debugOnFailure=true")
      if options.dumpOutputDirectory:
        self.urlOpts.append("dumpOutputDirectory=%s" % encodeURIComponent(options.dumpOutputDirectory))
      if options.dumpAboutMemoryAfterTest:
        self.urlOpts.append("dumpAboutMemoryAfterTest=true")
      if options.dumpDMDAfterTest:
        self.urlOpts.append("dumpDMDAfterTest=true")
      if options.quiet:
        self.urlOpts.append("quiet=true")

  def getTestFlavor(self, options):
    if options.browserChrome:
      return "browser-chrome"
    elif options.chrome:
      return "chrome"
    elif options.a11y:
      return "a11y"
    elif options.webapprtChrome:
      return "webapprt-chrome"
    else:
      return "mochitest"

  
  def isTest(self, options, filename):
    allow_js_css = False
    if options.browserChrome:
      allow_js_css = True
      testPattern = re.compile(r"browser_.+\.js")
    elif options.chrome or options.a11y:
      testPattern = re.compile(r"(browser|test)_.+\.(xul|html|js|xhtml)")
    elif options.webapprtContent:
      testPattern = re.compile(r"webapprt_")
    elif options.webapprtChrome:
      allow_js_css = True
      testPattern = re.compile(r"browser_")
    else:
      testPattern = re.compile(r"test_")

    if not allow_js_css and (".js" in filename or ".css" in filename):
      return False

    pathPieces = filename.split("/")

    return (testPattern.match(pathPieces[-1]) and
            not re.search(r'\^headers\^$', filename))

  def getTestPath(self, options):
    if options.ipcplugins:
      return "dom/plugins/test"
    else:
      return options.testPath

  def getTestRoot(self, options):
    if options.browserChrome:
      if options.immersiveMode:
        return 'metro'
      return 'browser'
    elif options.a11y:
      return 'a11y'
    elif options.webapprtChrome:
      return 'webapprtChrome'
    elif options.chrome:
      return 'chrome'
    return self.TEST_PATH

  def buildTestURL(self, options):
    testHost = "http://mochi.test:8888"
    testPath = self.getTestPath(options)
    testURL = "/".join([testHost, self.TEST_PATH, testPath])
    if os.path.isfile(os.path.join(self.oldcwd, os.path.dirname(__file__), self.TEST_PATH, testPath)) and options.repeat > 0:
      testURL = "/".join([testHost, self.TEST_PATH, os.path.dirname(testPath)])
    if options.chrome or options.a11y:
      testURL = "/".join([testHost, self.CHROME_PATH])
    elif options.browserChrome:
      testURL = "about:blank"
    return testURL

  def buildTestPath(self, options):
    """ Build the url path to the specific test harness and test file or directory
        Build a manifest of tests to run and write out a json file for the harness to read
    """
    manifest = None

    testRoot = self.getTestRoot(options)
    
    testdir = SCRIPT_DIR.split(os.getcwd())[-1]
    testdir = testdir.strip(os.sep)
    testRootAbs = os.path.abspath(os.path.join(testdir, testRoot))
    if isinstance(options.manifestFile, TestManifest):
        manifest = options.manifestFile
    elif options.manifestFile and os.path.isfile(options.manifestFile):
      manifestFileAbs = os.path.abspath(options.manifestFile)
      assert manifestFileAbs.startswith(testRootAbs)
      manifest = TestManifest([options.manifestFile], strict=False)
    else:
      masterName = self.getTestFlavor(options) + '.ini'
      masterPath = os.path.join(testdir, testRoot, masterName)

      if os.path.exists(masterPath):
        manifest = TestManifest([masterPath], strict=False)

    if manifest:
      
      
      
      info = {}
      for k, v in mozinfo.info.items():
        if isinstance(k, unicode):
          k = k.encode('ascii')
        info[k] = v

      
      tests = manifest.active_tests(disabled=True, options=options, **info)
      paths = []
      testPath = self.getTestPath(options)

      for test in tests:
        pathAbs = os.path.abspath(test['path'])
        assert pathAbs.startswith(testRootAbs)
        tp = pathAbs[len(testRootAbs):].replace('\\', '/').strip('/')

        
        if testPath and not tp.startswith(testPath):
          continue

        if not self.isTest(options, tp):
          print 'Warning: %s from manifest %s is not a valid test' % (test['name'], test['manifest'])
          continue

        testob = {'path': tp}
        if test.has_key('disabled'):
          testob['disabled'] = test['disabled']
        paths.append(testob)

      
      def path_sort(ob1, ob2):
        path1 = ob1['path'].split('/')
        path2 = ob2['path'].split('/')
        return cmp(path1, path2)

      paths.sort(path_sort)

      
      with open(os.path.join(testdir, 'tests.json'), 'w') as manifestFile:
        manifestFile.write(json.dumps({'tests': paths}))
      options.manifestFile = 'tests.json'

    return self.buildTestURL(options)

  def startWebSocketServer(self, options, debuggerInfo):
    """ Launch the websocket server """
    self.wsserver = WebSocketServer(options, SCRIPT_DIR, debuggerInfo)
    self.wsserver.start()

  def startWebServer(self, options):
    """Create the webserver and start it up"""

    self.server = MochitestServer(options)
    self.server.start()

    if options.pidFile != "":
      with open(options.pidFile + ".xpcshell.pid", 'w') as f:
        f.write("%s" % self.server._process.pid)

  def startServers(self, options, debuggerInfo):
    
    
    self.webServer = options.webServer
    self.httpPort = options.httpPort
    self.sslPort = options.sslPort
    self.webSocketPort = options.webSocketPort

    
    
    
    if not options.httpdPath:
      options.httpdPath = os.path.join(options.utilityPath, "components")

    self.startWebServer(options)
    self.startWebSocketServer(options, debuggerInfo)

    
    self.sslTunnel = SSLTunnel(options)
    self.sslTunnel.buildConfig(self.locations)
    self.sslTunnel.start()

    
    
    
    
    if self.server is not None:
      self.server.ensureReady(self.SERVER_STARTUP_TIMEOUT)

  def stopServers(self):
    """Servers are no longer needed, and perhaps more importantly, anything they
        might spew to console might confuse things."""
    if self.server is not None:
      try:
        log.info('Stopping web server')
        self.server.stop()
      except Exception:
        log.exception('Exception when stopping web server')

    if self.wsserver is not None:
      try:
        log.info('Stopping web socket server')
        self.wsserver.stop()
      except Exception:
        log.exception('Exception when stopping web socket server');

    if self.sslTunnel is not None:
      try:
        log.info('Stopping ssltunnel')
        self.sslTunnel.stop()
      except Exception:
        log.exception('Exception stopping ssltunnel');

  def copyExtraFilesToProfile(self, options):
    "Copy extra files or dirs specified on the command line to the testing profile."
    for f in options.extraProfileFiles:
      abspath = self.getFullPath(f)
      if os.path.isfile(abspath):
        shutil.copy2(abspath, options.profilePath)
      elif os.path.isdir(abspath):
        dest = os.path.join(options.profilePath, os.path.basename(abspath))
        shutil.copytree(abspath, dest)
      else:
        log.warning("runtests.py | Failed to copy %s to profile", abspath)

  def installChromeJar(self, chrome, options):
    """
      copy mochijar directory to profile as an extension so we have chrome://mochikit for all harness code
    """
    
    with open(os.path.join(options.profilePath, "extensions", "staged", "mochikit@mozilla.org", "chrome.manifest"), "a") as mfile:
      mfile.write(chrome)

  def addChromeToProfile(self, options):
    "Adds MochiKit chrome tests to the profile."

    
    chromedir = os.path.join(options.profilePath, "chrome")
    os.mkdir(chromedir)

    
    chrome = """
@namespace url("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul"); /* set default namespace to XUL */
toolbar,
toolbarpalette {
  background-color: rgb(235, 235, 235) !important;
}
toolbar#nav-bar {
  background-image: none !important;
}
"""
    with open(os.path.join(options.profilePath, "userChrome.css"), "a") as chromeFile:
      chromeFile.write(chrome)

    manifest = os.path.join(options.profilePath, "tests.manifest")
    with open(manifest, "w") as manifestFile:
      
      chrometestDir = os.path.join(os.path.abspath("."), SCRIPT_DIR) + "/"
      if mozinfo.isWin:
        chrometestDir = "file:///" + chrometestDir.replace("\\", "/")
      manifestFile.write("content mochitests %s contentaccessible=yes\n" % chrometestDir)

      if options.testingModulesDir is not None:
        manifestFile.write("resource testing-common file:///%s\n" %
          options.testingModulesDir)

    
    if not os.path.isdir(os.path.join(SCRIPT_DIR, self.jarDir)):
      log.testFail("invalid setup: missing mochikit extension")
      return None

    
    
    chrome = ""
    if options.browserChrome or options.chrome or options.a11y or options.webapprtChrome:
      chrome += """
overlay chrome://browser/content/browser.xul chrome://mochikit/content/browser-test-overlay.xul
overlay chrome://browser/content/shell.xhtml chrome://mochikit/content/browser-test-overlay.xul
overlay chrome://navigator/content/navigator.xul chrome://mochikit/content/browser-test-overlay.xul
overlay chrome://webapprt/content/webapp.xul chrome://mochikit/content/browser-test-overlay.xul
"""

    self.installChromeJar(chrome, options)
    return manifest

  def getExtensionsToInstall(self, options):
    "Return a list of extensions to install in the profile"
    extensions = options.extensionsToInstall or []
    appDir = options.app[:options.app.rfind(os.sep)] if options.app else options.utilityPath

    extensionDirs = [
      
      os.path.normpath(os.path.join(SCRIPT_DIR, "extensions")),
    ]
    if appDir:
      
      extensionDirs.append(os.path.join(appDir, "distribution", "extensions"))

    for extensionDir in extensionDirs:
      if os.path.isdir(extensionDir):
        for dirEntry in os.listdir(extensionDir):
          if dirEntry not in options.extensionsToExclude:
            path = os.path.join(extensionDir, dirEntry)
            if os.path.isdir(path) or (os.path.isfile(path) and path.endswith(".xpi")):
              extensions.append(path)

    
    extensions.append(os.path.join(SCRIPT_DIR, self.jarDir))
    return extensions

class SSLTunnel:
  def __init__(self, options):
    self.process = None
    self.utilityPath = options.utilityPath
    self.xrePath = options.xrePath
    self.certPath = options.certPath
    self.sslPort = options.sslPort
    self.httpPort = options.httpPort
    self.webServer = options.webServer
    self.webSocketPort = options.webSocketPort

    self.customCertRE = re.compile("^cert=(?P<nickname>[0-9a-zA-Z_ ]+)")
    self.clientAuthRE = re.compile("^clientauth=(?P<clientauth>[a-z]+)")
    self.redirRE      = re.compile("^redir=(?P<redirhost>[0-9a-zA-Z_ .]+)")

  def writeLocation(self, config, loc):
    for option in loc.options:
      match = self.customCertRE.match(option)
      if match:
        customcert = match.group("nickname");
        config.write("listen:%s:%s:%s:%s\n" %
                     (loc.host, loc.port, self.sslPort, customcert))

      match = self.clientAuthRE.match(option)
      if match:
        clientauth = match.group("clientauth");
        config.write("clientauth:%s:%s:%s:%s\n" %
                     (loc.host, loc.port, self.sslPort, clientauth))

      match = self.redirRE.match(option)
      if match:
        redirhost = match.group("redirhost")
        config.write("redirhost:%s:%s:%s:%s\n" %
                     (loc.host, loc.port, self.sslPort, redirhost))

  def buildConfig(self, locations):
    """Create the ssltunnel configuration file"""
    configFd, self.configFile = tempfile.mkstemp(prefix="ssltunnel", suffix=".cfg")
    with os.fdopen(configFd, "w") as config:
      config.write("httpproxy:1\n")
      config.write("certdbdir:%s\n" % self.certPath)
      config.write("forward:127.0.0.1:%s\n" % self.httpPort)
      config.write("websocketserver:%s:%s\n" % (self.webServer, self.webSocketPort))
      config.write("listen:*:%s:pgo server certificate\n" % self.sslPort)

      for loc in locations:
        if loc.scheme == "https" and "nocert" not in loc.options:
          self.writeLocation(config, loc)

  def start(self):
    """ Starts the SSL Tunnel """

    
    bin_suffix = mozinfo.info.get('bin_suffix', '')
    ssltunnel = os.path.join(self.utilityPath, "ssltunnel" + bin_suffix)
    if not os.path.exists(ssltunnel):
      log.error("INFO | runtests.py | expected to find ssltunnel at %s", ssltunnel)
      exit(1)

    env = environment(xrePath=self.xrePath)
    self.process = mozprocess.ProcessHandler([ssltunnel, self.configFile],
                                               env=env)
    self.process.run()
    log.info("INFO | runtests.py | SSL tunnel pid: %d", self.process.pid)

  def stop(self):
    """ Stops the SSL Tunnel and cleans up """
    if self.process is not None:
      self.process.kill()
    if os.path.exists(self.configFile):
      os.remove(self.configFile)

def checkAndConfigureV4l2loopback(device):
  '''
  Determine if a given device path is a v4l2loopback device, and if so
  toggle a few settings on it via fcntl. Very linux-specific.

  Returns (status, device name) where status is a boolean.
  '''
  if not mozinfo.isLinux:
    return False, ''

  libc = ctypes.cdll.LoadLibrary('libc.so.6')
  O_RDWR = 2
  
  class v4l2_capability(ctypes.Structure):
    _fields_ = [
      ('driver', ctypes.c_char * 16),
      ('card', ctypes.c_char * 32),
      ('bus_info', ctypes.c_char * 32),
      ('version', ctypes.c_uint32),
      ('capabilities', ctypes.c_uint32),
      ('device_caps', ctypes.c_uint32),
      ('reserved', ctypes.c_uint32 * 3)
      ]
  VIDIOC_QUERYCAP = 0x80685600

  fd = libc.open(device, O_RDWR)
  if fd < 0:
    return False, ''

  vcap = v4l2_capability()
  if libc.ioctl(fd, VIDIOC_QUERYCAP, ctypes.byref(vcap)) != 0:
    return False, ''

  if vcap.driver != 'v4l2 loopback':
    return False, ''

  class v4l2_control(ctypes.Structure):
    _fields_ = [
      ('id', ctypes.c_uint32),
      ('value', ctypes.c_int32)
    ]

  
  
  KEEP_FORMAT = 0x8000000
  SUSTAIN_FRAMERATE = 0x8000001
  VIDIOC_S_CTRL = 0xc008561c

  control = v4l2_control()
  control.id = KEEP_FORMAT
  control.value = 1
  libc.ioctl(fd, VIDIOC_S_CTRL, ctypes.byref(control))

  control.id = SUSTAIN_FRAMERATE
  control.value = 1
  libc.ioctl(fd, VIDIOC_S_CTRL, ctypes.byref(control))
  libc.close(fd)

  return True, vcap.card

def findTestMediaDevices():
  '''
  Find the test media devices configured on this system, and return a dict
  containing information about them. The dict will have keys for 'audio'
  and 'video', each containing the name of the media device to use.

  If audio and video devices could not be found, return None.

  This method is only currently implemented for Linux.
  '''
  if not mozinfo.isLinux:
    return None

  info = {}
  
  name = None
  device = None
  for dev in sorted(glob.glob('/dev/video*')):
    result, name_ = checkAndConfigureV4l2loopback(dev)
    if result:
      name = name_
      device = dev
      break

  if not (name and device):
    log.error('Couldn\'t find a v4l2loopback video device')
    return None

  
  subprocess.check_call(['/usr/bin/gst-launch-0.10', 'videotestsrc',
                         'pattern=green', 'num-buffers=1', '!',
                         'v4l2sink', 'device=%s' % device])
  info['video'] = name

  
  def sine_source_loaded():
    o = subprocess.check_output(['/usr/bin/pactl', 'list', 'short', 'modules'])
    return filter(lambda x: 'module-sine-source' in x, o.splitlines())

  if not sine_source_loaded():
    
    subprocess.check_call(['/usr/bin/pactl', 'load-module',
                           'module-sine-source'])
  if not sine_source_loaded():
    log.error('Couldn\'t load module-sine-source')
    return None

  
  info['audio'] = 'Sine source at 440 Hz'
  return info

class Mochitest(MochitestUtilsMixin):
  certdbNew = False
  sslTunnel = None
  vmwareHelper = None
  DEFAULT_TIMEOUT = 60.0
  mediaDevices = None

  
  
  test_name = 'automation.py'

  def __init__(self):
    super(Mochitest, self).__init__()

    
    self.environment = environment

    
    
    
    self.SERVER_STARTUP_TIMEOUT = 180 if mozinfo.info.get('debug') else 90

    
    self.browserProcessId = None


    self.haveDumpedScreen = False

  def extraPrefs(self, extraPrefs):
    """interpolate extra preferences from option strings"""

    try:
      return dict(parseKeyValue(extraPrefs, context='--setpref='))
    except KeyValueParseError, e:
      print str(e)
      sys.exit(1)

  def fillCertificateDB(self, options):
    
    

    pwfilePath = os.path.join(options.profilePath, ".crtdbpw")
    with open(pwfilePath, "w") as pwfile:
      pwfile.write("\n")

    
    env = self.environment(xrePath=options.xrePath)
    bin_suffix = mozinfo.info.get('bin_suffix', '')
    certutil = os.path.join(options.utilityPath, "certutil" + bin_suffix)
    pk12util = os.path.join(options.utilityPath, "pk12util" + bin_suffix)

    if self.certdbNew:
      
      certdbPath = "sql:" + options.profilePath
    else:
      
      certdbPath = options.profilePath

    status = call([certutil, "-N", "-d", certdbPath, "-f", pwfilePath], env=env)
    if status:
      return status

    
    files = os.listdir(options.certPath)
    for item in files:
      root, ext = os.path.splitext(item)
      if ext == ".ca":
        trustBits = "CT,,"
        if root.endswith("-object"):
          trustBits = "CT,,CT"
        call([certutil, "-A", "-i", os.path.join(options.certPath, item),
              "-d", certdbPath, "-f", pwfilePath, "-n", root, "-t", trustBits],
              env=env)
      elif ext == ".client":
        call([pk12util, "-i", os.path.join(options.certPath, item),
              "-w", pwfilePath, "-d", certdbPath],
              env=env)

    os.unlink(pwfilePath)
    return 0

  def buildProfile(self, options):
    """ create the profile and add optional chrome bits and files if requested """
    if options.browserChrome and options.timeout:
      options.extraPrefs.append("testing.browserTestHarness.timeout=%d" % options.timeout)
    options.extraPrefs.append("browser.tabs.remote=%s" % ('true' if options.e10s else 'false'))
    options.extraPrefs.append("browser.tabs.remote.autostart=%s" % ('true' if options.e10s else 'false'))

    
    extensions = self.getExtensionsToInstall(options)

    
    appsPath = os.path.join(SCRIPT_DIR, 'profile_data', 'webapps_mochitest.json')
    if os.path.exists(appsPath):
      with open(appsPath) as apps_file:
        apps = json.load(apps_file)
    else:
      apps = None

    
    prefsPath = os.path.join(SCRIPT_DIR, 'profile_data', 'prefs_general.js')
    prefs = dict(Preferences.read_prefs(prefsPath))
    prefs.update(self.extraPrefs(options.extraPrefs))

    
    interpolation = {"server": "%s:%s" % (options.webServer, options.httpPort)}
    prefs = json.loads(json.dumps(prefs) % interpolation)
    for pref in prefs:
      prefs[pref] = Preferences.cast(prefs[pref])
    
    

    
    proxy = {'remote': options.webServer,
             'http': options.httpPort,
             'https': options.sslPort,
    
    
    
    
    
             'ws': options.sslPort
             }

    
    if options.useTestMediaDevices:
      prefs['media.audio_loopback_dev'] = self.mediaDevices['audio']
      prefs['media.video_loopback_dev'] = self.mediaDevices['video']


    
    self.profile = Profile(profile=options.profilePath,
                           addons=extensions,
                           locations=self.locations,
                           preferences=prefs,
                           apps=apps,
                           proxy=proxy
                           )

    
    options.profilePath = self.profile.profile

    manifest = self.addChromeToProfile(options)
    self.copyExtraFilesToProfile(options)

    
    
    certificateStatus = self.fillCertificateDB(options)
    if certificateStatus:
      log.info("TEST-UNEXPECTED-FAIL | runtests.py | Certificate integration failed")
      return None

    return manifest

  def buildBrowserEnv(self, options, debugger=False):
    """build the environment variables for the specific test and operating system"""
    browserEnv = self.environment(xrePath=options.xrePath, debugger=debugger,
                                  dmdPath=options.dmdPath)

    
    
    browserEnv["XPCOM_DEBUG_BREAK"] = "stack"

    
    try:
      browserEnv.update(dict(parseKeyValue(options.environment, context='--setenv')))
    except KeyValueParseError, e:
      log.error(str(e))
      return

    browserEnv["XPCOM_MEM_BLOAT_LOG"] = self.leak_report_file

    if options.fatalAssertions:
      browserEnv["XPCOM_DEBUG_BREAK"] = "stack-and-abort"

    
    
    self.nsprLogs = NSPR_LOG_MODULES and "MOZ_UPLOAD_DIR" in os.environ
    if self.nsprLogs:
      browserEnv["NSPR_LOG_MODULES"] = NSPR_LOG_MODULES

      browserEnv["NSPR_LOG_FILE"] = "%s/nspr.log" % tempfile.gettempdir()
      browserEnv["GECKO_SEPARATE_NSPR_LOGS"] = "1"

    if debugger and not options.slowscript:
      browserEnv["JS_DISABLE_SLOW_SCRIPT_SIGNALS"] = "1"

    return browserEnv

  def cleanup(self, manifest, options):
    """ remove temporary files and profile """
    os.remove(manifest)
    del self.profile
    if options.pidFile != "":
      try:
        os.remove(options.pidFile)
        if os.path.exists(options.pidFile + ".xpcshell.pid"):
          os.remove(options.pidFile + ".xpcshell.pid")
      except:
        log.warn("cleaning up pidfile '%s' was unsuccessful from the test harness", options.pidFile)

  def dumpScreen(self, utilityPath):
    if self.haveDumpedScreen:
      log.info("Not taking screenshot here: see the one that was previously logged")
      return
    self.haveDumpedScreen = True
    dumpScreen(utilityPath)

  def killAndGetStack(self, processPID, utilityPath, debuggerInfo, dump_screen=False):
    """
    Kill the process, preferrably in a way that gets us a stack trace.
    Also attempts to obtain a screenshot before killing the process
    if specified.
    """

    if dump_screen:
      self.dumpScreen(utilityPath)

    if mozinfo.info.get('crashreporter', True) and not debuggerInfo:
      if mozinfo.isWin:
        
        crashinject = os.path.normpath(os.path.join(utilityPath, "crashinject.exe"))
        if os.path.exists(crashinject):
          status = subprocess.Popen([crashinject, str(processPID)]).wait()
          printstatus(status, "crashinject")
          if status == 0:
            return
      else:
        try:
          os.kill(processPID, signal.SIGABRT)
        except OSError:
          
          log.info("Can't trigger Breakpad, process no longer exists")
        return
    log.info("Can't trigger Breakpad, just killing process")
    killPid(processPID)

  def checkForZombies(self, processLog, utilityPath, debuggerInfo):
    """Look for hung processes"""

    if not os.path.exists(processLog):
      log.info('Automation Error: PID log not found: %s', processLog)
      
      return True

    
    log.info('INFO | zombiecheck | Reading PID log: %s', processLog)
    processList = []
    pidRE = re.compile(r'launched child process (\d+)$')
    with open(processLog) as processLogFD:
      for line in processLogFD:
        log.info(line.rstrip())
        m = pidRE.search(line)
        if m:
          processList.append(int(m.group(1)))

    
    foundZombie = False
    for processPID in processList:
      log.info("INFO | zombiecheck | Checking for orphan process with PID: %d", processPID)
      if isPidAlive(processPID):
        foundZombie = True
        log.info("TEST-UNEXPECTED-FAIL | zombiecheck | child process %d still alive after shutdown", processPID)
        self.killAndGetStack(processPID, utilityPath, debuggerInfo, dump_screen=not debuggerInfo)

    return foundZombie

  def startVMwareRecording(self, options):
    """ starts recording inside VMware VM using the recording helper dll """
    assert mozinfo.isWin
    from ctypes import cdll
    self.vmwareHelper = cdll.LoadLibrary(self.vmwareHelperPath)
    if self.vmwareHelper is None:
      log.warning("runtests.py | Failed to load "
                   "VMware recording helper")
      return
    log.info("runtests.py | Starting VMware recording.")
    try:
      self.vmwareHelper.StartRecording()
    except Exception, e:
      log.warning("runtests.py | Failed to start "
                  "VMware recording: (%s)" % str(e))
      self.vmwareHelper = None

  def stopVMwareRecording(self):
    """ stops recording inside VMware VM using the recording helper dll """
    try:
      assert mozinfo.isWin
      if self.vmwareHelper is not None:
        log.info("runtests.py | Stopping VMware recording.")
        self.vmwareHelper.StopRecording()
    except Exception, e:
      log.warning("runtests.py | Failed to stop "
                  "VMware recording: (%s)" % str(e))
      log.exception('Error stopping VMWare recording')

    self.vmwareHelper = None

  def runApp(self,
             testUrl,
             env,
             app,
             profile,
             extraArgs,
             utilityPath,
             debuggerInfo=None,
             symbolsPath=None,
             timeout=-1,
             onLaunch=None,
             webapprtChrome=False,
             hide_subtests=False,
             screenshotOnFail=False):
    """
    Run the app, log the duration it took to execute, return the status code.
    Kills the app if it runs for longer than |maxTime| seconds, or outputs nothing for |timeout| seconds.
    """

    
    interactive = False
    debug_args = None
    if debuggerInfo:
        interactive = debuggerInfo['interactive']
        debug_args = [debuggerInfo['path']] + debuggerInfo['args']

    
    if timeout == -1:
      timeout = self.DEFAULT_TIMEOUT

    
    is_test_build = mozinfo.info.get('tests_enabled', True)
    bin_suffix = mozinfo.info.get('bin_suffix', '')

    
    env = env.copy()

    
    try:
      
      tmpfd, processLog = tempfile.mkstemp(suffix='pidlog')
      os.close(tmpfd)
      env["MOZ_PROCESS_LOG"] = processLog

      if interactive:
        
        
        timeout = None
        signal.signal(signal.SIGINT, lambda sigid, frame: None)

      
      cmd = os.path.abspath(app)
      args = list(extraArgs)
      
      
      args.append('-foreground')
      if testUrl:
        if debuggerInfo and debuggerInfo['requiresEscapedArgs']:
          testUrl = testUrl.replace("&", "\\&")
        args.append(testUrl)

      if mozinfo.info["debug"] and not webapprtChrome:
        shutdownLeaks = ShutdownLeaks(log.info)
      else:
        shutdownLeaks = None

      
      outputHandler = self.OutputHandler(harness=self,
                                         utilityPath=utilityPath,
                                         symbolsPath=symbolsPath,
                                         dump_screen_on_timeout=not debuggerInfo,
                                         dump_screen_on_fail=screenshotOnFail,
                                         hide_subtests=hide_subtests,
                                         shutdownLeaks=shutdownLeaks,
        )

      def timeoutHandler():
        outputHandler.log_output_buffer()
        browserProcessId = outputHandler.browserProcessId
        self.handleTimeout(timeout, proc, utilityPath, debuggerInfo, browserProcessId)
      kp_kwargs = {'kill_on_timeout': False,
                   'cwd': SCRIPT_DIR,
                   'onTimeout': [timeoutHandler]}
      kp_kwargs['processOutputLine'] = [outputHandler]

      
      self.lastTestSeen = self.test_name
      startTime = datetime.now()

      
      if mozinfo.info.get('appname') == 'b2g' and mozinfo.info.get('toolkit') != 'gonk':
          runner_cls = mozrunner.FirefoxRunner
      else:
          runner_cls = mozrunner.runners.get(mozinfo.info.get('appname', 'firefox'),
                                             mozrunner.Runner)
      runner = runner_cls(profile=self.profile,
                          binary=cmd,
                          cmdargs=args,
                          env=env,
                          process_class=mozprocess.ProcessHandlerMixin,
                          kp_kwargs=kp_kwargs,
                          )

      
      
      runner.kp_kwargs = kp_kwargs

      
      runner.start(debug_args=debug_args,
                   interactive=interactive,
                   outputTimeout=timeout)
      proc = runner.process_handler
      log.info("INFO | runtests.py | Application pid: %d", proc.pid)

      if onLaunch is not None:
        
        
        
        
        onLaunch()

      
      
      
      
      
      status = proc.wait()
      printstatus(status, "Main app process")
      runner.process_handler = None

      if timeout is None:
        didTimeout = False
      else:
        didTimeout = proc.didTimeout

      
      outputHandler.finish(didTimeout)

      
      if status:
        log.info("TEST-UNEXPECTED-FAIL | %s | application terminated with exit code %s", self.lastTestSeen, status)
      else:
        self.lastTestSeen = 'Main app process exited normally'

      log.info("INFO | runtests.py | Application ran for: %s", str(datetime.now() - startTime))

      
      zombieProcesses = self.checkForZombies(processLog, utilityPath, debuggerInfo)

      
      minidump_path = os.path.join(self.profile.profile, "minidumps")
      crashed = mozcrash.check_for_crashes(minidump_path,
                                           symbolsPath,
                                           test_name=self.lastTestSeen)

      if crashed or zombieProcesses:
        status = 1

    finally:
      
      if os.path.exists(processLog):
        os.remove(processLog)

    return status

  def runTests(self, options, onLaunch=None):
    """ Prepare, configure, run tests and cleanup """

    
    
    
    
    
    
    debuggerInfo = getDebuggerInfo(self.oldcwd,
                                   options.debugger,
                                   options.debuggerArgs,
                                   options.debuggerInteractive)

    if options.useTestMediaDevices:
      devices = findTestMediaDevices()
      if not devices:
        log.error("Could not find test media devices to use")
        return 1
      self.mediaDevices = devices

    self.leak_report_file = os.path.join(options.profilePath, "runtests_leaks.log")

    browserEnv = self.buildBrowserEnv(options, debuggerInfo is not None)
    if browserEnv is None:
      return 1

    
    
    
    manifest = self.buildProfile(options)
    if manifest is None:
      return 1

    try:
      self.startServers(options, debuggerInfo)

      testURL = self.buildTestPath(options)
      self.buildURLOptions(options, browserEnv)
      if self.urlOpts:
        testURL += "?" + "&".join(self.urlOpts)

      if options.webapprtContent:
        options.browserArgs.extend(('-test-mode', testURL))
        testURL = None

      if options.immersiveMode:
        options.browserArgs.extend(('-firefoxpath', options.app))
        options.app = self.immersiveHelperPath

      if options.jsdebugger:
        options.browserArgs.extend(['-jsdebugger'])

      
      
      if os.path.exists(self.leak_report_file):
        os.remove(self.leak_report_file)

      
      if options.timeout:
        timeout = options.timeout + 30
      elif options.debugger or not options.autorun:
        timeout = None
      else:
        timeout = 330.0 

      if options.vmwareRecording:
        self.startVMwareRecording(options);

      log.info("runtests.py | Running tests: start.\n")
      try:
        status = self.runApp(testURL,
                             browserEnv,
                             options.app,
                             profile=self.profile,
                             extraArgs=options.browserArgs,
                             utilityPath=options.utilityPath,
                             debuggerInfo=debuggerInfo,
                             symbolsPath=options.symbolsPath,
                             timeout=timeout,
                             onLaunch=onLaunch,
                             webapprtChrome=options.webapprtChrome,
                             hide_subtests=options.hide_subtests,
                             screenshotOnFail=options.screenshotOnFail
        )
      except KeyboardInterrupt:
        log.info("runtests.py | Received keyboard interrupt.\n");
        status = -1
      except:
        traceback.print_exc()
        log.error("Automation Error: Received unexpected exception while running application\n")
        status = 1

    finally:
      if options.vmwareRecording:
        self.stopVMwareRecording();
      self.stopServers()

    processLeakLog(self.leak_report_file, options.leakThreshold)

    if self.nsprLogs:
      with zipfile.ZipFile("%s/nsprlog.zip" % browserEnv["MOZ_UPLOAD_DIR"], "w", zipfile.ZIP_DEFLATED) as logzip:
        for logfile in glob.glob("%s/nspr*.log*" % tempfile.gettempdir()):
          logzip.write(logfile)
          os.remove(logfile)

    log.info("runtests.py | Running tests: end.")

    if manifest is not None:
      self.cleanup(manifest, options)

    return status

  def handleTimeout(self, timeout, proc, utilityPath, debuggerInfo, browserProcessId):
    """handle process output timeout"""
    
    log.info("TEST-UNEXPECTED-FAIL | %s | application timed out after %d seconds with no output", self.lastTestSeen, int(timeout))
    browserProcessId = browserProcessId or proc.pid
    self.killAndGetStack(browserProcessId, utilityPath, debuggerInfo, dump_screen=not debuggerInfo)

  

  class OutputHandler(object):
    """line output handler for mozrunner"""
    def __init__(self, harness, utilityPath, symbolsPath=None, dump_screen_on_timeout=True, dump_screen_on_fail=False,
                 hide_subtests=False, shutdownLeaks=None):
      """
      harness -- harness instance
      dump_screen_on_timeout -- whether to dump the screen on timeout
      """
      self.harness = harness
      self.output_buffer = []
      self.running_test = False
      self.utilityPath = utilityPath
      self.symbolsPath = symbolsPath
      self.dump_screen_on_timeout = dump_screen_on_timeout
      self.dump_screen_on_fail = dump_screen_on_fail
      self.hide_subtests = hide_subtests
      self.shutdownLeaks = shutdownLeaks

      
      self.perl = which('perl')

      
      
      
      self.browserProcessId = None

      
      self.stackFixerFunction, self.stackFixerProcess = self.stackFixer()

    def processOutputLine(self, line):
      """per line handler of output for mozprocess"""
      for handler in self.outputHandlers():
        line = handler(line)
    __call__ = processOutputLine

    def outputHandlers(self):
      """returns ordered list of output handlers"""
      return [self.fix_stack,
              self.format,
              self.dumpScreenOnTimeout,
              self.dumpScreenOnFail,
              self.metro_subprocess_id,
              self.trackShutdownLeaks,
              self.check_test_failure,
              self.log,
              self.record_last_test,
              ]

    def stackFixer(self):
      """
      return 2-tuple, (stackFixerFunction, StackFixerProcess),
      if any, to use on the output lines
      """

      if not mozinfo.info.get('debug'):
        return None, None

      stackFixerFunction = stackFixerProcess = None

      def import_stackFixerModule(module_name):
        sys.path.insert(0, self.utilityPath)
        module = __import__(module_name, globals(), locals(), [])
        sys.path.pop(0)
        return module

      if self.symbolsPath and os.path.exists(self.symbolsPath):
        
        
        stackFixerModule = import_stackFixerModule('fix_stack_using_bpsyms')
        stackFixerFunction = lambda line: stackFixerModule.fixSymbols(line, self.symbolsPath)

      elif mozinfo.isLinux and self.perl:
        
        
        stackFixerCommand = [self.perl, os.path.join(self.utilityPath, "fix-linux-stack.pl")]
        stackFixerProcess = subprocess.Popen(stackFixerCommand, stdin=subprocess.PIPE,
                                             stdout=subprocess.PIPE)
        def fixFunc(line):
          stackFixerProcess.stdin.write(line + '\n')
          return stackFixerProcess.stdout.readline().rstrip()

        stackFixerFunction = fixFunc

      return (stackFixerFunction, stackFixerProcess)

    def finish(self, didTimeout):
      if self.stackFixerProcess:
        self.stackFixerProcess.communicate()
        status = self.stackFixerProcess.returncode
        if status and not didTimeout:
          log.info("TEST-UNEXPECTED-FAIL | runtests.py | Stack fixer process exited with code %d during test run", status)

      if self.shutdownLeaks:
        self.shutdownLeaks.process()

    def log_output_buffer(self):
        if self.output_buffer:
            lines = ['  %s' % line for line in self.output_buffer]
            log.info("Buffered test output:\n%s" % '\n'.join(lines))

    
    

    def fix_stack(self, line):
      if self.stackFixerFunction:
        return self.stackFixerFunction(line)
      return line

    def format(self, line):
      """format the line"""
      return line.rstrip().decode("UTF-8", "ignore")

    def dumpScreenOnTimeout(self, line):
      if not self.dump_screen_on_fail and self.dump_screen_on_timeout and "TEST-UNEXPECTED-FAIL" in line and "Test timed out" in line:
        self.log_output_buffer()
        self.harness.dumpScreen(self.utilityPath)
      return line

    def dumpScreenOnFail(self, line):
      if self.dump_screen_on_fail and "TEST-UNEXPECTED-FAIL" in line:
        self.log_output_buffer()
        self.harness.dumpScreen(self.utilityPath)
      return line

    def metro_subprocess_id(self, line):
      """look for metro browser subprocess id"""
      if "METRO_BROWSER_PROCESS" in line:
        index = line.find("=")
        if index != -1:
          self.browserProcessId = line[index+1:].rstrip()
          log.info("INFO | runtests.py | metro browser sub process id detected: %s", self.browserProcessId)
      return line

    def trackShutdownLeaks(self, line):
      if self.shutdownLeaks:
        self.shutdownLeaks.log(line)
      return line

    def check_test_failure(self, line):
      if 'TEST-END' in line:
        self.running_test = False
        if any('TEST-UNEXPECTED' in l for l in self.output_buffer):
          self.log_output_buffer()
      return line

    def log(self, line):
      if self.hide_subtests and self.running_test:
        self.output_buffer.append(line)
      else:
        
        if self.hide_subtests and 'TEST-END' in line:
            index = line.index('TEST-END') + len('TEST-END')
            line = line[:index] + ' ' * (len('TEST-START')-len('TEST-END')) + line[index:]
        log.info(line)
      return line

    def record_last_test(self, line):
      """record last test on harness"""
      if "TEST-START" in line and "|" in line:
        if not line.endswith('Shutdown'):
          self.output_buffer = []
          self.running_test = True
        self.harness.lastTestSeen = line.split("|")[1].strip()
      return line


  def makeTestConfig(self, options):
    "Creates a test configuration file for customizing test execution."
    options.logFile = options.logFile.replace("\\", "\\\\")
    options.testPath = options.testPath.replace("\\", "\\\\")
    testRoot = self.getTestRoot(options)

    if "MOZ_HIDE_RESULTS_TABLE" in os.environ and os.environ["MOZ_HIDE_RESULTS_TABLE"] == "1":
      options.hideResultsTable = True

    d = dict(options.__dict__)
    d['testRoot'] = testRoot
    content = json.dumps(d)

    with open(os.path.join(options.profilePath, "testConfig.js"), "w") as config:
      config.write(content)

  def installExtensionFromPath(self, options, path, extensionID = None):
    """install an extension to options.profilePath"""

    
    
    
    

    extensionPath = self.getFullPath(path)

    log.info("runtests.py | Installing extension at %s to %s." %
                (extensionPath, options.profilePath))

    addons = AddonManager(options.profilePath)

    
    
    
    
    
    if hasattr(addons, '__del__'):
      del addons.__del__

    addons.install_from_path(path)

  def installExtensionsToProfile(self, options):
    "Install special testing extensions, application distributed extensions, and specified on the command line ones to testing profile."
    for path in self.getExtensionsToInstall(options):
      self.installExtensionFromPath(options, path)


def main():

  
  mochitest = Mochitest()
  parser = MochitestOptions()
  options, args = parser.parse_args()
  options = parser.verifyOptions(options, mochitest)
  if options is None:
    
    sys.exit(1)

  options.utilityPath = mochitest.getFullPath(options.utilityPath)
  options.certPath = mochitest.getFullPath(options.certPath)
  if options.symbolsPath and not isURL(options.symbolsPath):
    options.symbolsPath = mochitest.getFullPath(options.symbolsPath)

  sys.exit(mochitest.runTests(options))

if __name__ == "__main__":
  main()

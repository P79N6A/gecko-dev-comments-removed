







































import SimpleHTTPServer
import SocketServer
import socket
import threading
import os
import sys
import shutil
from datetime import datetime

SCRIPT_DIR = os.path.abspath(os.path.realpath(os.path.dirname(sys.argv[0])))
sys.path.insert(0, SCRIPT_DIR)
from automation import Automation
from automationutils import getDebuggerInfo, addCommonOptions

PORT = 8888
PROFILE_DIRECTORY = os.path.abspath(os.path.join(SCRIPT_DIR, "./pgoprofile"))
MOZ_JAR_LOG_DIR = os.path.abspath(os.getenv("JARLOG_DIR"))
os.chdir(SCRIPT_DIR)

class EasyServer(SocketServer.TCPServer):
  allow_reuse_address = True

if __name__ == '__main__':
  from optparse import OptionParser
  automation = Automation()

  parser = OptionParser(usage='OBJDIR=path/to/objdir python %prog [NUM_RUNS]')
  addCommonOptions(parser)

  options, args = parser.parse_args()

  if not os.getenv('OBJDIR'):
      parser.error('Please specify the OBJDIR environment variable.')

  if not args:
      num_runs = 1
  else:
      try:
          num_runs = int(args[0])
      except:
          parser.error('NUM_RUNS argument must be an integer.')
      if num_runs < 1:
          parser.error('NUM_RUNS must be greater than zero.')

  debuggerInfo = getDebuggerInfo(".", options.debugger, options.debuggerArgs,
          options.debuggerInteractive)

  httpd = EasyServer(("", PORT), SimpleHTTPServer.SimpleHTTPRequestHandler)
  t = threading.Thread(target=httpd.serve_forever)
  t.setDaemon(True) 
  t.start()
  
  automation.setServerInfo("localhost", PORT)
  automation.initializeProfile(PROFILE_DIRECTORY)
  browserEnv = automation.environment()
  browserEnv["XPCOM_DEBUG_BREAK"] = "warn"
  browserEnv["MOZ_JAR_LOG_DIR"] = MOZ_JAR_LOG_DIR

  url = "http://localhost:%d/index.html" % PORT
  appPath = os.path.join(SCRIPT_DIR, automation.DEFAULT_APP)

  for i in range(0, num_runs):
      if num_runs != 1:
          print "Starting profiling run %d of %d" % (i + 1, num_runs)
      status = automation.runApp(url, browserEnv, appPath, PROFILE_DIRECTORY, {},
                                 debuggerInfo=debuggerInfo,
                                 
                                 
                                 timeout = None)
      if status != 0:
          sys.exit(status)

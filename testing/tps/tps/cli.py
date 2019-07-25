




































import json
import logging
import optparse
import os
import sys
import time
import traceback

from threading import RLock

from tps import TPSFirefoxRunner, TPSPulseMonitor, TPSTestRunner

def main():
  parser = optparse.OptionParser()
  parser.add_option("--email-results",
                    action = "store_true", dest = "emailresults",
                    default = False,
                    help = "email the test results to the recipients defined "
                           "in the config file")
  parser.add_option("--mobile",
                    action = "store_true", dest = "mobile",
                    default = False,
                    help = "run with mobile settings")
  parser.add_option("--autolog",
                    action = "store_true", dest = "autolog",
                    default = False,
                    help = "post results to Autolog")
  parser.add_option("--testfile",
                    action = "store", type = "string", dest = "testfile",
                    default = '../../services/sync/tests/tps/test_sync.js',
                    help = "path to the test file to run "
                           "[default: %default]")
  parser.add_option("--logfile",
                    action = "store", type = "string", dest = "logfile",
                    default = 'tps.log',
                    help = "path to the log file [default: %default]")
  parser.add_option("--binary",
                    action = "store", type = "string", dest = "binary",
                    default = None,
                    help = "path to the Firefox binary, specified either as "
                           "a local file or a url; if omitted, the PATH "
                           "will be searched;")
  parser.add_option("--configfile",
                    action = "store", type = "string", dest = "configfile",
                    default = None,
                    help = "path to the config file to use "
                           "[default: %default]")
  parser.add_option("--pulsefile",
                    action = "store", type = "string", dest = "pulsefile",
                    default = None,
                    help = "path to file containing a pulse message in "
                           "json format that you want to inject into the monitor")
  (options, args) = parser.parse_args()

  configfile = options.configfile
  if configfile is None:
    if os.environ.get('VIRTUAL_ENV'):
      configfile = os.path.join(os.path.dirname(__file__), 'config.json')
    if configfile is None or not os.access(configfile, os.F_OK):
      raise Exception("Unable to find config.json in a VIRTUAL_ENV; you must "
                      "specify a config file using the --configfile option")

  
  f = open(configfile, 'r')
  configcontent = f.read()
  f.close()
  config = json.loads(configcontent)

  rlock = RLock()
 
  extensionDir = config.get("extensiondir")
  if not extensionDir or extensionDir == '__EXTENSIONDIR__':
    extensionDir = os.path.join(os.getcwd(), "..", "..", "services", "sync", "tps")
  else:
    if sys.platform == 'win32':
      
      import re
      m = re.match('^\/\w\/', extensionDir)
      if m:
        extensionDir = "%s:/%s" % (m.group(0)[1:2], extensionDir[3:])
        extensionDir = extensionDir.replace("/", "\\")

  if options.binary is None:
    while True:
      try:
        
        
        monitor = TPSPulseMonitor(extensionDir,
                                  config=config,
                                  autolog=options.autolog,
                                  emailresults=options.emailresults,
                                  testfile=options.testfile,
                                  logfile=options.logfile,
                                  rlock=rlock)
        print "waiting for pulse build notifications"

        if options.pulsefile:
          
          
          builddata = json.loads(open(options.pulsefile, 'r').read())
          monitor.onBuildComplete(builddata)

        monitor.listen()
      except KeyboardInterrupt:
        sys.exit()
      except:
        traceback.print_exc()
        print 'sleeping 5 minutes'
        time.sleep(300)

  TPS = TPSTestRunner(extensionDir,
                      emailresults=options.emailresults,
                      testfile=options.testfile,
                      logfile=options.logfile,
                      binary=options.binary,
                      config=config,
                      rlock=rlock,
                      mobile=options.mobile,
                      autolog=options.autolog)
  TPS.run_tests()

if __name__ == "__main__":
  main()

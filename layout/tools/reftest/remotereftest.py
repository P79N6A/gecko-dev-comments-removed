




































import sys
import os
import time

SCRIPT_DIRECTORY = os.path.abspath(os.path.realpath(os.path.dirname(sys.argv[0])))
sys.path.append(SCRIPT_DIRECTORY)


from runreftest import RefTest
from runreftest import ReftestOptions
from automation import Automation
from devicemanager import DeviceManager
from remoteautomation import RemoteAutomation

class RemoteOptions(ReftestOptions):
    def __init__(self, automation):
        ReftestOptions.__init__(self, automation)

        defaults = {}
        defaults["logFile"] = "reftest.log"
        
        defaults["remoteTestRoot"] = None
        defaults["app"] = ""
        defaults["xrePath"] = ""
        defaults["utilityPath"] = ""

        self.add_option("--remote-app-path", action="store",
                    type = "string", dest = "remoteAppPath",
                    help = "Path to remote executable relative to device root using only forward slashes.  Either this or app must be specified, but not both.")
        defaults["remoteAppPath"] = None

        self.add_option("--deviceIP", action="store",
                    type = "string", dest = "deviceIP",
                    help = "ip address of remote device to test")
        defaults["deviceIP"] = None

        self.add_option("--devicePort", action="store",
                    type = "string", dest = "devicePort",
                    help = "port of remote device to test")
        defaults["devicePort"] = 20701

        self.add_option("--remote-product-name", action="store",
                    type = "string", dest = "remoteProductName",
                    help = "Name of product to test - either fennec or firefox, defaults to fennec")
        defaults["remoteProductName"] = "fennec"

        self.add_option("--remote-webserver", action="store",
                    type = "string", dest = "remoteWebServer",
                    help = "IP Address of the webserver hosting the reftest content")
        defaults["remoteWebServer"] = None

        self.add_option("--http-port", action = "store",
                    type = "string", dest = "httpPort",
                    help = "port of the web server for http traffic")
        defaults["httpPort"] = automation.DEFAULT_HTTP_PORT

        self.add_option("--ssl-port", action = "store",
                    type = "string", dest = "sslPort",
                    help = "Port for https traffic to the web server")
        defaults["sslPort"] = automation.DEFAULT_SSL_PORT

        self.add_option("--remote-logfile", action="store",
                    type = "string", dest = "remoteLogFile",
                    help = "Name of log file on the device relative to device root.  PLEASE USE ONLY A FILENAME.")
        defaults["remoteLogFile"] = "reftest.log"

        self.set_defaults(**defaults)

    def verifyRemoteOptions(self, options):
        
        options.remoteTestRoot = self._automation._devicemanager.getDeviceRoot() + '/reftest'
        options.remoteProfile = options.remoteTestRoot + "/profile"

        
        
        
        if (options.remoteAppPath and options.app):
            print "ERROR: You cannot specify both the remoteAppPath and the app"
            return None
        elif (options.remoteAppPath):
            options.app = options.remoteTestRoot + "/" + options.remoteAppPath
        elif (options.app == None):
            
            print "ERROR: You must specify either appPath or app"
            return None
        
        if (options.xrePath == None):
            print "ERROR: You must specify the path to the controller xre directory"
            return None

        
        
        
        return options

class RemoteReftest(RefTest):
    remoteApp = ''

    def __init__(self, automation, devicemanager, options, scriptDir):
        RefTest.__init__(self, automation)
        self._devicemanager = devicemanager
        self.scriptDir = scriptDir
        self.remoteApp = options.app
        self.remoteTestRoot = options.remoteTestRoot

    def createReftestProfile(self, options, profileDir):
        RefTest.createReftestProfile(self, options, profileDir)

        if (self._devicemanager.pushDir(profileDir, options.remoteProfile) == None):
            raise devicemanager.FileError("Failed to copy profiledir to device")

    def copyExtraFilesToProfile(self, options, profileDir):
        RefTest.copyExtraFilesToProfile(self, options, profileDir)
        if (self._devicemanager.pushDir(profileDir, options.remoteProfile) == None):
            raise devicemanager.FileError("Failed to copy extra files to device") 

    def registerExtension(self, browserEnv, options, profileDir, extraArgs = ['-silent'] ):
        self.automation.log.info("REFTEST INFO | runreftest.py | Performing extension manager registration: start.\n")
        
        
        
        status = self.automation.runApp(None, browserEnv, options.app, profileDir,
                                   extraArgs,
                                   utilityPath = options.utilityPath,
                                   xrePath=options.xrePath,
                                   symbolsPath=options.symbolsPath,
                                   maxTime = 20)
        
        self.automation.log.info("\nREFTEST INFO | runreftest.py | Performing extension manager registration: end.")

    def getManifestPath(self, path):
        return path

    def cleanup(self, profileDir):
        self._devicemanager.removeDir(self.remoteProfileDir)
        self._devicemanager.removeDir(self.remoteTestRoot)
        RefTest.cleanup(self, profileDir)

def main():
    dm = DeviceManager(None, None)
    automation = RemoteAutomation(dm)
    parser = RemoteOptions(automation)
    options, args = parser.parse_args()

    if (options.deviceIP == None):
        print "Error: you must provide a device IP to connect to via the --device option"
        sys.exit(1)

    dm = DeviceManager(options.deviceIP, options.devicePort)
    automation.setDeviceManager(dm)

    if (options.remoteProductName != None):
        automation.setProduct(options.remoteProductName)

    
    options = parser.verifyRemoteOptions(options)
    if (options == None):
        print "ERROR: Invalid options specified, use --help for a list of valid options"
        sys.exit(1)

    automation.setAppName(options.app)
    automation.setRemoteProfile(options.remoteProfile)
    reftest = RemoteReftest(automation, dm, options, SCRIPT_DIRECTORY)

    if (options.remoteWebServer == "127.0.0.1"):
        print "Error: remoteWebServer must be non localhost"
        sys.exit(1)



    reftest.runTests(args[0], options)

if __name__ == "__main__":
    main()

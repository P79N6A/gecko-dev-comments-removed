




































from mozrunner import FirefoxRunner
from mozprofile import FirefoxProfile
from optparse import OptionParser
from ConfigParser import ConfigParser, NoOptionError, NoSectionError
from time import sleep
import logging
import urllib2
import os, sys, platform

class FBRunner:
    def __init__(self, **kwargs):    
        
        logLevel = logging.DEBUG if kwargs["debug"] else logging.INFO
        filename = kwargs["log"]
        self.log = logging.getLogger("FIREBUG")
        if filename:
            dirname = os.path.dirname(filename)
            if dirname and not os.path.exists(dirname):
                os.makedirs(dirname)
            handler = logging.FileHandler(filename)
            format = "%(asctime)s - %(name)s %(levelname)s | %(message)s"
        else:
            handler = logging.StreamHandler()
            format = "%(name)s %(levelname)s | %(message)s"
        handler.setFormatter(logging.Formatter(format))
        self.log.addHandler(handler)
        self.log.setLevel(logLevel)
            
        
        self.binary = kwargs["binary"]
        self.profile = kwargs["profile"]
        self.serverpath = kwargs["serverpath"]
        self.version = kwargs["version"]
        self.testlist = kwargs["testlist"]
        self.platform = platform.system().lower()
        
        
        
        
        self.TEST_TIMEOUT = 60        
        
        
        self.appVersion = ""
        if self.binary:
            app = ConfigParser()
            app.read(os.path.join(os.path.dirname(self.binary), "application.ini"))
            ver = app.get("App", "Version").rstrip("0123456789pre")    
            self.appVersion = ver[:-1] if ver[-1]=="." else ver

        
        localConfig = ConfigParser()
        localConfig.read("fb-test-runner.config")
        if not self.serverpath:
            self.serverpath = localConfig.get("runner_args", "server")
        
        
        self.serverpath = self.serverpath.rstrip("/") + "/"

        
        if not self.version:
            try:
                self.version = localConfig.get("version_map", self.appVersion)
            except NoOptionError:
                self.version = localConfig.get("version_map", "default")
                self.log.warning("Could not find an appropriate version of Firebug to use, using Firebug " + self.version)

        
        try:
            self.download(self.serverpath + "releases/firebug/test-bot.config", "test-bot.config")
        except urllib2.URLError:
            self.log.error("Could not download test-bot.config, check that '" + self.serverpath + "releases/firebug/test-bot.config' is valid")
            raise
        self.config = ConfigParser()
        self.config.read("test-bot.config")
        
        
        if not self.testlist:
            self.testlist = self.config.get("Firebug"+self.version, "TEST_LIST")

    def cleanup(self):
        """
        Remove temporarily downloaded files
        """
        try:
            for tmpFile in ["firebug.xpi", "fbtest.xpi", "test-bot.config"]:
                if os.path.exists(tmpFile):
                    self.log.debug("Removing " + tmpFile)
                    os.remove(tmpFile)
        except Exception, e:
            self.log.warn("Could not clean up temporary files: " + str(e))
            
    def download(self, url, savepath):
        """
        Save the file located at 'url' into 'filename'
        """
        self.log.debug("Downloading '" + url + "' to '" + savepath + "'")
        ret = urllib2.urlopen(url)
        savedir = os.path.dirname(savepath)
        if savedir and not os.path.exists(savedir):
            os.makedirs(savedir)
        outfile = open(savepath, 'wb')
        outfile.write(ret.read())
        outfile.close()
        
    def get_extensions(self):
        """
        Downloads the firebug and fbtest extensions
        for the specified Firebug version
        """
        self.log.debug("Downloading firebug and fbtest extensions from server")
        FIREBUG_XPI = self.config.get("Firebug" + self.version, "FIREBUG_XPI")
        FBTEST_XPI = self.config.get("Firebug" + self.version, "FBTEST_XPI")
        self.download(FIREBUG_XPI, "firebug.xpi")
        self.download(FBTEST_XPI, "fbtest.xpi")

    def disable_compatibilityCheck(self):
        """
        Disables compatibility check which could
        potentially prompt the user for action
        """
        self.log.debug("Disabling compatibility check")
        try:
            prefs = open(os.path.join(self.profile, "prefs.js"), "a")
            prefs.write("user_pref(\"extensions.checkCompatibility." + self.appVersion + "\", false);\n")
            prefs.close()
        except Exception, e:
            self.log.warn("Could not disable compatibility check: " + str(e))

    def run(self):
        """
        Code for running the tests
        """
        if self.profile:
            
            if not os.path.exists(self.profile):
                self.log.warn("Profile '" + self.profile + "' doesn't exist.  Creating temporary profile")
                self.profile = None
            else:
                
                if os.path.exists(os.path.join(self.profile, "firebug/fbtest/logs")):
                    self.log.debug("Moving existing log files to archive")
                    for name in os.listdir(os.path.join(self.profile, "firebug/fbtest/logs")):
                        os.rename(os.path.join(self.profile, "firebug/fbtest/logs", name), os.path.join(self.profile, "firebug/fbtest/logs_old", name))

        
        try:
            self.get_extensions()
        except (NoSectionError, NoOptionError), e:            
            self.log.error("Extensions could not be downloaded, malformed test-bot.config: " + str(e))
            self.cleanup()
            raise
        except urllib2.URLError, e:
            self.log.error("Extensions could not be downloaded, urllib2 error: " + str(e))
            self.cleanup()
            raise
    
        
        mozEnv = os.environ
        mozEnv["XPC_DEBUG_WARN"] = "warn"                
        mozEnv["MOZ_CRASHREPORTER_NO_REPORT"] = "true"   

        
        self.log.info("Starting Firebug Tests")
        try:
            self.log.debug("Creating Firefox profile and installing extensions")
            mozProfile = FirefoxProfile(profile=self.profile, addons=["firebug.xpi", "fbtest.xpi"])
            self.profile = mozProfile.profile
            
            
            if self.binary:
                self.disable_compatibilityCheck()
            else:
                self.log.warn("Can't disable compatibility check because binary wasn't specified")

            self.log.debug("Running Firefox with cmdargs '-runFBTests " + self.testlist + "'")
            mozRunner = FirefoxRunner(profile=mozProfile, binary=self.binary, cmdargs=["-runFBTests", self.testlist], env=mozEnv)         
            mozRunner.start()
        except Exception, e:
            self.log.error("Could not start Firefox: " + str(e))
            self.cleanup()
            raise

        
        timeout, logfile = 0, 0
        
        while not logfile and timeout < 60:
            try:
                for name in os.listdir(os.path.join(self.profile, "firebug/fbtest/logs")):
                    logfile = open(os.path.join(self.profile, "firebug/fbtest/logs/", name))
            except OSError:
                timeout += 1
                sleep(1)
                
        
        if not logfile:
            self.log.error("Could not find the log file in profile '" + self.profile + "'")
            self.cleanup()
            raise
        
        else:
            line, timeout = "", 0
            while timeout < self.TEST_TIMEOUT:
                line = logfile.readline()
                if line == "":
                    sleep(1)
                    timeout += 1
                else:
                    print line.rstrip()
                    if line.find("Test Suite Finished") != -1:
                        break
                    timeout = 0
        
        
        if timeout >= self.TEST_TIMEOUT:
            logfile.seek(1)
            line = logfile.readlines()[-1]
            if line.find("FIREBUG INFO") != -1:
                line = line[line.find("|") + 1:].lstrip()   
                line = line[:line.find("|")].rstrip()
            else:
                line = "Unknown Test"
            print "FIREBUG TEST-UNEXPECTED-FAIL | " + line + " | Possible Firefox crash detected"       
            self.log.warn("Possible crash detected - test run aborted")
            
        
        logfile.close()
        mozRunner.stop()
        self.cleanup()
        self.log.debug("Exiting - Status successful")



def cli(argv=sys.argv[1:]):
    parser = OptionParser("usage: %prog [options]")
    parser.add_option("--appname", dest="binary",
                      help="Firefox binary path")
                    
    parser.add_option("--profile-path", dest="profile",
                      help="The profile to use when running Firefox")
                        
    parser.add_option("-s", "--serverpath", dest="serverpath", 
                      help="The http server containing the Firebug tests")
                        
    parser.add_option("-v", "--version", dest="version",
                      help="The firebug version to run")
                        
    parser.add_option("-t", "--testlist", dest="testlist",
                      help="Specify the name of the testlist to use, should usually use the default")
                      
    parser.add_option("--log", dest="log",
                      help="Path to the log file (default is stdout)")
                      
    parser.add_option("--debug", dest="debug",
                      action="store_true",
                      help="Enable debug logging")
    (opt, remainder) = parser.parse_args(argv)
    
    try:
        runner = FBRunner(binary=opt.binary, profile=opt.profile, serverpath=opt.serverpath, 
                                    version=opt.version, testlist=opt.testlist, log=opt.log, debug=opt.debug)
        runner.run()
    except Exception:
        return -1

if __name__ == '__main__':
	sys.exit(cli())

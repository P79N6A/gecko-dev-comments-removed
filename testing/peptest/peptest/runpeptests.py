



































from optparse import OptionParser
from mozprofile import FirefoxProfile, ThunderbirdProfile, Profile
from mozrunner import FirefoxRunner, ThunderbirdRunner, Runner
from mozhttpd import MozHttpd
from manifestparser import TestManifest
from pepprocess import PepProcess
from pepresults import Results

import peputils as utils
import traceback
import mozlog
import glob
import shutil
import os
import sys

results = Results()
here = os.path.dirname(os.path.realpath(__file__))


class Peptest():
    """
    Peptest
    Runs and logs tests designed to test responsiveness
    """
    profile_class = Profile
    runner_class = Runner

    def __init__(self, options, **kwargs):
        self.options = options
        self.server = None
        self.logger = mozlog.getLogger('PEP')

        
        self.profile = self.profile_class(profile=self.options.profilePath,
                                          addons=[os.path.join(here, 'extension')])

        
        if self.options.serverPath:
            self.runServer()

        tests = []
        
        if self.options.testPath.endswith('.js'):
            
            testObj = {}
            testObj['path'] = os.path.realpath(self.options.testPath)
            testObj['name'] = os.path.basename(self.options.testPath)
            tests.append(testObj)
        else:
            
            
            manifest = TestManifest()
            manifest.read(self.options.testPath)
            tests = manifest.get()

        
        manifestObj = {}
        manifestObj['tests'] = tests

        
        jsonManifest = open(os.path.join(here, 'manifest.json'), 'w')
        jsonManifest.write(str(manifestObj).replace("'", "\""))
        jsonManifest.close()

        
        env = os.environ.copy()
        env['MOZ_INSTRUMENT_EVENT_LOOP'] = '1'
        env['MOZ_INSTRUMENT_EVENT_LOOP_THRESHOLD'] = str(options.tracerThreshold)
        env['MOZ_INSTRUMENT_EVENT_LOOP_INTERVAL'] = str(options.tracerInterval)
        env['MOZ_CRASHREPORTER_NO_REPORT'] = '1'

        
        cmdargs = []
        
        cmdargs.extend(self.options.browserArgs)
        cmdargs.extend(['-pep-start', os.path.realpath(jsonManifest.name)])

        
        self.runner = self.runner_class(profile=self.profile,
                                        binary=self.options.binary,
                                        cmdargs=cmdargs,
                                        env=env,
                                        process_class=PepProcess)

    def start(self):
        self.logger.debug('Starting Peptest')

        
        self.runner.start()
        self.runner.wait(outputTimeout=self.options.timeout)
        crashed = self.checkForCrashes(results.currentTest)
        self.stop()

        if crashed or results.has_fails():
            return 1
        return 0

    def runServer(self):
        """
        Start a basic HTML server to host
        test related files.
        """
        if not self.options.serverPath:
            self.logger.warning('Can\'t start HTTP server, --server-path not specified')
            return
        self.logger.debug('Starting server on port ' + str(self.options.serverPort))
        self.server = MozHttpd(port=self.options.serverPort,
                               docroot=self.options.serverPath)
        self.server.start(block=False)

    def stop(self):
        """Kill the app"""
        
        if self.runner is not None:
            self.runner.stop()

        
        if self.server:
            self.server.stop()

        
        files = ['manifest.json']
        for f in files:
            if os.path.exists(os.path.join(here, f)):
                os.remove(os.path.join(here, f))

        
        dumpDir = os.path.join(self.profile.profile, 'minidumps')
        if self.options.profilePath and os.path.exists(dumpDir):
            shutil.rmtree(dumpDir)

    def checkForCrashes(self, testName=None):
        """
        Detects when a crash occurs and prints the output from
        MINIDUMP_STACKWALK. Returns true if crash detected,
        otherwise false.
        """
        stackwalkPath = os.environ.get('MINIDUMP_STACKWALK', None)
        
        if testName is None:
            try:
                testName = os.path.basename(sys._getframe(1).f_code.co_filename)
            except:
                testName = "unknown"

        foundCrash = False
        dumpDir = os.path.join(self.profile.profile, 'minidumps')
        dumps = glob.glob(os.path.join(dumpDir, '*.dmp'))

        symbolsPath = self.options.symbolsPath

        for d in dumps:
            import subprocess
            foundCrash = True
            self.logger.info("PROCESS-CRASH | %s | application crashed (minidump found)", testName)
            print "Crash dump filename: " + d

            
            if symbolsPath and stackwalkPath and os.path.exists(stackwalkPath):
                
                if utils.isURL(symbolsPath):
                    bundle = utils.download(symbolsPath, here)
                    symbolsPath = os.path.join(os.path.dirname(bundle), 'symbols')
                    utils.extract(bundle, symbolsPath, delete=True)

                
                p = subprocess.Popen([stackwalkPath, d, symbolsPath],
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)
                (out, err) = p.communicate()
                if len(out) > 3:
                    
                    print out
                else:
                    print "stderr from minidump_stackwalk:"
                    print err
                if p.returncode != 0:
                    print "minidump_stackwalk exited with return code %d" % p.returncode
            else:
                self.logger.warning('No symbols_path or stackwalk path specified, can\'t process dump')
                break

        
        if utils.isURL(self.options.symbolsPath):
            if os.path.exists(symbolsPath):
                shutil.rmtree(symbolsPath)
        return foundCrash

class FirefoxPeptest(Peptest):
    profile_class = FirefoxProfile
    runner_class = FirefoxRunner

class ThunderbirdPeptest(Peptest):
    profile_class = ThunderbirdProfile
    runner_class = ThunderbirdRunner

applications = {'firefox': FirefoxPeptest,
                'thunderbird': ThunderbirdPeptest}


class PeptestOptions(OptionParser):
    def __init__(self, **kwargs):
        OptionParser.__init__(self, **kwargs)
        self.add_option("-t", "--test-path",
                        action="store", type="string", dest="testPath",
                        help="path to the test manifest")

        self.add_option("-b", "--binary",
                        action="store", type="string", dest="binary",
                        help="absolute path to application, overriding default")

        self.add_option("--app",
                        action="store", type="string", dest="app",
                        default="firefox",
                        help="app to run the tests on (firefox or thunderbird). "
                             "defaults to firefox")

        self.add_option("--log-file",
                        action="store", type="string", dest="logFile",
                        metavar="FILE", default=None,
                        help="file to which logging occurs")

        self.add_option("--timeout",
                        type="int", dest="timeout",
                        default=None,
                        help="global timeout in seconds (with no output)")
        LOG_LEVELS = ("DEBUG", "INFO", "WARNING", "ERROR")
        LEVEL_STRING = ", ".join(LOG_LEVELS)
        self.add_option("--log-level",
                        action="store", type="choice", dest="logLevel",
                        choices=LOG_LEVELS, metavar="LEVEL",
                        default=None,
                        help="one of %s to determine the level of logging"
                             "logging" % LEVEL_STRING)

        self.add_option("--setenv",
                        action="append", type="string", dest="environment",
                        metavar="NAME=VALUE", default=[],
                        help="sets the given variable in the application's "
                             "environment")

        self.add_option("--browser-arg",
                        action="append",  type="string", dest="browserArgs",
                        metavar="ARG", default=[],
                        help="provides an argument to the test application")

        self.add_option("--leak-threshold",
                        action="store", type="int", dest="leakThreshold",
                        metavar="THRESHOLD", default=0,
                        help="fail if the number of bytes leaked through "
                             "refcounted objects (or bytes in classes with "
                             "MOZ_COUNT_CTOR and MOZ_COUNT_DTOR) is greater "
                             "than the given number")

        self.add_option("--fatal-assertions",
                        action="store_true", dest="fatalAssertions",
                        default=False,
                        help="abort testing whenever an assertion is hit "
                             "(requires a debug build to be effective)")

        self.add_option("-p", "--profile-path", action="store",
                        type="string", dest="profilePath",
                        default=None,
                        help="path to the profile to use. "
                             "If none specified, a temporary profile is created")

        self.add_option("--server-port",
                        action="store", type="int", dest="serverPort",
                        default=8888,
                        help="The port to host test related files on")

        self.add_option("--server-path",
                        action="store", type="string", dest="serverPath",
                        default=None,
                        help="Starts a basic HTTP server rooted at the specified "
                             "directory. Can be used for hosting test related files")

        self.add_option("--symbols-path",
                        action = "store", type = "string", dest = "symbolsPath",
                        default = None,
                        help = "absolute path to directory containing breakpad symbols, "
                               "or the URL of a zip file containing symbols")

        self.add_option("--tracer-threshold",
                        action="store", type="int", dest="tracerThreshold",
                        default=50,
                        help="time in milliseconds at which point an event is "
                             "considered unresponsive. Default to 50ms")

        self.add_option("--tracer-interval",
                        action="store", type="int", dest="tracerInterval",
                        default=10,
                        help="interval in milliseconds that tracer events are "
                             "sent through the event loop. Default to 10ms")

        usage = """
                Usage instructions for runtests.py.
                %prog [options]
                All arguments except --test-path are optional.
                """

        self.set_usage(usage)

    def verifyOptions(self, options):
        """ verify correct options and cleanup paths """
        
        if not options.testPath:
            print "error: --test-path must specify the path to a test or test manifest"
            return None
        return options


def main(args=sys.argv[1:]):
    """
    Return codes
    0 - success
    1 - test failures
    2 - fatal error
    """
    parser = PeptestOptions()
    options, args = parser.parse_args()
    options = parser.verifyOptions(options)
    if options == None:
        return 2

    
    logger = mozlog.getLogger('PEP', options.logFile)
    if options.logLevel:
        logger.setLevel(getattr(mozlog, options.logLevel, 'INFO'))

    try:
        peptest = applications[options.app](options)
        return peptest.start()
    except Exception:
        cla, exc = sys.exc_info()[:2]
        logger.error("%s: %s" % (cla.__name__, exc))
        logger.debug("Traceback:\n%s" % (traceback.format_exc()))
        return 2

if __name__ == '__main__':
    sys.exit(main())

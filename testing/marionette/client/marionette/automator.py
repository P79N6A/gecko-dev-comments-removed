







import ConfigParser
import json
import logging
import os
import sys
import traceback
import urllib
import mozlog
import shutil
from optparse import OptionParser
from threading import Thread, RLock
from manifestparser import TestManifest
from runtests import MarionetteTestRunner
from marionette import Marionette
from mozinstall import install

from mozillapulse.config import PulseConfiguration
from mozillapulse.consumers import GenericConsumer


class B2GPulseConsumer(GenericConsumer):
    def __init__(self, **kwargs):
        super(B2GPulseConsumer, self).__init__(PulseConfiguration(**kwargs),
                                               'org.mozilla.exchange.b2g',
                                               **kwargs)


class B2GAutomation:
    def __init__(self, tests, testfile=None,
                 es_server=None, rest_server=None, testgroup='marionette'):
        self.logger = mozlog.getLogger('B2G_AUTOMATION')
        self.tests = tests
        self.testfile = testfile
        self.es_server = es_server
        self.rest_server = rest_server
        self.testgroup = testgroup
        self.lock = RLock()

        self.logger.info("Testlist: %s" % self.tests)

        pulse = B2GPulseConsumer(applabel='b2g_build_listener')
        pulse.configure(topic='#', callback=self.on_build)

        if not self.testfile:
            self.logger.info('waiting for pulse messages...')
            pulse.listen()
        else:
            t = Thread(target=pulse.listen)
            t.daemon = True
            t.start()
            f = open(self.testfile, 'r')
            data = json.loads(f.read())
            self.on_build(data, None)

    def get_test_list(self, manifest):
        self.logger.info("Reading test manifest: %s" % manifest)
        mft = TestManifest()
        mft.read(manifest)

        
        
        
        testlist = []
        for i in mft.active_tests(exists=False, disabled=False):
            testlist.append(i["path"])

        return testlist

    def on_build(self, data, msg):
        
        if msg is not None:
            msg.ack()
        self.lock.acquire()

        try:
            self.logger.info("got pulse message! %s" % repr(data))
            if "buildurl" in data["payload"]:
                directory = self.install_build(data['payload']['buildurl'])
                rev = data["payload"]["commit"]
                if directory == None:
                    self.logger.info("Failed to return build directory")
                else:
                    self.run_marionette(directory, rev)
                    self.cleanup(directory)
            else:
                self.logger.error("Failed to find buildurl in msg, not running test")

        except:
            self.logger.exception("error while processing build")

        self.lock.release()

    
    def install_build(self, url):
        try:
            self.logger.info("Installing build from url: %s" % url)
            buildfile = os.path.abspath("b2gtarball.tar.gz")
            urllib.urlretrieve(url, buildfile)
        except:
            self.logger.exception("Failed to download build")

        try:
            self.logger.info("Untarring build")
            
            
            dest = os.path.join(os.path.dirname(buildfile), 'downloadedbuild')
            if (os.access(dest, os.F_OK)):
                shutil.rmtree(dest)
            install(buildfile, dest)
            
            qemu = os.path.join(dest, 'qemu')
            if os.path.exists(qemu):
                return qemu
            else:
                return None
        except:
            self.logger.exception("Failed to untar file")
        return None

    def run_marionette(self, dir, rev):
        self.logger.info("Starting test run for revision: %s" % rev)
        runner = MarionetteTestRunner(emulator=True,
                                      homedir=dir,
                                      autolog=True,
                                      revision=rev,
                                      logger=self.logger,
                                      es_server=self.es_server,
                                      rest_server=self.rest_server,
                                      testgroup=self.testgroup)
        for test in self.tests:
            manifest = test[1].replace('$homedir$', os.path.dirname(dir))
            testgroup = test[0]
            runner.testgroup = testgroup
            runner.run_tests([manifest], 'b2g')

    def cleanup(self, dir):
        self.logger.info("Cleaning up")
        if os.path.exists("b2gtarball.tar.gz"):
            os.remove("b2gtarball.tar.gz")
        if os.path.exists(dir):
            shutil.rmtree(dir)

def main():
    parser = OptionParser(usage="%prog <options>")
    parser.add_option("--config", action="store", dest="config_file",
                      default="automation.conf",
                      help="Specify the configuration file")
    parser.add_option("--testfile", action="store", dest="testfile",
                      help = "Start in test mode without using pulse, "
                      "utilizing the pulse message defined in the specified file")
    parser.add_option("--test-manifest", action="store", dest="testmanifest",
                      default = os.path.join("tests","unit-tests.ini"),
                      help="Specify the test manifest, defaults to tests/all-tests.ini")
    parser.add_option("--log-file", action="store", dest="logfile",
                      default="b2gautomation.log",
                      help="Log file to store results, defaults to b2gautomation.log")

    LOG_LEVELS = ("DEBUG", "INFO", "WARNING", "ERROR")
    LEVEL_STRING = ", ".join(LOG_LEVELS)
    parser.add_option("--log-level", action="store", type="choice",
                      dest="loglevel", default="DEBUG", choices=LOG_LEVELS,
                      help = "One of %s for logging level, defaults  to debug" % LEVEL_STRING)
    options, args = parser.parse_args()

    cfg = ConfigParser.ConfigParser()
    cfg.read(options.config_file)
    try:
        es_server = cfg.get('marionette', 'es_server')
    except:
        
        es_server = None
    try:
        rest_server = cfg.get('marionette', 'rest_server')
    except:
        
        rest_server = None

    try:
        tests = cfg.items('tests')
    except:
        tests = [('marionette', options.testmanifest)]

    if not options.testmanifest:
        parser.print_usage()
        parser.exit()

    if not os.path.exists(options.testmanifest):
        print "Could not find manifest file: %s" % options.testmanifest
        parser.print_usage()
        parser.exit()

    
    if os.path.exists(options.logfile):
        os.remove(options.logfile)

    logger = mozlog.getLogger("B2G_AUTOMATION", options.logfile)
    if options.loglevel:
        logger.setLevel(getattr(mozlog, options.loglevel, "DEBUG"))
    logger.addHandler(logging.StreamHandler())

    try:
        b2gauto = B2GAutomation(tests,
                                testfile=options.testfile,
                                es_server=es_server,
                                rest_server=rest_server)
    except:
        s = traceback.format_exc()
        logger.error(s)
        return 1
    return 0

if __name__ == "__main__":
    main()


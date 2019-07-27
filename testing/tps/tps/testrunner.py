



import json
import os
import platform
import random
import re
import tempfile
import time
import traceback

from mozhttpd import MozHttpd
import mozinfo
from mozprofile import Profile
import mozversion

from .firefoxrunner import TPSFirefoxRunner
from .phase import TPSTestPhase


class TempFile(object):
    """Class for temporary files that delete themselves when garbage-collected.
    """

    def __init__(self, prefix=None):
        self.fd, self.filename = self.tmpfile = tempfile.mkstemp(prefix=prefix)

    def write(self, data):
        if self.fd:
            os.write(self.fd, data)

    def close(self):
        if self.fd:
            os.close(self.fd)
            self.fd = None

    def cleanup(self):
        if self.fd:
            self.close()
        if os.access(self.filename, os.F_OK):
            os.remove(self.filename)

    __del__ = cleanup


class TPSTestRunner(object):

    extra_env = {
        'MOZ_CRASHREPORTER_DISABLE': '1',
        'GNOME_DISABLE_CRASH_DIALOG': '1',
        'XRE_NO_WINDOWS_CRASH_DIALOG': '1',
        'MOZ_NO_REMOTE': '1',
        'XPCOM_DEBUG_BREAK': 'warn',
    }

    default_preferences = {
        'app.update.enabled': False,
        'browser.dom.window.dump.enabled': True,
        'browser.sessionstore.resume_from_crash': False,
        'browser.shell.checkDefaultBrowser': False,
        'browser.tabs.warnOnClose': False,
        'browser.warnOnQuit': False,
        
        'extensions.autoDisableScopes': 10,
        'extensions.getAddons.get.url': 'http://127.0.0.1:4567/addons/api/%IDS%.xml',
        'extensions.update.enabled': False,
        
        'extensions.update.notifyUser': False,
        'services.sync.addons.ignoreRepositoryChecking': True,
        'services.sync.firstSync': 'notReady',
        'services.sync.lastversion': '1.0',
        'toolkit.startup.max_resumed_crashes': -1,
        
        'dom.serviceWorkers.periodic-updates.enabled': False,
    }

    debug_preferences = {
        'services.sync.log.appender.console': 'Trace',
        'services.sync.log.appender.dump': 'Trace',
        'services.sync.log.appender.file.level': 'Trace',
        'services.sync.log.appender.file.logOnSuccess': True,
        'services.sync.log.rootLogger': 'Trace',
        'services.sync.log.logger.addonutils': 'Trace',
        'services.sync.log.logger.declined': 'Trace',
        'services.sync.log.logger.service.main': 'Trace',
        'services.sync.log.logger.status': 'Trace',
        'services.sync.log.logger.authenticator': 'Trace',
        'services.sync.log.logger.network.resources': 'Trace',
        'services.sync.log.logger.service.jpakeclient': 'Trace',
        'services.sync.log.logger.engine.bookmarks': 'Trace',
        'services.sync.log.logger.engine.clients': 'Trace',
        'services.sync.log.logger.engine.forms': 'Trace',
        'services.sync.log.logger.engine.history': 'Trace',
        'services.sync.log.logger.engine.passwords': 'Trace',
        'services.sync.log.logger.engine.prefs': 'Trace',
        'services.sync.log.logger.engine.tabs': 'Trace',
        'services.sync.log.logger.engine.addons': 'Trace',
        'services.sync.log.logger.engine.apps': 'Trace',
        'services.sync.log.logger.identity': 'Trace',
        'services.sync.log.logger.userapi': 'Trace',
    }

    syncVerRe = re.compile(
        r'Sync version: (?P<syncversion>.*)\n')
    ffVerRe = re.compile(
        r'Firefox version: (?P<ffver>.*)\n')
    ffBuildIDRe = re.compile(
        r'Firefox buildid: (?P<ffbuildid>.*)\n')

    def __init__(self, extensionDir,
                 binary=None,
                 config=None,
                 debug=False,
                 ignore_unused_engines=False,
                 logfile='tps.log',
                 mobile=False,
                 rlock=None,
                 resultfile='tps_result.json',
                 testfile=None):
        self.binary = binary
        self.config = config if config else {}
        self.debug = debug
        self.extensions = []
        self.ignore_unused_engines = ignore_unused_engines
        self.logfile = os.path.abspath(logfile)
        self.mobile = mobile
        self.rlock = rlock
        self.resultfile = resultfile
        self.testfile = testfile

        self.addonversion = None
        self.branch = None
        self.changeset = None
        self.errorlogs = {}
        self.extensionDir = extensionDir
        self.firefoxRunner = None
        self.nightly = False
        self.numfailed = 0
        self.numpassed = 0
        self.postdata = {}
        self.productversion = None
        self.repo = None
        self.tpsxpi = None

    @property
    def mobile(self):
        return self._mobile

    @mobile.setter
    def mobile(self, value):
        self._mobile = value
        self.synctype = 'desktop' if not self._mobile else 'mobile'

    def log(self, msg, printToConsole=False):
        """Appends a string to the logfile"""

        f = open(self.logfile, 'a')
        f.write(msg)
        f.close()
        if printToConsole:
            print msg

    def writeToResultFile(self, postdata, body=None,
                          sendTo=['crossweave@mozilla.com']):
        """Writes results to test file"""

        results = {'results': []}

        if os.access(self.resultfile, os.F_OK):
            f = open(self.resultfile, 'r')
            results = json.loads(f.read())
            f.close()

        f = open(self.resultfile, 'w')
        if body is not None:
            postdata['body'] = body
        if self.numpassed is not None:
            postdata['numpassed'] = self.numpassed
        if self.numfailed is not None:
            postdata['numfailed'] = self.numfailed
        if self.firefoxRunner and self.firefoxRunner.url:
            postdata['firefoxrunnerurl'] = self.firefoxRunner.url

        postdata['sendTo'] = sendTo
        results['results'].append(postdata)
        f.write(json.dumps(results, indent=2))
        f.close()

    def _zip_add_file(self, zip, file, rootDir):
        zip.write(os.path.join(rootDir, file), file)

    def _zip_add_dir(self, zip, dir, rootDir):
        try:
            zip.write(os.path.join(rootDir, dir), dir)
        except:
            
            pass
        for root, dirs, files in os.walk(os.path.join(rootDir, dir)):
            for f in files:
                zip.write(os.path.join(root, f), os.path.join(dir, f))

    def run_single_test(self, testdir, testname):
        testpath = os.path.join(testdir, testname)
        self.log("Running test %s\n" % testname, True)

        
        
        f = open(testpath, 'r')
        testcontent = f.read()
        f.close()
        try:
            test = json.loads(testcontent)
        except:
            test = json.loads(testcontent[testcontent.find('{'):testcontent.find('}') + 1])

        testcontent += 'var config = %s;\n' % json.dumps(self.config, indent=2)
        testcontent += 'var seconds_since_epoch = %d;\n' % int(time.time())

        tmpfile = TempFile(prefix='tps_test_')
        tmpfile.write(testcontent)
        tmpfile.close()

        
        profiles = {}
        phaselist = []
        for phase in test:
            profilename = test[phase]

            
            if not profilename in profiles:
                profiles[profilename] = Profile(preferences = self.preferences,
                                                addons = self.extensions)

            
            phaselist.append(TPSTestPhase(
                phase,
                profiles[profilename],
                testname,
                tmpfile.filename,
                self.logfile,
                self.env,
                self.firefoxRunner,
                self.log,
                ignore_unused_engines=self.ignore_unused_engines))

        
        phaselist = sorted(phaselist, key=lambda phase: phase.phase)

        
        for phase in phaselist:
            phase.run()

            
            if phase.status != 'PASS':
                for profile in profiles:
                    self.log('\nDumping sync log for profile %s\n' %  profiles[profile].profile)
                    for root, dirs, files in os.walk(os.path.join(profiles[profile].profile, 'weave', 'logs')):
                        for f in files:
                            weavelog = os.path.join(profiles[profile].profile, 'weave', 'logs', f)
                            if os.access(weavelog, os.F_OK):
                                with open(weavelog, 'r') as fh:
                                    for line in fh:
                                        possible_time = line[0:13]
                                        if len(possible_time) == 13 and possible_time.isdigit():
                                            time_ms = int(possible_time)
                                            formatted = time.strftime('%Y-%m-%d %H:%M:%S',
                                                    time.localtime(time_ms / 1000))
                                            self.log('%s.%03d %s' % (
                                                formatted, time_ms % 1000, line[14:] ))
                                        else:
                                            self.log(line)
                break;

        
        f = open(self.logfile)
        logdata = f.read()
        match = self.syncVerRe.search(logdata)
        sync_version = match.group('syncversion') if match else 'unknown'
        match = self.ffVerRe.search(logdata)
        firefox_version = match.group('ffver') if match else 'unknown'
        match = self.ffBuildIDRe.search(logdata)
        firefox_buildid = match.group('ffbuildid') if match else 'unknown'
        f.close()
        if phase.status == 'PASS':
            logdata = ''
        else:
            
            logdata = logdata[logdata.find('Running test %s' % (str(testname))):]

        result = {
          'PASS': lambda x: ('TEST-PASS', ''),
          'FAIL': lambda x: ('TEST-UNEXPECTED-FAIL', x.rstrip()),
          'unknown': lambda x: ('TEST-UNEXPECTED-FAIL', 'test did not complete')
        } [phase.status](phase.errline)
        logstr = "\n%s | %s%s\n" % (result[0], testname, (' | %s' % result[1] if result[1] else ''))

        try:
            repoinfo = mozversion.get_version(self.binary)
        except:
            repoinfo = {}
        apprepo = repoinfo.get('application_repository', '')
        appchangeset = repoinfo.get('application_changeset', '')

        
        tmplogfile = None
        if logdata:
            tmplogfile = TempFile(prefix='tps_log_')
            tmplogfile.write(logdata)
            tmplogfile.close()
            self.errorlogs[testname] = tmplogfile

        resultdata = ({ 'productversion': { 'version': firefox_version,
                                            'buildid': firefox_buildid,
                                            'builddate': firefox_buildid[0:8],
                                            'product': 'Firefox',
                                            'repository': apprepo,
                                            'changeset': appchangeset,
                                          },
                        'addonversion': { 'version': sync_version,
                                          'product': 'Firefox Sync' },
                        'name': testname,
                        'message': result[1],
                        'state': result[0],
                        'logdata': logdata
                      })

        self.log(logstr, True)
        for phase in phaselist:
            print "\t%s: %s" % (phase.phase, phase.status)
            if phase.status == 'FAIL':
                break

        return resultdata

    def update_preferences(self):
        self.preferences = self.default_preferences.copy()

        if self.mobile:
            self.preferences.update({'services.sync.client.type' : 'mobile'})

        
        
        dummy = {'fx_account': 'dummy@somewhere', 'sync_account': 'dummy'}
        auth_type = self.config.get('auth_type', 'fx_account')
        self.preferences.update({'services.sync.username': dummy[auth_type]})

        if self.debug:
            self.preferences.update(self.debug_preferences)

    def run_tests(self):
        
        if os.access(self.logfile, os.F_OK):
            os.remove(self.logfile)

        
        self.env = os.environ.copy()
        self.env.update(self.extra_env)

        
        self.update_preferences()

        
        
        if self.rlock:
            self.rlock.acquire()

        try:
            
            
            if not self.firefoxRunner:
                self.firefoxRunner = TPSFirefoxRunner(self.binary)

            
            self.run_test_group()

        except:
            traceback.print_exc()
            self.numpassed = 0
            self.numfailed = 1
            try:
                self.writeToResultFile(self.postdata,
                                       '<pre>%s</pre>' % traceback.format_exc())
            except:
                traceback.print_exc()
        else:
            try:

                if self.numfailed > 0 or self.numpassed == 0:
                    To = self.config['email'].get('notificationlist')
                else:
                    To = self.config['email'].get('passednotificationlist')
                self.writeToResultFile(self.postdata,
                                       sendTo=To)
            except:
                traceback.print_exc()
                try:
                    self.writeToResultFile(self.postdata,
                                           '<pre>%s</pre>' % traceback.format_exc())
                except:
                    traceback.print_exc()

        
        if self.rlock:
            self.rlock.release()

        
        print 'Test Summary\n'
        for test in self.postdata.get('tests', {}):
            print '%s | %s | %s' % (test['state'], test['name'], test['message'])

    def run_test_group(self):
        self.results = []

        
        self.numpassed = 0
        self.numfailed = 0

        
        self.extensions = []
        self.extensions.append(os.path.join(self.extensionDir, 'tps'))
        self.extensions.append(os.path.join(self.extensionDir, "mozmill"))

        
        try:
            f = open(self.testfile)
            jsondata = f.read()
            f.close()
            testfiles = json.loads(jsondata)
            testlist = testfiles['tests']
        except ValueError:
            testlist = [os.path.basename(self.testfile)]
        testdir = os.path.dirname(self.testfile)

        self.mozhttpd = MozHttpd(port=4567, docroot=testdir)
        self.mozhttpd.start()

        
        for test in testlist:
            result = self.run_single_test(testdir, test)

            if not self.productversion:
                self.productversion = result['productversion']
            if not self.addonversion:
                self.addonversion = result['addonversion']

            self.results.append({'state': result['state'],
                                 'name': result['name'],
                                 'message': result['message'],
                                 'logdata': result['logdata']})
            if result['state'] == 'TEST-PASS':
                self.numpassed += 1
            else:
                self.numfailed += 1

        self.mozhttpd.stop()

        
        self.postdata = { 'tests': self.results,
                          'os': '%s %sbit' % (mozinfo.version, mozinfo.bits),
                          'testtype': 'crossweave',
                          'productversion': self.productversion,
                          'addonversion': self.addonversion,
                          'synctype': self.synctype,
                        }

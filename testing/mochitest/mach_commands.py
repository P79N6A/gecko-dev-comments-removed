



from __future__ import unicode_literals

from argparse import Namespace
from collections import defaultdict
from itertools import chain
import logging
import os
import shutil
import sys
import warnings
import which

from mozbuild.base import (
    MachCommandBase,
    MachCommandConditions as conditions,
    MozbuildObject,
)

from mach.decorators import (
    CommandArgument,
    CommandProvider,
    Command,
)
import mozpack.path as mozpath

here = os.path.abspath(os.path.dirname(__file__))


ADB_NOT_FOUND = '''
The mochitest command requires the adb binary to be on your path.

If you have a B2G build, this can be found in
'{}/out/host/<platform>/bin'.
'''.lstrip()

GAIA_PROFILE_NOT_FOUND = '''
The mochitest command requires a non-debug gaia profile. Either
pass in --profile, or set the GAIA_PROFILE environment variable.

If you do not have a non-debug gaia profile, you can build one:
    $ git clone https://github.com/mozilla-b2g/gaia
    $ cd gaia
    $ make

The profile should be generated in a directory called 'profile'.
'''.lstrip()

GAIA_PROFILE_IS_DEBUG = '''
The mochitest command requires a non-debug gaia profile. The
specified profile, {}, is a debug profile.

If you do not have a non-debug gaia profile, you can build one:
    $ git clone https://github.com/mozilla-b2g/gaia
    $ cd gaia
    $ make

The profile should be generated in a directory called 'profile'.
'''.lstrip()

ENG_BUILD_REQUIRED = '''
The mochitest command requires an engineering build. It may be the case that
VARIANT=user or PRODUCTION=1 were set. Try re-building with VARIANT=eng:

    $ VARIANT=eng ./build.sh

There should be an app called 'test-container.gaiamobile.org' located in
{}.
'''.lstrip()

SUPPORTED_TESTS_NOT_FOUND = '''
The mochitest command could not find any supported tests to run! The
following flavors and subsuites were found, but are either not supported on
{} builds, or were excluded on the command line:

{}

Double check the command line you used, and make sure you are running in
context of the proper build. To switch build contexts, either run |mach|
from the appropriate objdir, or export the correct mozconfig:

    $ export MOZCONFIG=path/to/mozconfig
'''.lstrip()

TESTS_NOT_FOUND = '''
The mochitest command could not find any mochitests under the following
test path(s):

{}

Please check spelling and make sure there are mochitests living there.
'''.lstrip()

NOW_RUNNING = '''
######
### Now running mochitest-{}.
######
'''



ALL_FLAVORS = {
    'mochitest': {
        'suite': 'plain',
        'aliases': ('plain', 'mochitest'),
        'enabled_apps': ('firefox', 'b2g', 'android', 'mulet', 'b2g_desktop'),
    },
    'chrome': {
        'suite': 'chrome',
        'aliases': ('chrome', 'mochitest-chrome'),
        'enabled_apps': ('firefox', 'mulet', 'b2g', 'android'),
        'extra_args': {
            'chrome': True,
        }
    },
    'browser-chrome': {
        'suite': 'browser',
        'aliases': ('browser', 'browser-chrome', 'mochitest-browser-chrome', 'bc'),
        'enabled_apps': ('firefox',),
        'extra_args': {
            'browserChrome': True,
        }
    },
    'jetpack-package': {
        'suite': 'jetpack-package',
        'aliases': ('jetpack-package', 'mochitest-jetpack-package', 'jpp'),
        'enabled_apps': ('firefox',),
        'extra_args': {
            'jetpackPackage': True,
        }
    },
    'jetpack-addon': {
        'suite': 'jetpack-addon',
        'aliases': ('jetpack-addon', 'mochitest-jetpack-addon', 'jpa'),
        'enabled_apps': ('firefox',),
        'extra_args': {
            'jetpackAddon': True,
        }
    },
    'a11y': {
        'suite': 'a11y',
        'aliases': ('a11y', 'mochitest-a11y', 'accessibility'),
        'enabled_apps': ('firefox',),
        'extra_args': {
            'a11y': True,
        }
    },
    'webapprt-chrome': {
        'suite': 'webapprt-chrome',
        'aliases': ('webapprt-chrome', 'mochitest-webapprt-chrome'),
        'enabled_apps': ('firefox',),
        'extra_args': {
            'webapprtChrome': True,
        }
    },
    'webapprt-content': {
        'suite': 'webapprt-content',
        'aliases': ('webapprt-content', 'mochitest-webapprt-content'),
        'enabled_apps': ('firefox',),
        'extra_args': {
            'webapprtContent': True,
        }
    },
}

SUPPORTED_APPS = ['firefox', 'b2g', 'android', 'mulet', 'b2g_desktop']
SUPPORTED_FLAVORS = list(chain.from_iterable([f['aliases'] for f in ALL_FLAVORS.values()]))
CANONICAL_FLAVORS = sorted([f['aliases'][0] for f in ALL_FLAVORS.values()])


class MochitestRunner(MozbuildObject):

    """Easily run mochitests.

    This currently contains just the basics for running mochitests. We may want
    to hook up result parsing, etc.
    """

    def get_webapp_runtime_path(self):
        import mozinfo
        app_name = 'webapprt-stub' + mozinfo.info.get('bin_suffix', '')
        app_path = os.path.join(self.distdir, 'bin', app_name)
        if sys.platform.startswith('darwin'):
            
            
            
            
            mac_dir_name = os.path.join(
                self.mochitest_dir,
                'webapprtChrome',
                'webapprt',
                'test',
                'chrome',
                'TestApp.app',
                'Contents',
                'MacOS')
            mac_app_name = 'webapprt' + mozinfo.info.get('bin_suffix', '')
            mac_app_path = os.path.join(mac_dir_name, mac_app_name)
            shutil.copy(app_path, mac_app_path)
            return mac_app_path
        return app_path

    
    
    
    def get_webapp_runtime_xre_path(self):
        if sys.platform.startswith('darwin'):
            xre_path = os.path.join(
                self.distdir,
                self.substs['MOZ_MACBUNDLE_NAME'],
                'Contents',
                'Resources')
        else:
            xre_path = os.path.join(self.distdir, 'bin')
        return xre_path

    def __init__(self, *args, **kwargs):
        MozbuildObject.__init__(self, *args, **kwargs)

        
        build_path = os.path.join(self.topobjdir, 'build')
        if build_path not in sys.path:
            sys.path.append(build_path)

        self.tests_dir = os.path.join(self.topobjdir, '_tests')
        self.mochitest_dir = os.path.join(
            self.tests_dir,
            'testing',
            'mochitest')
        self.bin_dir = os.path.join(self.topobjdir, 'dist', 'bin')

    def resolve_tests(self, test_paths, test_objects=None, cwd=None):
        if test_objects:
            return test_objects

        from mozbuild.testing import TestResolver
        resolver = self._spawn(TestResolver)
        tests = list(resolver.resolve_tests(paths=test_paths, cwd=cwd))
        return tests

    def run_b2g_test(self, context, tests=None, suite='mochitest', **kwargs):
        """Runs a b2g mochitest."""
        if kwargs.get('desktop'):
            kwargs['profile'] = kwargs.get('profile') or os.environ.get('GAIA_PROFILE')
            if not kwargs['profile'] or not os.path.isdir(kwargs['profile']):
                print(GAIA_PROFILE_NOT_FOUND)
                sys.exit(1)

            if os.path.isfile(os.path.join(kwargs['profile'], 'extensions',
                                           'httpd@gaiamobile.org')):
                print(GAIA_PROFILE_IS_DEBUG.format(kwargs['profile']))
                sys.exit(1)
        elif context.target_out:
            host_webapps_dir = os.path.join(context.target_out, 'data', 'local', 'webapps')
            if not os.path.isdir(os.path.join(
                    host_webapps_dir, 'test-container.gaiamobile.org')):
                print(ENG_BUILD_REQUIRED.format(host_webapps_dir))
                sys.exit(1)

        
        os.chdir(self.mochitest_dir)

        
        
        with warnings.catch_warnings():
            warnings.simplefilter('ignore')

            import imp
            path = os.path.join(self.mochitest_dir, 'runtestsb2g.py')
            with open(path, 'r') as fh:
                imp.load_module('mochitest', fh, path,
                                ('.py', 'r', imp.PY_SOURCE))

            import mochitest

        options = Namespace(**kwargs)

        from manifestparser import TestManifest
        manifest = TestManifest()
        manifest.tests.extend(tests)
        options.manifestFile = manifest

        if options.desktop:
            return mochitest.run_desktop_mochitests(options)

        try:
            which.which('adb')
        except which.WhichError:
            
            print(ADB_NOT_FOUND.format(options.b2gPath))
            return 1

        return mochitest.run_remote_mochitests(options)

    def run_desktop_test(self, context, tests=None, suite=None, **kwargs):
        """Runs a mochitest.

        suite is the type of mochitest to run. It can be one of ('plain',
        'chrome', 'browser', 'a11y', 'jetpack-package', 'jetpack-addon',
        'webapprt-chrome', 'webapprt-content').
        """
        
        if 'mochitest' not in sys.modules:
            import imp
            path = os.path.join(self.mochitest_dir, 'runtests.py')
            with open(path, 'r') as fh:
                imp.load_module('mochitest', fh, path,
                                ('.py', 'r', imp.PY_SOURCE))

        import mochitest

        
        os.chdir(self.topobjdir)

        
        
        remove_handlers = [l for l in logging.getLogger().handlers
                           if isinstance(l, logging.StreamHandler)]
        for handler in remove_handlers:
            logging.getLogger().removeHandler(handler)

        options = Namespace(**kwargs)

        if suite == 'webapprt-content':
            if not options.app or options.app == self.get_binary_path():
                options.app = self.get_webapp_runtime_path()
            options.xrePath = self.get_webapp_runtime_xre_path()
        elif suite == 'webapprt-chrome':
            options.browserArgs.append("-test-mode")
            if not options.app or options.app == self.get_binary_path():
                options.app = self.get_webapp_runtime_path()
            options.xrePath = self.get_webapp_runtime_xre_path()
            
            
            if sys.platform.startswith('darwin'):
                options.browserArgs.extend(('-runtime', os.path.join(self.distdir, self.substs['MOZ_MACBUNDLE_NAME'])))

        from manifestparser import TestManifest
        manifest = TestManifest()
        manifest.tests.extend(tests)
        options.manifestFile = manifest

        
        if len(tests) == 1 and options.closeWhenDone and suite == 'plain':
            options.closeWhenDone = False

        
        self.log_manager.enable_unstructured()
        result = mochitest.run_test_harness(options)
        self.log_manager.disable_unstructured()
        return result

    def run_android_test(self, context, tests, suite=None, **kwargs):
        host_ret = verify_host_bin()
        if host_ret != 0:
            return host_ret

        import imp
        path = os.path.join(self.mochitest_dir, 'runtestsremote.py')
        with open(path, 'r') as fh:
            imp.load_module('runtestsremote', fh, path,
                            ('.py', 'r', imp.PY_SOURCE))
        import runtestsremote

        options = Namespace(**kwargs)

        from manifestparser import TestManifest
        manifest = TestManifest()
        manifest.tests.extend(tests)
        options.manifestFile = manifest

        return runtestsremote.run_test_harness(options)




def setup_argument_parser():
    build_obj = MozbuildObject.from_environment(cwd=here)

    build_path = os.path.join(build_obj.topobjdir, 'build')
    if build_path not in sys.path:
        sys.path.append(build_path)

    mochitest_dir = os.path.join(build_obj.topobjdir, '_tests', 'testing', 'mochitest')

    with warnings.catch_warnings():
        warnings.simplefilter('ignore')

        import imp
        path = os.path.join(build_obj.topobjdir, mochitest_dir, 'runtests.py')
        with open(path, 'r') as fh:
            imp.load_module('mochitest', fh, path,
                            ('.py', 'r', imp.PY_SOURCE))

        from mochitest_options import MochitestArgumentParser

    return MochitestArgumentParser()




def is_buildapp_in(*apps):
    def is_buildapp_supported(cls):
        for a in apps:
            c = getattr(conditions, 'is_{}'.format(a), None)
            if c and c(cls):
                return True
        return False

    is_buildapp_supported.__doc__ = 'Must have a {} build.'.format(
        ' or '.join(apps))
    return is_buildapp_supported


def verify_host_bin():
    
    MOZ_HOST_BIN = os.environ.get('MOZ_HOST_BIN')
    if not MOZ_HOST_BIN:
        print('environment variable MOZ_HOST_BIN must be set to a directory containing host xpcshell')
        return 1
    elif not os.path.isdir(MOZ_HOST_BIN):
        print('$MOZ_HOST_BIN does not specify a directory')
        return 1
    elif not os.path.isfile(os.path.join(MOZ_HOST_BIN, 'xpcshell')):
        print('$MOZ_HOST_BIN/xpcshell does not exist')
        return 1
    return 0


@CommandProvider
class MachCommands(MachCommandBase):
    @Command('mochitest', category='testing',
             conditions=[is_buildapp_in(*SUPPORTED_APPS)],
             description='Run any flavor of mochitest (integration test).',
             parser=setup_argument_parser)
    @CommandArgument('-f', '--flavor',
                     metavar='{{{}}}'.format(', '.join(CANONICAL_FLAVORS)),
                     choices=SUPPORTED_FLAVORS,
                     help='Only run tests of this flavor.')
    def run_mochitest_general(self, flavor=None, test_objects=None, **kwargs):
        buildapp = None
        for app in SUPPORTED_APPS:
            if is_buildapp_in(app)(self):
                buildapp = app
                break

        flavors = None
        if flavor:
            for fname, fobj in ALL_FLAVORS.iteritems():
                if flavor in fobj['aliases']:
                    if buildapp not in fobj['enabled_apps']:
                        continue
                    flavors = [fname]
                    break
        else:
            flavors = [f for f, v in ALL_FLAVORS.iteritems() if buildapp in v['enabled_apps']]

        from mozbuild.controller.building import BuildDriver
        self._ensure_state_subdir_exists('.')

        driver = self._spawn(BuildDriver)
        driver.install_tests(remove=False)

        test_paths = kwargs['test_paths']
        kwargs['test_paths'] = []

        if test_paths and buildapp == 'b2g':
            
            
            
            gecko_path = mozpath.abspath(mozpath.join(kwargs['b2gPath'], 'gecko'))
            if gecko_path != self.topsrcdir:
                new_paths = []
                for tp in test_paths:
                    if mozpath.abspath(tp).startswith(gecko_path):
                        new_paths.append(mozpath.relpath(tp, gecko_path))
                    else:
                        new_paths.append(tp)
                test_paths = new_paths

        mochitest = self._spawn(MochitestRunner)
        tests = mochitest.resolve_tests(test_paths, test_objects, cwd=self._mach_context.cwd)

        subsuite = kwargs.get('subsuite')
        if subsuite == 'default':
            kwargs['subsuite'] = None

        suites = defaultdict(list)
        unsupported = set()
        for test in tests:
            
            if test['flavor'] not in ALL_FLAVORS:
                continue

            key = (test['flavor'], test['subsuite'])
            if test['flavor'] not in flavors:
                unsupported.add(key)
                continue

            if subsuite == 'default':
                
                if test['subsuite']:
                    unsupported.add(key)
                    continue
            elif subsuite and test['subsuite'] != subsuite:
                unsupported.add(key)
                continue

            suites[key].append(test)

        if not suites:
            
            if not unsupported:
                print(TESTS_NOT_FOUND.format('\n'.join(
                    sorted(list(test_paths or test_objects)))))
                return 1

            msg = []
            for f, s in unsupported:
                fobj = ALL_FLAVORS[f]
                apps = fobj['enabled_apps']
                name = fobj['aliases'][0]
                if s:
                    name = '{} --subsuite {}'.format(name, s)

                if buildapp not in apps:
                    reason = 'requires {}'.format(' or '.join(apps))
                else:
                    reason = 'excluded by the command line'
                msg.append('    mochitest -f {} ({})'.format(name, reason))
            print(SUPPORTED_TESTS_NOT_FOUND.format(
                buildapp, '\n'.join(sorted(msg))))
            return 1

        if buildapp in ('b2g', 'b2g_desktop'):
            run_mochitest = mochitest.run_b2g_test
        elif buildapp == 'android':
            run_mochitest = mochitest.run_android_test
        else:
            run_mochitest = mochitest.run_desktop_test

        overall = None
        for (flavor, subsuite), tests in sorted(suites.items()):
            fobj = ALL_FLAVORS[flavor]
            msg = fobj['aliases'][0]
            if subsuite:
                msg = '{} with subsuite {}'.format(msg, subsuite)
            print(NOW_RUNNING.format(msg))

            harness_args = kwargs.copy()
            harness_args['subsuite'] = subsuite
            harness_args.update(fobj.get('extra_args', {}))

            result = run_mochitest(
                self._mach_context,
                tests=tests,
                suite=fobj['suite'],
                **harness_args)

            if result:
                overall = result

        
        return overall


@CommandProvider
class RobocopCommands(MachCommandBase):

    @Command('robocop', category='testing',
             conditions=[conditions.is_android],
             description='Run a Robocop test.',
             parser=setup_argument_parser)
    @CommandArgument('--serve', default=False, action='store_true',
        help='Run no tests but start the mochi.test web server and launch '
             'Fennec with a test profile.')
    def run_robocop(self, serve=False, **kwargs):
        if serve:
            kwargs['autorun'] = False

        if not kwargs.get('robocopIni'):
            kwargs['robocopIni'] = os.path.join(self.topobjdir, '_tests', 'testing',
                                                'mochitest', 'robocop.ini')

        if not kwargs.get('robocopApk'):
            kwargs['robocopApk'] = os.path.join(self.topobjdir, 'build', 'mobile',
                                                'robocop', 'robocop-debug.apk')

        from mozbuild.controller.building import BuildDriver
        self._ensure_state_subdir_exists('.')

        driver = self._spawn(BuildDriver)
        driver.install_tests(remove=False)

        test_paths = kwargs['test_paths']
        kwargs['test_paths'] = []

        from mozbuild.testing import TestResolver
        resolver = self._spawn(TestResolver)
        tests = list(resolver.resolve_tests(paths=test_paths, cwd=self._mach_context.cwd,
            flavor='instrumentation', subsuite='robocop'))

        mochitest = self._spawn(MochitestRunner)
        return mochitest.run_android_test(self._mach_context, tests, 'robocop', **kwargs)


def REMOVED(cls):
    """Command no longer exists! Use |mach mochitest| instead.

    The |mach mochitest| command will automatically detect which flavors and
    subsuites exist in a given directory. If desired, flavors and subsuites
    can be restricted using `--flavor` and `--subsuite` respectively. E.g:

        $ ./mach mochitest dom/indexedDB

    will run all of the plain, chrome and browser-chrome mochitests in that
    directory. To only run the plain mochitests:

        $ ./mach mochitest -f plain dom/indexedDB
    """
    return False


@CommandProvider
class DeprecatedCommands(MachCommandBase):
    @Command('mochitest-plain', category='testing', conditions=[REMOVED])
    def mochitest_plain(self):
        pass

    @Command('mochitest-chrome', category='testing', conditions=[REMOVED])
    def mochitest_chrome(self):
        pass

    @Command('mochitest-browser', category='testing', conditions=[REMOVED])
    def mochitest_browser(self):
        pass

    @Command('mochitest-devtools', category='testing', conditions=[REMOVED])
    def mochitest_devtools(self):
        pass

    @Command('mochitest-a11y', category='testing', conditions=[REMOVED])
    def mochitest_a11y(self):
        pass

    @Command('jetpack-addon', category='testing', conditions=[REMOVED])
    def jetpack_addon(self):
        pass

    @Command('jetpack-package', category='testing', conditions=[REMOVED])
    def jetpack_package(self):
        pass

    @Command('webapprt-test-chrome', category='testing', conditions=[REMOVED])
    def webapprt_chrome(self):
        pass

    @Command('webapprt-test-content', category='testing', conditions=[REMOVED])
    def webapprt_content(self):
        pass

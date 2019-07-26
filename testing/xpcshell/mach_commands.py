





from __future__ import unicode_literals

import mozpack.path
import os
import sys

from StringIO import StringIO

from mozbuild.base import (
    MachCommandBase,
    MozbuildObject,
)

from mach.decorators import (
    CommandArgument,
    CommandProvider,
    Command,
)


if sys.version_info[0] < 3:
    unicode_type = unicode
else:
    unicode_type = str




class InvalidTestPathError(Exception):
    """Exception raised when the test path is not valid."""


class XPCShellRunner(MozbuildObject):
    """Run xpcshell tests."""
    def run_suite(self, **kwargs):
        manifest = os.path.join(self.topobjdir, '_tests', 'xpcshell',
            'xpcshell.ini')

        return self._run_xpcshell_harness(manifest=manifest, **kwargs)

    def run_test(self, test_file, debug=False, interactive=False,
        keep_going=False, shuffle=False):
        """Runs an individual xpcshell test."""

        if test_file == 'all':
            self.run_suite(debug=debug, interactive=interactive,
                keep_going=keep_going, shuffle=shuffle)
            return

        test_src_file = self.get_src_path(test_file)
        test_src_dir = test_src_file if os.path.isdir(test_src_file) \
            else mozpack.path.dirname(test_src_file)

        relative_dir = mozpack.path.relpath(test_src_dir, self.topsrcdir)

        test_obj_dir = mozpack.path.join(self.topobjdir, '_tests', 'xpcshell',
                relative_dir)

        xpcshell_ini_file = mozpack.path.join(test_obj_dir, 'xpcshell.ini')
        if not os.path.exists(xpcshell_ini_file):
            raise InvalidTestPathError('An xpcshell.ini could not be found '
                'for the passed test path. Please select a path whose '
                'directory contains an xpcshell.ini file. It is possible you '
                'received this error because the tree is not built or tests '
                'are not enabled.')

        args = {
            'debug': debug,
            'interactive': interactive,
            'keep_going': keep_going,
            'shuffle': shuffle,
            'test_dirs': [test_obj_dir],
        }

        if os.path.isfile(test_src_file):
            args['test_path'] = mozpack.path.basename(test_src_file)

        return self._run_xpcshell_harness(**args)

    def get_src_path(self, test_file):
        """Returns the absolute path to test_file within topsrcdir."""
        test_file = mozpack.path.normsep(test_file)
        topsrcdir = mozpack.path.normsep(self.topsrcdir)
        if test_file.startswith(topsrcdir):
            return test_file
        return mozpack.path.join(topsrcdir, test_file)

    def _run_xpcshell_harness(self, test_dirs=None, manifest=None,
        test_path=None, debug=False, shuffle=False, interactive=False,
        keep_going=False):

        
        import runxpcshelltests

        dummy_log = StringIO()
        xpcshell = runxpcshelltests.XPCShellTests(log=dummy_log)
        self.log_manager.enable_unstructured()

        tests_dir = os.path.join(self.topobjdir, '_tests', 'xpcshell')
        modules_dir = os.path.join(self.topobjdir, '_tests', 'modules')

        args = {
            'xpcshell': os.path.join(self.bindir, 'xpcshell'),
            'mozInfo': os.path.join(self.topobjdir, 'mozinfo.json'),
            'symbolsPath': os.path.join(self.distdir, 'crashreporter-symbols'),
            'interactive': interactive,
            'keepGoing': keep_going,
            'logfiles': False,
            'shuffle': shuffle,
            'testsRootDir': tests_dir,
            'testingModulesDir': modules_dir,
            'profileName': 'firefox',
            'verbose': test_path is not None,
            'xunitFilename': os.path.join(self.statedir, 'xpchsell.xunit.xml'),
            'xunitName': 'xpcshell',
            'pluginsPath': os.path.join(self.distdir, 'plugins'),
        }

        if manifest is not None:
            args['manifest'] = manifest
        elif test_dirs is not None:
            if isinstance(test_dirs, list):
                args['testdirs'] = test_dirs
            else:
                args['testdirs'] = [test_dirs]
        else:
            raise Exception('One of test_dirs or manifest must be provided.')

        if test_path is not None:
            args['testPath'] = test_path

        
        
        filtered_args = {}
        for k, v in args.items():
            if isinstance(v, unicode_type):
                v = v.encode('utf-8')

            if isinstance(k, unicode_type):
                k = k.encode('utf-8')

            filtered_args[k] = v

        result = xpcshell.runTests(**filtered_args)

        self.log_manager.disable_unstructured()

        return int(not result)


@CommandProvider
class MachCommands(MachCommandBase):
    @Command('xpcshell-test', help='Run an xpcshell test.')
    @CommandArgument('test_file', default='all', nargs='?', metavar='TEST',
        help='Test to run. Can be specified as a single JS file, a directory, '
             'or omitted. If omitted, the entire test suite is executed.')
    @CommandArgument('--debug', '-d', action='store_true',
        help='Run test in a debugger.')
    @CommandArgument('--interactive', '-i', action='store_true',
        help='Open an xpcshell prompt before running tests.')
    @CommandArgument('--keep-going', '-k', action='store_true',
        help='Continue running tests after a SIGINT is received.')
    @CommandArgument('--shuffle', '-s', action='store_true',
        help='Randomize the execution order of tests.')
    def run_xpcshell_test(self, **params):
        
        
        
        self._ensure_state_subdir_exists('.')

        xpcshell = self._spawn(XPCShellRunner)

        try:
            return xpcshell.run_test(**params)
        except InvalidTestPathError as e:
            print(e.message)
            return 1

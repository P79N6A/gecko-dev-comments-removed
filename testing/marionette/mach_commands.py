



from __future__ import unicode_literals

import imp
import os
import sys
import argparse

from mozlog.structured import commandline

from mozbuild.base import (
    MachCommandBase,
    MachCommandConditions as conditions,
)

from mach.decorators import (
    CommandArgument,
    CommandProvider,
    Command,
)

MARIONETTE_DISABLED_B2G = '''
The %s command requires a Marionette-enabled build.

Please create an engineering build, which has Marionette enabled.  You can do
this by ommitting the VARIANT variable when building, or using:

VARIANT=eng ./build.sh
'''


_parser = argparse.ArgumentParser()
commandline.add_logging_group(_parser)

def run_marionette(tests, b2g_path=None, emulator=None, testtype=None,
    address=None, binary=None, topsrcdir=None, **kwargs):

    from marionette.runtests import (
        MarionetteTestRunner,
        BaseMarionetteOptions,
        startTestRunner
    )

    parser = BaseMarionetteOptions()
    commandline.add_logging_group(parser)
    options, args = parser.parse_args()

    if not tests:
        tests = [os.path.join(topsrcdir,
                    'testing/marionette/client/marionette/tests/unit-tests.ini')]

    if b2g_path:
        options.homedir = b2g_path
        if emulator:
            options.emulator = emulator
    else:
        options.binary = binary
        path, exe = os.path.split(options.binary)

    for k, v in kwargs.iteritems():
        setattr(options, k, v)

    parser.verify_usage(options, tests)

    options.logger = commandline.setup_logging("Marionette Unit Tests",
                                               options,
                                               {"mach": sys.stdout})

    runner = startTestRunner(MarionetteTestRunner, options, tests)
    if runner.failed > 0:
        return 1

    return 0

@CommandProvider
class B2GCommands(MachCommandBase):
    def __init__(self, context):
        MachCommandBase.__init__(self, context)

        for attr in ('b2g_home', 'device_name'):
            setattr(self, attr, getattr(context, attr, None))
    @Command('marionette-webapi', category='testing',
        description='Run a Marionette webapi test (test WebAPIs using marionette).',
        conditions=[conditions.is_b2g])
    @CommandArgument('--type',
        default='b2g',
        help='Test type, usually one of: browser, b2g, b2g-qemu.')
    @CommandArgument('tests', nargs='*', metavar='TESTS',
        help='Path to test(s) to run.')
    def run_marionette_webapi(self, tests, **kwargs):
        emulator = None
        if self.device_name:
            if self.device_name.startswith('emulator'):
                emulator = 'arm'
                if 'x86' in self.device_name:
                    emulator = 'x86'

        if self.substs.get('ENABLE_MARIONETTE') != '1':
            print(MARIONETTE_DISABLED_B2G % 'marionette-webapi')
            return 1

        return run_marionette(tests, b2g_path=self.b2g_home, emulator=emulator,
            topsrcdir=self.topsrcdir, **kwargs)

@CommandProvider
class MachCommands(MachCommandBase):
    @Command('marionette-test', category='testing',
        description='Run a Marionette test (Check UI or the internal JavaScript using marionette).',
        conditions=[conditions.is_firefox],
        parser=_parser,
    )
    @CommandArgument('--address',
        help='host:port of running Gecko instance to connect to.')
    @CommandArgument('--type',
        default='browser',
        help='Test type, usually one of: browser, b2g, b2g-qemu.')
    @CommandArgument('--profile',
        help='Path to gecko profile to use.')
    @CommandArgument('--gecko-log',
        help='Path to gecko log file, or "-" for stdout.')
    @CommandArgument('--jsdebugger', action='store_true',
        help='Enable the jsdebugger for marionette javascript.')
    @CommandArgument('--pydebugger',
        help='Enable python post-mortem debugger when a test fails.'
             ' Pass in the debugger you want to use, eg pdb or ipdb.')
    @CommandArgument('--e10s', action='store_true',
        help='Enable electrolysis for marionette tests (desktop only).')
    @CommandArgument('tests', nargs='*', metavar='TESTS',
        help='Path to test(s) to run.')
    def run_marionette_test(self, tests, **kwargs):
        binary = self.get_binary_path('app')
        return run_marionette(tests, binary=binary, topsrcdir=self.topsrcdir, **kwargs)

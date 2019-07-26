






from __future__ import unicode_literals

import argparse
import codecs
import imp
import logging
import os
import sys
import uuid

from mozbuild.base import BuildConfig
from mozbuild.config import ConfigSettings
from mozbuild.logger import LoggingManager

from mach.registrar import populate_argument_parser




SETTINGS_PROVIDERS = [
    BuildConfig,
]



CONSUMED_ARGUMENTS = [
    'settings_file',
    'verbose',
    'logfile',
    'log_interval',
    'command',
    'cls',
    'method',
    'func',
]


class ArgumentParser(argparse.ArgumentParser):
    """Custom implementation argument parser to make things look pretty."""

    def error(self, message):
        """Custom error reporter to give more helpful text on bad commands."""
        if not message.startswith('argument command: invalid choice'):
            argparse.ArgumentParser.error(self, message)
            assert False

        print('Invalid command specified. The list of commands is below.\n')
        self.print_help()
        sys.exit(1)

    def format_help(self):
        text = argparse.ArgumentParser.format_help(self)

        
        
        
        
        
        
        search = 'Commands:\n  {'
        start = text.find(search)

        if start != -1:
            end = text.find('}\n', start)
            assert end != -1

            real_start = start + len('Commands:\n')
            real_end = end + len('}\n')

            text = text[0:real_start] + text[real_end:]

        return text


class Mach(object):
    """Contains code for the command-line `mach` interface."""

    USAGE = """%(prog)s [global arguments] command [command arguments]

mach (German for "do") is the main interface to the Mozilla build system and
common developer tasks.

You tell mach the command you want to perform and it does it for you.

Some common commands are:

    %(prog)s build     Build/compile the source tree.
    %(prog)s test      Run tests.
    %(prog)s help      Show full help, including the list of all commands.

To see more help for a specific command, run:

  %(prog)s <command> --help
"""

    def __init__(self, cwd):
        global MODULES_SCANNED

        assert os.path.isdir(cwd)

        self.cwd = cwd
        self.log_manager = LoggingManager()
        self.logger = logging.getLogger(__name__)
        self.settings = ConfigSettings()

        self.log_manager.register_structured_logger(self.logger)

    def load_commands_from_sys_path(self):
        """Discover and load mach command modules from sys.path.

        This iterates over all paths on sys.path. If the path contains a
        "mach/commands" subdirectory, all .py files in that directory will be
        loaded and examined for mach commands.
        """
        
        
        if b'mach.commands' not in sys.modules:
            mod = imp.new_module(b'mach.commands')
            sys.modules[b'mach.commands'] = mod

        for path in sys.path:
            
            commands_path = os.path.join(path, 'mach', 'commands')

            if not os.path.isdir(commands_path):
                continue

            self.load_commands_from_directory(commands_path)

    def load_commands_from_directory(self, path):
        """Scan for mach commands from modules in a directory.

        This takes a path to a directory, loads the .py files in it, and
        registers and found mach command providers with this mach instance.
        """
        for f in sorted(os.listdir(path)):
            if not f.endswith('.py') or f == '__init__.py':
                continue

            full_path = os.path.join(path, f)
            module_name = 'mach.commands.%s' % f[0:-3]

            self.load_commands_from_file(full_path, module_name=module_name)

    def load_commands_from_file(self, path, module_name=None):
        """Scan for mach commands from a file.

        This takes a path to a file and loads it as a Python module under the
        module name specified. If no name is specified, a random one will be
        chosen.
        """
        if module_name is None:
            module_name = 'mach.commands.%s' % uuid.uuid1().get_hex()

        imp.load_source(module_name, path)

    def run(self, argv):
        """Runs mach with arguments provided from the command line.

        Returns the integer exit code that should be used. 0 means success. All
        other values indicate failure.
        """

        
        
        
        
        
        orig_stdin = sys.stdin
        orig_stdout = sys.stdout
        orig_stderr = sys.stderr

        try:
            if sys.stdin.encoding is None:
                sys.stdin = codecs.getreader('utf-8')(sys.stdin)

            if sys.stdout.encoding is None:
                sys.stdout = codecs.getwriter('utf-8')(sys.stdout)

            if sys.stderr.encoding is None:
                sys.stderr = codecs.getwriter('utf-8')(sys.stderr)

            return self._run(argv)
        except KeyboardInterrupt:
            print('mach interrupted by signal or user action. Stopping.')
            return 1
        finally:
            sys.stdin = orig_stdin
            sys.stdout = orig_stdout
            sys.stderr = orig_stderr

    def _run(self, argv):
        parser = self.get_argument_parser()

        if not len(argv):
            
            
            
            parser.usage = Mach.USAGE
            parser.print_usage()
            return 0

        if argv[0] == 'help':
            parser.print_help()
            return 0

        args = parser.parse_args(argv)

        
        if args.logfile:
            self.log_manager.add_json_handler(args.logfile)

        
        log_level = logging.INFO
        if args.verbose:
            log_level = logging.DEBUG

        
        
        self.log_manager.add_terminal_logging(level=log_level,
            write_interval=args.log_interval)

        self.load_settings(args)
        conf = BuildConfig(self.settings)

        stripped = {k: getattr(args, k) for k in vars(args) if k not in
            CONSUMED_ARGUMENTS}

        
        
        if hasattr(args, 'cls'):
            cls = getattr(args, 'cls')
            instance = cls(self.cwd, self.settings, self.log_manager)
            fn = getattr(instance, getattr(args, 'method'))

        
        elif hasattr(args, 'func'):
            fn = getattr(args, 'func')
        else:
            raise Exception('Dispatch configuration error in module.')

        result = fn(**stripped)

        if not result:
            result = 0

        assert isinstance(result, int)

        return result

    def log(self, level, action, params, format_str):
        """Helper method to record a structured log event."""
        self.logger.log(level, format_str,
            extra={'action': action, 'params': params})

    def load_settings(self, args):
        """Determine which settings files apply and load them.

        Currently, we only support loading settings from a single file.
        Ideally, we support loading from multiple files. This is supported by
        the ConfigSettings API. However, that API currently doesn't track where
        individual values come from, so if we load from multiple sources then
        save, we effectively do a full copy. We don't want this. Until
        ConfigSettings does the right thing, we shouldn't expose multi-file
        loading.

        We look for a settings file in the following locations. The first one
        found wins:

          1) Command line argument
          2) Environment variable
          3) Default path
        """
        for provider in SETTINGS_PROVIDERS:
            provider.register_settings()
            self.settings.register_provider(provider)

        p = os.path.join(self.cwd, 'mach.ini')

        if args.settings_file:
            p = args.settings_file
        elif 'MACH_SETTINGS_FILE' in os.environ:
            p = os.environ['MACH_SETTINGS_FILE']

        self.settings.load_file(p)

        return os.path.exists(p)

    def get_argument_parser(self):
        """Returns an argument parser for the command-line interface."""

        parser = ArgumentParser(add_help=False,
            usage='%(prog)s [global arguments] command [command arguments]')

        
        
        subparser = parser.add_subparsers(dest='command', title='Commands')
        parser.set_defaults(command='help')

        global_group = parser.add_argument_group('Global Arguments')

        global_group.add_argument('-h', '--help', action='help',
            help='Show this help message and exit.')

        global_group.add_argument('--settings', dest='settings_file',
            metavar='FILENAME', help='Path to settings file.')

        global_group.add_argument('-v', '--verbose', dest='verbose',
            action='store_true', default=False,
            help='Print verbose output.')
        global_group.add_argument('-l', '--log-file', dest='logfile',
            metavar='FILENAME', type=argparse.FileType('ab'),
            help='Filename to write log data to.')
        global_group.add_argument('--log-interval', dest='log_interval',
            action='store_true', default=False,
            help='Prefix log line with interval from last message rather '
                'than relative time. Note that this is NOT execution time '
                'if there are parallel operations.')

        populate_argument_parser(subparser)

        return parser

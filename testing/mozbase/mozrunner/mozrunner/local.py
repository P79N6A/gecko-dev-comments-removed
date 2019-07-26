





import ConfigParser
import mozinfo
import optparse
import os
import platform
import subprocess
import sys

if mozinfo.isMac:
    from plistlib import readPlist

from mozprofile import Profile, FirefoxProfile, MetroFirefoxProfile, ThunderbirdProfile, MozProfileCLI

from .base import Runner
from .utils import findInPath, get_metadata_from_egg


__all__ = ['CLI',
           'cli',
           'LocalRunner',
           'local_runners',
           'package_metadata',
           'FirefoxRunner',
           'MetroFirefoxRunner',
           'ThunderbirdRunner']


package_metadata = get_metadata_from_egg('mozrunner')




debuggers = {'gdb': {'interactive': True,
                     'args': ['-q', '--args'],},
             'valgrind': {'interactive': False,
                          'args': ['--leak-check=full']}
             }


def debugger_arguments(debugger, arguments=None, interactive=None):
    """Finds debugger arguments from debugger given and defaults

    :param debugger: name or path to debugger
    :param arguments: arguments for the debugger, or None to use defaults
    :param interactive: whether the debugger should run in interactive mode

    """
    
    executable = debugger
    if not os.path.exists(executable):
        executable = findInPath(debugger)
    if executable is None:
        raise Exception("Path to '%s' not found" % debugger)

    
    dirname, debugger = os.path.split(debugger)
    if debugger not in debuggers:
        return ([executable] + (arguments or []), bool(interactive))

    
    if arguments is None:
        arguments = debuggers[debugger].get('args', [])
    if interactive is None:
        interactive = debuggers[debugger].get('interactive', False)
    return ([executable] + arguments, interactive)


class LocalRunner(Runner):
    """Handles all running operations. Finds bins, runs and kills the process"""

    profile_class = Profile 

    @classmethod
    def create(cls, binary=None, cmdargs=None, env=None, kp_kwargs=None, profile_args=None,
               clean_profile=True, process_class=None, **kwargs):
        profile = cls.profile_class(**(profile_args or {}))
        return cls(profile, binary=binary, cmdargs=cmdargs, env=env, kp_kwargs=kp_kwargs,
                                           clean_profile=clean_profile, process_class=process_class, **kwargs)

    def __init__(self, profile, binary, cmdargs=None, env=None,
                 kp_kwargs=None, clean_profile=None, process_class=None, **kwargs):

        Runner.__init__(self, profile, clean_profile=clean_profile, kp_kwargs=kp_kwargs,
                        process_class=process_class, env=env, **kwargs)

        
        self.binary = binary
        if not self.binary:
            raise Exception("Binary not specified")
        if not os.path.exists(self.binary):
            raise OSError("Binary path does not exist: %s" % self.binary)

        
        self.binary = os.path.abspath(self.binary)

        
        plist = '%s/Contents/Info.plist' % self.binary
        if mozinfo.isMac and os.path.exists(plist):
            info = readPlist(plist)
            self.binary = os.path.join(self.binary, "Contents/MacOS/",
                                       info['CFBundleExecutable'])

        self.cmdargs = cmdargs or []
        _cmdargs = [i for i in self.cmdargs
                    if i != '-foreground']
        if len(_cmdargs) != len(self.cmdargs):
            
            
            self.cmdargs = _cmdargs
            self.cmdargs.append('-foreground')
        if mozinfo.isMac and '-foreground' not in self.cmdargs:
            
            
            self.cmdargs.append('-foreground')

        
        if env is None:
            self.env = os.environ.copy()
        else:
            self.env = env.copy()
        
        self.env['MOZ_NO_REMOTE'] = '1'
        
        self.env['NO_EM_RESTART'] = '1'

        
        if sys.platform == 'linux2' and self.binary.endswith('-bin'):
            dirname = os.path.dirname(self.binary)
            if os.environ.get('LD_LIBRARY_PATH', None):
                self.env['LD_LIBRARY_PATH'] = '%s:%s' % (os.environ['LD_LIBRARY_PATH'], dirname)
            else:
                self.env['LD_LIBRARY_PATH'] = dirname

    @property
    def command(self):
        """Returns the command list to run"""
        commands = [self.binary, '-profile', self.profile.profile]

        
        commands[1:1] = self.cmdargs

        
        
        
        if mozinfo.isMac and hasattr(platform, 'mac_ver') and \
                platform.mac_ver()[0][:4] < '10.6':
            commands = ["arch", "-arch", "i386"] + commands

        return commands

    def get_repositoryInfo(self):
        """Read repository information from application.ini and platform.ini"""
        config = ConfigParser.RawConfigParser()
        dirname = os.path.dirname(self.binary)
        repository = { }

        for file, section in [('application', 'App'), ('platform', 'Build')]:
            config.read(os.path.join(dirname, '%s.ini' % file))

            for key, id in [('SourceRepository', 'repository'),
                            ('SourceStamp', 'changeset')]:
                try:
                    repository['%s_%s' % (file, id)] = config.get(section, key);
                except:
                    repository['%s_%s' % (file, id)] = None

        return repository


class FirefoxRunner(LocalRunner):
    """Specialized LocalRunner subclass for running Firefox."""

    profile_class = FirefoxProfile

    def __init__(self, profile, binary=None, **kwargs):

        
        binary = binary or os.environ.get('BROWSER_PATH')
        LocalRunner.__init__(self, profile, binary, **kwargs)


class MetroFirefoxRunner(LocalRunner):
    """Specialized LocalRunner subclass for running Firefox Metro"""

    profile_class = MetroFirefoxProfile

    
    here = os.path.dirname(os.path.abspath(__file__))
    immersiveHelperPath = os.path.sep.join([here,
                                            'resources',
                                            'metrotestharness.exe'])

    def __init__(self, profile, binary=None, **kwargs):

        
        binary = binary or os.environ.get('BROWSER_PATH')
        LocalRunner.__init__(self, profile, binary, **kwargs)

        if not os.path.exists(self.immersiveHelperPath):
            raise OSError('Can not find Metro launcher: %s' % self.immersiveHelperPath)

        if not mozinfo.isWin:
            raise Exception('Firefox Metro mode is only supported on Windows 8 and onwards')

    @property
    def command(self):
       command = LocalRunner.command.fget(self)
       command[:0] = [self.immersiveHelperPath, '-firefoxpath']

       return command


class ThunderbirdRunner(LocalRunner):
    """Specialized LocalRunner subclass for running Thunderbird"""
    profile_class = ThunderbirdProfile


local_runners = {'firefox': FirefoxRunner,
                 'metrofirefox' : MetroFirefoxRunner,
                 'thunderbird': ThunderbirdRunner}


class CLI(MozProfileCLI):
    """Command line interface"""

    module = "mozrunner"

    def __init__(self, args=sys.argv[1:]):
        self.metadata = getattr(sys.modules[self.module],
                                'package_metadata',
                                {})
        version = self.metadata.get('Version')
        parser_args = {'description': self.metadata.get('Summary')}
        if version:
            parser_args['version'] = "%prog " + version
        self.parser = optparse.OptionParser(**parser_args)
        self.add_options(self.parser)
        (self.options, self.args) = self.parser.parse_args(args)

        if getattr(self.options, 'info', None):
            self.print_metadata()
            sys.exit(0)

        
        try:
            self.runner_class = local_runners[self.options.app]
        except KeyError:
            self.parser.error('Application "%s" unknown (should be one of "%s")' %
                              (self.options.app, ', '.join(local_runners.keys())))

    def add_options(self, parser):
        """add options to the parser"""

        
        MozProfileCLI.add_options(self, parser)

        
        parser.add_option('-b', "--binary",
                          dest="binary", help="Binary path.",
                          metavar=None, default=None)
        parser.add_option('--app', dest='app', default='firefox',
                          help="Application to use [DEFAULT: %default]")
        parser.add_option('--app-arg', dest='appArgs',
                          default=[], action='append',
                          help="provides an argument to the test application")
        parser.add_option('--debugger', dest='debugger',
                          help="run under a debugger, e.g. gdb or valgrind")
        parser.add_option('--debugger-args', dest='debugger_args',
                          action='store',
                          help="arguments to the debugger")
        parser.add_option('--interactive', dest='interactive',
                          action='store_true',
                          help="run the program interactively")
        if self.metadata:
            parser.add_option("--info", dest="info", default=False,
                              action="store_true",
                              help="Print module information")

    

    def get_metadata_from_egg(self):
        import pkg_resources
        ret = {}
        dist = pkg_resources.get_distribution(self.module)
        if dist.has_metadata("PKG-INFO"):
            for line in dist.get_metadata_lines("PKG-INFO"):
                key, value = line.split(':', 1)
                ret[key] = value
        if dist.has_metadata("requires.txt"):
            ret["Dependencies"] = "\n" + dist.get_metadata("requires.txt")
        return ret

    def print_metadata(self, data=("Name", "Version", "Summary", "Home-page",
                                   "Author", "Author-email", "License", "Platform", "Dependencies")):
        for key in data:
            if key in self.metadata:
                print key + ": " + self.metadata[key]

    

    def command_args(self):
        """additional arguments for the mozilla application"""
        return map(os.path.expanduser, self.options.appArgs)

    def runner_args(self):
        """arguments to instantiate the runner class"""
        return dict(cmdargs=self.command_args(),
                    binary=self.options.binary,
                    profile_args=self.profile_args())

    def create_runner(self):
        return self.runner_class.create(**self.runner_args())

    def run(self):
        runner = self.create_runner()
        self.start(runner)
        runner.cleanup()

    def debugger_arguments(self):
        """Get the debugger arguments

        returns a 2-tuple of debugger arguments:
            (debugger_arguments, interactive)

        """
        debug_args = self.options.debugger_args
        if debug_args is not None:
            debug_args = debug_args.split()
        interactive = self.options.interactive
        if self.options.debugger:
            debug_args, interactive = debugger_arguments(self.options.debugger, debug_args, interactive)
        return debug_args, interactive

    def start(self, runner):
        """Starts the runner and waits for the application to exit

        It can also happen via a keyboard interrupt. It should be
        overwritten to provide custom running of the runner instance.

        """
        
        debug_args, interactive = self.debugger_arguments()
        runner.start(debug_args=debug_args, interactive=interactive)
        print 'Starting: ' + ' '.join(runner.command)
        try:
            runner.wait()
        except KeyboardInterrupt:
            runner.stop()


def cli(args=sys.argv[1:]):
    CLI(args).run()


if __name__ == '__main__':
    cli()

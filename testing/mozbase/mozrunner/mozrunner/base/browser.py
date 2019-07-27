



import mozinfo
import os
import platform
import sys

from .runner import BaseRunner


class GeckoRuntimeRunner(BaseRunner):
    """
    The base runner class used for local gecko runtime binaries,
    such as Firefox and Thunderbird.
    """

    def __init__(self, binary, cmdargs=None, **runner_args):
        BaseRunner.__init__(self, **runner_args)

        self.binary = binary
        self.cmdargs = cmdargs or []

        
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
        command = [self.binary, '-profile', self.profile.profile]

        _cmdargs = [i for i in self.cmdargs
                    if i != '-foreground']
        if len(_cmdargs) != len(self.cmdargs):
            
            
            self.cmdargs = _cmdargs
            self.cmdargs.append('-foreground')
        if mozinfo.isMac and '-foreground' not in self.cmdargs:
            
            
            self.cmdargs.append('-foreground')

        
        command[1:1] = self.cmdargs

        if hasattr(self.app_ctx, 'wrap_command'):
            command = self.app_ctx.wrap_command(command)
        return command

    def start(self, *args, **kwargs):
        
        if not self.profile.exists():
            self.profile.reset()
            assert self.profile.exists(), "%s : failure to reset profile" % self.__class__.__name__

        has_debugger = "debug_args" in kwargs and kwargs["debug_args"]
        if has_debugger:
            self.env["MOZ_CRASHREPORTER_DISABLE"] = "1"
        else:
            self.env["MOZ_CRASHREPORTER_NO_REPORT"] = "1"
            self.env["MOZ_CRASHREPORTER"] = "1"


        BaseRunner.start(self, *args, **kwargs)

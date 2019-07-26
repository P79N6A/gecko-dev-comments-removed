




import subprocess

from mozprocess.processhandler import ProcessHandler
import mozlog



def abstractmethod(method):
  line = method.func_code.co_firstlineno
  filename = method.func_code.co_filename
  def not_implemented(*args, **kwargs):
    raise NotImplementedError('Abstract method %s at File "%s", line %s '
                              'should be implemented by a concrete class' %
                              (repr(method), filename, line))
  return not_implemented

class Runner(object):

    def __init__(self, profile, clean_profile=True, process_class=None, kp_kwargs=None, env=None):
        self.clean_profile = clean_profile
        self.env = env or {}
        self.kp_kwargs = kp_kwargs or {}
        self.process_class = process_class or ProcessHandler
        self.process_handler = None
        self.profile = profile
        self.log = mozlog.getLogger('MozRunner')

    @abstractmethod
    def start(self, *args, **kwargs):
        """
        Run the process
        """

        
        self.stop()

        
        if not self.profile.exists():
            self.profile.reset()
            assert self.profile.exists(), "%s : failure to reset profile" % self.__class__.__name__

        cmd = self._wrap_command(self.command)

        
        if debug_args:
            cmd = list(debug_args) + cmd

        if interactive:
            self.process_handler = subprocess.Popen(cmd, env=self.env)
            
        else:
            
            self.process_handler = self.process_class(cmd, env=self.env, **self.kp_kwargs)
            self.process_handler.run(timeout, outputTimeout)

    def wait(self, timeout=None):
        """
        Wait for the process to exit.
        Returns the process return code if the process exited,
        returns None otherwise.

        If timeout is not None, will return after timeout seconds.
        Use is_running() to determine whether or not a timeout occured.
        Timeout is ignored if interactive was set to True.
        """
        if self.process_handler is None:
            return

        if isinstance(self.process_handler, subprocess.Popen):
            return_code = self.process_handler.wait()
        else:
            self.process_handler.wait(timeout)
            return_code = self.process_handler.proc.poll()
            if return_code is not None:
                self.process_handler = None
        return return_code

    def is_running(self):
        """
        Returns True if the process is still running, False otherwise
        """
        return self.process_handler is not None


    def stop(self):
        """
        Kill the process
        """
        if self.process_handler is None:
            return
        self.process_handler.kill()
        self.process_handler = None

    def reset(self):
        """
        Reset the runner to its default state
        """
        if getattr(self, 'profile', False):
            self.profile.reset()

    def cleanup(self):
        """
        Cleanup all runner state
        """
        if self.is_running():
            self.stop()
        if getattr(self, 'profile', False) and self.clean_profile:
            self.profile.cleanup()

    __del__ = cleanup

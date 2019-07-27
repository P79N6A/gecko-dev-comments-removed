



































"""killableprocess - Subprocesses which can be reliably killed

This module is a subclass of the builtin "subprocess" module. It allows
processes that launch subprocesses to be reliably killed on Windows (via the Popen.kill() method.

It also adds a timeout argument to Wait() for a limited period of time before
forcefully killing the process.

Note: On Windows, this module requires Windows 2000 or higher (no support for
Windows 95, 98, or NT 4.0). It also requires ctypes, which is bundled with
Python 2.5+ or available from http://python.net/crew/theller/ctypes/
"""

import subprocess
import sys
import os
import time
import datetime
import types
import exceptions

try:
    from subprocess import CalledProcessError
except ImportError:
    
    class CalledProcessError(Exception):
        """This exception is raised when a process run by check_call() returns
        a non-zero exit status. The exit status will be stored in the
        returncode attribute."""
        def __init__(self, returncode, cmd):
            self.returncode = returncode
            self.cmd = cmd
        def __str__(self):
            return "Command '%s' returned non-zero exit status %d" % (self.cmd, self.returncode)

mswindows = (sys.platform == "win32")

if mswindows:
    import winprocess
else:
    import signal




STILL_ACTIVE = 259

def call(*args, **kwargs):
    waitargs = {}
    if "timeout" in kwargs:
        waitargs["timeout"] = kwargs.pop("timeout")

    return Popen(*args, **kwargs).wait(**waitargs)

def check_call(*args, **kwargs):
    """Call a program with an optional timeout. If the program has a non-zero
    exit status, raises a CalledProcessError."""

    retcode = call(*args, **kwargs)
    if retcode:
        cmd = kwargs.get("args")
        if cmd is None:
            cmd = args[0]
        raise CalledProcessError(retcode, cmd)

if not mswindows:
    def DoNothing(*args):
        pass

class Popen(subprocess.Popen):
    kill_called = False
    if mswindows:
        def _execute_child(self, *args_tuple):
            
            if sys.hexversion < 0x02070600: 
                (args, executable, preexec_fn, close_fds,
                    cwd, env, universal_newlines, startupinfo,
                    creationflags, shell,
                    p2cread, p2cwrite,
                    c2pread, c2pwrite,
                    errread, errwrite) = args_tuple
                to_close = set()
            else: 
                (args, executable, preexec_fn, close_fds,
                    cwd, env, universal_newlines, startupinfo,
                    creationflags, shell, to_close,
                    p2cread, p2cwrite,
                    c2pread, c2pwrite,
                    errread, errwrite) = args_tuple

            if not isinstance(args, types.StringTypes):
                args = subprocess.list2cmdline(args)

            
            creationflags |= winprocess.CREATE_NEW_PROCESS_GROUP

            if startupinfo is None:
                startupinfo = winprocess.STARTUPINFO()

            if None not in (p2cread, c2pwrite, errwrite):
                startupinfo.dwFlags |= winprocess.STARTF_USESTDHANDLES

                startupinfo.hStdInput = int(p2cread)
                startupinfo.hStdOutput = int(c2pwrite)
                startupinfo.hStdError = int(errwrite)
            if shell:
                startupinfo.dwFlags |= winprocess.STARTF_USESHOWWINDOW
                startupinfo.wShowWindow = winprocess.SW_HIDE
                comspec = os.environ.get("COMSPEC", "cmd.exe")
                args = comspec + " /c " + args

            
            canCreateJob = winprocess.CanCreateJobObject()

            
            creationflags |= winprocess.CREATE_SUSPENDED
            creationflags |= winprocess.CREATE_UNICODE_ENVIRONMENT
            if canCreateJob:
                
                
                creationflags |= winprocess.CREATE_BREAKAWAY_FROM_JOB

            
            hp, ht, pid, tid = winprocess.CreateProcess(
                executable, args,
                None, None, 
                1, 
                creationflags,
                winprocess.EnvironmentBlock(env),
                cwd, startupinfo)
            self._child_created = True
            self._handle = hp
            self._thread = ht
            self.pid = pid
            self.tid = tid

            if canCreateJob:
                
                
                self._job = winprocess.CreateJobObject()
                winprocess.AssignProcessToJobObject(self._job, int(hp))
            else:
                self._job = None

            winprocess.ResumeThread(int(ht))
            ht.Close()

            if p2cread is not None:
                p2cread.Close()
            if c2pwrite is not None:
                c2pwrite.Close()
            if errwrite is not None:
                errwrite.Close()
            time.sleep(.1)

    def kill(self, group=True):
        """Kill the process. If group=True, all sub-processes will also be killed."""
        self.kill_called = True

        if mswindows:
            if group and self._job:
                winprocess.TerminateJobObject(self._job, 127)
            else:
                winprocess.TerminateProcess(self._handle, 127)
            self.returncode = 127
        else:
            if group:
                try:
                    os.killpg(self.pid, signal.SIGKILL)
                except: pass
            else:
                os.kill(self.pid, signal.SIGKILL)
            self.returncode = -9

    def wait(self, timeout=None, group=True):
        """Wait for the process to terminate. Returns returncode attribute.
        If timeout seconds are reached and the process has not terminated,
        it will be forcefully killed. If timeout is -1, wait will not
        time out."""
        if timeout is not None:
            
            timeout = timeout * 1000

        starttime = datetime.datetime.now()

        if mswindows:
            if timeout is None:
                timeout = -1
            rc = winprocess.WaitForSingleObject(self._handle, timeout)

            if (rc == winprocess.WAIT_OBJECT_0 or
                rc == winprocess.WAIT_ABANDONED or
                rc == winprocess.WAIT_FAILED):
                
                
                
                

                
                def check():
                    now = datetime.datetime.now()
                    diff = now - starttime
                    if (diff.seconds * 1000000 + diff.microseconds) < (timeout * 1000):    
                        if self._job:
                            if (winprocess.QueryInformationJobObject(self._job, 8)['BasicInfo']['ActiveProcesses'] > 0):
                                
                                return 1
                        else:
                            
                            self.returncode = winprocess.GetExitCodeProcess(self._handle)
                            if (self.returncode == STILL_ACTIVE):
                                
                                return 1
                        
                        return 0
                    else:
                        
                        return -1

                notdone = check()
                while notdone == 1:
                    time.sleep(.5)
                    notdone = check()

                if notdone == -1:
                    
                    
                    self.kill(group)
                                
            else:
                
                
                self.kill(group)

        else:
            if sys.platform in ('linux2', 'sunos5', 'solaris') \
                    or sys.platform.startswith('freebsd'):
                def group_wait(timeout):
                    try:
                        os.waitpid(self.pid, 0)
                    except OSError, e:
                        pass 
                    return self.returncode
            elif sys.platform == 'darwin':
                def group_wait(timeout):
                    try:
                        count = 0
                        if timeout is None and self.kill_called:
                            timeout = 10 
                        if timeout is None:
                            while 1:
                                os.killpg(self.pid, signal.SIG_DFL)
                        while ((count * 2) <= timeout):
                            os.killpg(self.pid, signal.SIG_DFL)
                            
                            time.sleep(.5); count += 500
                    except exceptions.OSError:
                        return self.returncode
                        
            if timeout is None:
                if group is True:
                    return group_wait(timeout)
                else:
                    subprocess.Popen.wait(self)
                    return self.returncode

            returncode = False

            now = datetime.datetime.now()
            diff = now - starttime
            while (diff.seconds * 1000 * 1000 + diff.microseconds) < (timeout * 1000) and ( returncode is False ):
                if group is True:
                    return group_wait(timeout)
                else:
                    if subprocess.poll() is not None:
                        returncode = self.returncode
                time.sleep(.5)
                now = datetime.datetime.now()
                diff = now - starttime
            return self.returncode

        return self.returncode
    
    __del__ = lambda self: None        
        
def setpgid_preexec_fn():
    os.setpgid(0, 0)
   
def runCommand(cmd, **kwargs):
    if sys.platform != "win32":
        return Popen(cmd, preexec_fn=setpgid_preexec_fn, **kwargs)
    else:
        return Popen(cmd, **kwargs)

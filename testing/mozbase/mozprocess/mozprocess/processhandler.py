



import os
import select
import signal
import subprocess
import sys
import threading
import time
import traceback
from Queue import Queue
from datetime import datetime
__all__ = ['ProcessHandlerMixin', 'ProcessHandler']


MOZPROCESS_DEBUG = os.getenv("MOZPROCESS_DEBUG")


isWin = os.name == "nt"
isPosix = os.name == "posix" 

if isWin:
    import ctypes, ctypes.wintypes, msvcrt
    from ctypes import sizeof, addressof, c_ulong, byref, WinError, c_longlong
    import winprocess
    from qijo import JobObjectAssociateCompletionPortInformation,\
    JOBOBJECT_ASSOCIATE_COMPLETION_PORT, JobObjectExtendedLimitInformation,\
    JOBOBJECT_BASIC_LIMIT_INFORMATION, JOBOBJECT_EXTENDED_LIMIT_INFORMATION, IO_COUNTERS

class ProcessHandlerMixin(object):
    """
    A class for launching and manipulating local processes.

    :param cmd: command to run. May be a string or a list. If specified as a list, the first element will be interpreted as the command, and all additional elements will be interpreted as arguments to that command.
    :param args: list of arguments to pass to the command (defaults to None). Must not be set when `cmd` is specified as a list.
    :param cwd: working directory for command (defaults to None).
    :param env: is the environment to use for the process (defaults to os.environ).
    :param ignore_children: causes system to ignore child processes when True, defaults to False (which tracks child processes).
    :param kill_on_timeout: when True, the process will be killed when a timeout is reached. When False, the caller is responsible for killing the process. Failure to do so could cause a call to wait() to hang indefinitely. (Defaults to True.)
    :param processOutputLine: function or list of functions to be called for each line of output produced by the process (defaults to None).
    :param onTimeout: function or list of functions to be called when the process times out.
    :param onFinish: function or list of functions to be called when the process terminates normally without timing out.
    :param kwargs: additional keyword args to pass directly into Popen.

    NOTE: Child processes will be tracked by default.  If for any reason
    we are unable to track child processes and ignore_children is set to False,
    then we will fall back to only tracking the root process.  The fallback
    will be logged.
    """

    class Process(subprocess.Popen):
        """
        Represents our view of a subprocess.
        It adds a kill() method which allows it to be stopped explicitly.
        """

        MAX_IOCOMPLETION_PORT_NOTIFICATION_DELAY = 180
        MAX_PROCESS_KILL_DELAY = 30

        def __init__(self,
                     args,
                     bufsize=0,
                     executable=None,
                     stdin=None,
                     stdout=None,
                     stderr=None,
                     preexec_fn=None,
                     close_fds=False,
                     shell=False,
                     cwd=None,
                     env=None,
                     universal_newlines=False,
                     startupinfo=None,
                     creationflags=0,
                     ignore_children=False):

            
            self._ignore_children = ignore_children

            if not self._ignore_children and not isWin:
                
                
                
                
                def setpgidfn():
                    os.setpgid(0, 0)
                preexec_fn = setpgidfn

            try:
                subprocess.Popen.__init__(self, args, bufsize, executable,
                                          stdin, stdout, stderr,
                                          preexec_fn, close_fds,
                                          shell, cwd, env,
                                          universal_newlines, startupinfo, creationflags)
            except OSError:
                print >> sys.stderr, args
                raise

        def __del__(self, _maxint=sys.maxint):
            if isWin:
                handle = getattr(self, '_handle', None)
                if handle:
                    if hasattr(self, '_internal_poll'):
                        self._internal_poll(_deadstate=_maxint)
                    else:
                        self.poll(_deadstate=sys.maxint)
                if handle or self._job or self._io_port:
                    self._cleanup()
            else:
                subprocess.Popen.__del__(self)

        def kill(self, sig=None):
            if isWin:
                if not self._ignore_children and self._handle and self._job:
                    winprocess.TerminateJobObject(self._job, winprocess.ERROR_CONTROL_C_EXIT)
                    self.returncode = winprocess.GetExitCodeProcess(self._handle)
                elif self._handle:
                    err = None
                    try:
                        winprocess.TerminateProcess(self._handle, winprocess.ERROR_CONTROL_C_EXIT)
                    except:
                        err = "Could not terminate process"
                    winprocess.GetExitCodeProcess(self._handle)
                    self._cleanup()
                    if err is not None:
                        raise OSError(err)
            else:
                sig = sig or signal.SIGKILL
                if not self._ignore_children:
                    try:
                        os.killpg(self.pid, sig)
                    except BaseException, e:
                        if getattr(e, "errno", None) != 3:
                            
                            print >> sys.stdout, "Could not kill process, could not find pid: %s, assuming it's already dead" % self.pid
                else:
                    os.kill(self.pid, sig)

            self.returncode = self.wait()
            self._cleanup()
            return self.returncode

        def poll(self):
            """ Popen.poll
                Check if child process has terminated. Set and return returncode attribute.
            """
            
            if isWin and getattr(self, '_handle', None):
                return None

            return subprocess.Popen.poll(self)

        def wait(self):
            """ Popen.wait
                Called to wait for a running process to shut down and return
                its exit code
                Returns the main process's exit code
            """
            
            self.returncode = self._wait()
            self._cleanup()
            return self.returncode

        """ Private Members of Process class """

        if isWin:
            
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
                if not isinstance(args, basestring):
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

                
                if not canCreateJob and not self._ignore_children:
                    
                    
                    print >> sys.stderr, "ProcessManager UNABLE to use job objects to manage child processes"

                
                creationflags |= winprocess.CREATE_SUSPENDED
                creationflags |= winprocess.CREATE_UNICODE_ENVIRONMENT
                if canCreateJob:
                    creationflags |= winprocess.CREATE_BREAKAWAY_FROM_JOB
                else:
                    
                    
                    print "ProcessManager NOT managing child processes"

                
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

                if not self._ignore_children and canCreateJob:
                    try:
                        
                        
                        
                        self._io_port = winprocess.CreateIoCompletionPort()
                        self._job = winprocess.CreateJobObject()

                        
                        joacp = JOBOBJECT_ASSOCIATE_COMPLETION_PORT(winprocess.COMPKEY_JOBOBJECT,
                                                                    self._io_port)
                        winprocess.SetInformationJobObject(self._job,
                                                          JobObjectAssociateCompletionPortInformation,
                                                          addressof(joacp),
                                                          sizeof(joacp)
                                                          )

                        
                        
                        jbli = JOBOBJECT_BASIC_LIMIT_INFORMATION(
                                                c_longlong(0), 
                                                c_longlong(0), 
                                                winprocess.JOB_OBJECT_LIMIT_BREAKAWAY_OK,
                                                0, 
                                                0, 
                                                0, 
                                                None, 
                                                0, 
                                                0, 
                                                )

                        iocntr = IO_COUNTERS()
                        jeli = JOBOBJECT_EXTENDED_LIMIT_INFORMATION(
                                                jbli, 
                                                iocntr,    
                                                0,    
                                                0,    
                                                0,    
                                                0)    

                        winprocess.SetInformationJobObject(self._job,
                                                           JobObjectExtendedLimitInformation,
                                                           addressof(jeli),
                                                           sizeof(jeli)
                                                           )

                        
                        winprocess.AssignProcessToJobObject(self._job, int(hp))

                        
                        
                        self._process_events = Queue()

                        
                        self._procmgrthread = threading.Thread(target = self._procmgr)
                    except:
                        print >> sys.stderr, """Exception trying to use job objects;
falling back to not using job objects for managing child processes"""
                        tb = traceback.format_exc()
                        print >> sys.stderr, tb
                        
                        self._cleanup_job_io_port()
                else:
                    self._job = None

                winprocess.ResumeThread(int(ht))
                if getattr(self, '_procmgrthread', None):
                    self._procmgrthread.start()
                ht.Close()

                for i in (p2cread, c2pwrite, errwrite):
                    if i is not None:
                        i.Close()

            
            
            def _procmgr(self):
                if not (self._io_port) or not (self._job):
                    return

                try:
                    self._poll_iocompletion_port()
                except KeyboardInterrupt:
                    raise KeyboardInterrupt

            def _poll_iocompletion_port(self):
                
                self._spawned_procs = {}
                countdowntokill = 0

                if MOZPROCESS_DEBUG:
                    print "DBG::MOZPROC Self.pid value is: %s" % self.pid

                while True:
                    msgid = c_ulong(0)
                    compkey = c_ulong(0)
                    pid = c_ulong(0)
                    portstatus = winprocess.GetQueuedCompletionStatus(self._io_port,
                                                                      byref(msgid),
                                                                      byref(compkey),
                                                                      byref(pid),
                                                                      5000)

                    
                    
                    if countdowntokill != 0:
                        diff = datetime.now() - countdowntokill
                        
                        
                        
                        
                        
                        if diff.seconds > self.MAX_IOCOMPLETION_PORT_NOTIFICATION_DELAY:
                            print >> sys.stderr, "Parent process %s exited with children alive:" % self.pid
                            print >> sys.stderr, "PIDS: %s" %  ', '.join([str(i) for i in self._spawned_procs])
                            print >> sys.stderr, "Attempting to kill them..."
                            self.kill()
                            self._process_events.put({self.pid: 'FINISHED'})

                    if not portstatus:
                        
                        errcode = winprocess.GetLastError()
                        if errcode == winprocess.ERROR_ABANDONED_WAIT_0:
                            
                            print >> sys.stderr, "IO Completion Port unexpectedly closed"
                            break
                        elif errcode == winprocess.WAIT_TIMEOUT:
                            
                            continue
                        else:
                            print >> sys.stderr, "Error Code %s trying to query IO Completion Port, exiting" % errcode
                            raise WinError(errcode)
                            break

                    if compkey.value == winprocess.COMPKEY_TERMINATE.value:
                        if MOZPROCESS_DEBUG:
                            print "DBG::MOZPROC compkeyterminate detected"
                        
                        break

                    
                    if compkey.value == winprocess.COMPKEY_JOBOBJECT.value:
                        if msgid.value == winprocess.JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO:
                            
                            
                            if MOZPROCESS_DEBUG:
                                print "DBG::MOZPROC job object msg active processes zero"
                            self._process_events.put({self.pid: 'FINISHED'})
                            break
                        elif msgid.value == winprocess.JOB_OBJECT_MSG_NEW_PROCESS:
                            
                            
                            
                            if pid.value != self.pid:
                                self._spawned_procs[pid.value] = 1
                                if MOZPROCESS_DEBUG:
                                    print "DBG::MOZPROC new process detected with pid value: %s" % pid.value
                        elif msgid.value == winprocess.JOB_OBJECT_MSG_EXIT_PROCESS:
                            if MOZPROCESS_DEBUG:
                                print "DBG::MOZPROC process id %s exited normally" % pid.value
                            
                            if pid.value == self.pid and len(self._spawned_procs) > 0:
                                
                                countdowntokill = datetime.now()
                            elif pid.value in self._spawned_procs:
                                
                                del(self._spawned_procs[pid.value])
                        elif msgid.value == winprocess.JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS:
                            
                            if MOZPROCESS_DEBUG:
                                print "DBG::MOZPROC process id %s existed abnormally" % pid.value
                            if pid.value == self.pid and len(self._spawned_procs) > 0:
                                
                                countdowntokill = datetime.now()
                            elif pid.value in self._spawned_procs:
                                
                                del self._spawned_procs[pid.value]
                        else:
                            
                            if MOZPROCESS_DEBUG:
                                print "DBG::MOZPROC We got a message %s" % msgid.value
                            pass

            def _wait(self):

                
                if self._handle:
                    self.returncode = winprocess.GetExitCodeProcess(self._handle)
                else:
                    
                    return self.returncode

                
                threadalive = False
                if hasattr(self, "_procmgrthread"):
                    if hasattr(self._procmgrthread, 'is_alive'):
                        threadalive = self._procmgrthread.is_alive()
                    else:
                        threadalive = self._procmgrthread.isAlive()
                if self._job and threadalive:
                    
                    
                    
                    
                    
                    
                    err = None
                    try:
                        
                        
                        item = self._process_events.get(timeout=self.MAX_IOCOMPLETION_PORT_NOTIFICATION_DELAY +
                                                                self.MAX_PROCESS_KILL_DELAY)
                        if item[self.pid] == 'FINISHED':
                            self._process_events.task_done()
                    except:
                        err = "IO Completion Port failed to signal process shutdown"
                    
                    if self._handle:
                        self.returncode = winprocess.GetExitCodeProcess(self._handle)
                    self._cleanup()

                    if err is not None:
                        raise OSError(err)


                else:
                    
                    

                    if MOZPROCESS_DEBUG and not self._ignore_children:
                        print "DBG::MOZPROC NOT USING JOB OBJECTS!!!"
                    
                    if self.returncode != winprocess.STILL_ACTIVE:
                        self._cleanup()
                        return self.returncode

                    rc = None
                    if self._handle:
                        rc = winprocess.WaitForSingleObject(self._handle, -1)

                    if rc == winprocess.WAIT_TIMEOUT:
                        
                        print "Timed out waiting for process to close, attempting TerminateProcess"
                        self.kill()
                    elif rc == winprocess.WAIT_OBJECT_0:
                        
                        print "Single process terminated successfully"
                        self.returncode = winprocess.GetExitCodeProcess(self._handle)
                    else:
                        
                        rc = winprocess.GetLastError()
                        if rc:
                            raise WinError(rc)

                    self._cleanup()

                return self.returncode

            def _cleanup_job_io_port(self):
                """ Do the job and IO port cleanup separately because there are
                    cases where we want to clean these without killing _handle
                    (i.e. if we fail to create the job object in the first place)
                """
                if getattr(self, '_job') and self._job != winprocess.INVALID_HANDLE_VALUE:
                    self._job.Close()
                    self._job = None
                else:
                    
                    
                    self._job = None

                if getattr(self, '_io_port', None) and self._io_port != winprocess.INVALID_HANDLE_VALUE:
                    self._io_port.Close()
                    self._io_port = None
                else:
                    self._io_port = None

                if getattr(self, '_procmgrthread', None):
                    self._procmgrthread = None

            def _cleanup(self):
                self._cleanup_job_io_port()
                if self._thread and self._thread != winprocess.INVALID_HANDLE_VALUE:
                    self._thread.Close()
                    self._thread = None
                else:
                    self._thread = None

                if self._handle and self._handle != winprocess.INVALID_HANDLE_VALUE:
                    self._handle.Close()
                    self._handle = None
                else:
                    self._handle = None

        elif isPosix:

            def _wait(self):
                """ Haven't found any reason to differentiate between these platforms
                    so they all use the same wait callback.  If it is necessary to
                    craft different styles of wait, then a new _wait method
                    could be easily implemented.
                """

                if not self._ignore_children:
                    try:
                        
                        
                        
                        
                        
                        
                        
                        status = os.waitpid(self.pid, 0)[1]

                        
                        
                        if status > 255:
                            return status >> 8
                        return -status
                    except OSError, e:
                        if getattr(e, "errno", None) != 10:
                            
                            
                            print >> sys.stderr, "Encountered error waiting for pid to close: %s" % e
                            raise

                        return self.returncode

                else:
                    
                    subprocess.Popen.wait(self)
                    return self.returncode

            def _cleanup(self):
                pass

        else:
            
            print >> sys.stderr, "Unrecognized platform, process groups may not be managed properly"

            def _wait(self):
                self.returncode = subprocess.Popen.wait(self)
                return self.returncode

            def _cleanup(self):
                pass

    def __init__(self,
                 cmd,
                 args=None,
                 cwd=None,
                 env=None,
                 ignore_children = False,
                 kill_on_timeout = True,
                 processOutputLine=(),
                 onTimeout=(),
                 onFinish=(),
                 **kwargs):
        self.cmd = cmd
        self.args = args
        self.cwd = cwd
        self.didTimeout = False
        self._ignore_children = ignore_children
        self._kill_on_timeout = kill_on_timeout
        self.keywordargs = kwargs
        self.outThread = None
        self.read_buffer = ''

        if env is None:
            env = os.environ.copy()
        self.env = env

        
        if callable(processOutputLine):
            processOutputLine = [processOutputLine]
        self.processOutputLineHandlers = list(processOutputLine)
        if callable(onTimeout):
            onTimeout = [onTimeout]
        self.onTimeoutHandlers = list(onTimeout)
        if callable(onFinish):
            onFinish = [onFinish]
        self.onFinishHandlers = list(onFinish)

        
        
        if isinstance(self.cmd, list):
            if self.args != None:
                raise TypeError("cmd and args must not both be lists")
            (self.cmd, self.args) = (self.cmd[0], self.cmd[1:])
        elif self.args is None:
            self.args = []

    @property
    def timedOut(self):
        """True if the process has timed out."""
        return self.didTimeout

    @property
    def commandline(self):
        """the string value of the command line (command + args)"""
        return subprocess.list2cmdline([self.cmd] + self.args)

    def run(self, timeout=None, outputTimeout=None):
        """
        Starts the process.

        If timeout is not None, the process will be allowed to continue for
        that number of seconds before being killed. If the process is killed
        due to a timeout, the onTimeout handler will be called.

        If outputTimeout is not None, the process will be allowed to continue
        for that number of seconds without producing any output before
        being killed.
        """
        self.didTimeout = False
        self.startTime = datetime.now()

        
        args = dict(stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    cwd=self.cwd,
                    env=self.env,
                    ignore_children=self._ignore_children)

        
        args.update(self.keywordargs)

        
        self.proc = self.Process([self.cmd] + self.args, **args)

        self.processOutput(timeout=timeout, outputTimeout=outputTimeout)

    def kill(self, sig=None):
        """
        Kills the managed process.

        If you created the process with 'ignore_children=False' (the
        default) then it will also also kill all child processes spawned by
        it. If you specified 'ignore_children=True' when creating the
        process, only the root process will be killed.

        Note that this does not manage any state, save any output etc,
        it immediately kills the process.

        :param sig: Signal used to kill the process, defaults to SIGKILL
                    (has no effect on Windows)
        """
        try:
            self.proc.kill(sig=sig)

            
            
            
            return self.wait()
        except AttributeError:
            
            if not hasattr(self, 'proc'):
                print >> sys.stderr, "Unable to kill Process because call to ProcessHandler constructor failed."
            else:
                raise

    def readWithTimeout(self, f, timeout):
        """
        Try to read a line of output from the file object *f*.

        *f* must be a  pipe, like the *stdout* member of a subprocess.Popen
        object created with stdout=PIPE. If no output
        is received within *timeout* seconds, return a blank line.

        Returns a tuple (line, did_timeout), where *did_timeout* is True
        if the read timed out, and False otherwise.
        """
        
        
        return self._readWithTimeout(f, timeout)

    def processOutputLine(self, line):
        """Called for each line of output that a process sends to stdout/stderr."""
        for handler in self.processOutputLineHandlers:
            handler(line)

    def onTimeout(self):
        """Called when a process times out."""
        for handler in self.onTimeoutHandlers:
            handler()

    def onFinish(self):
        """Called when a process finishes without a timeout."""
        for handler in self.onFinishHandlers:
            handler()

    def poll(self):
        """Check if child process has terminated

        Returns the current returncode value:
        - None if the process hasn't terminated yet
        - A negative number if the process was killed by signal N (Unix only)
        - '0' if the process ended without failures

        """
        
        
        
        if self.outThread and self.outThread.isAlive():
            return None
        elif hasattr(self.proc, "returncode"):
            return self.proc.returncode
        else:
            return self.proc.poll()

    def processOutput(self, timeout=None, outputTimeout=None):
        """
        Handle process output until the process terminates or times out.

        If timeout is not None, the process will be allowed to continue for
        that number of seconds before being killed.

        If outputTimeout is not None, the process will be allowed to continue
        for that number of seconds without producing any output before
        being killed.
        """
        def _processOutput():
            self.didTimeout = False
            logsource = self.proc.stdout

            lineReadTimeout = None
            if timeout:
                lineReadTimeout = timeout - (datetime.now() - self.startTime).seconds
            elif outputTimeout:
                lineReadTimeout = outputTimeout

            (lines, self.didTimeout) = self.readWithTimeout(logsource, lineReadTimeout)
            while lines != "":
                for line in lines.splitlines():
                    self.processOutputLine(line.rstrip())

                if self.didTimeout:
                    break

                if timeout:
                    lineReadTimeout = timeout - (datetime.now() - self.startTime).seconds
                (lines, self.didTimeout) = self.readWithTimeout(logsource, lineReadTimeout)

            if self.didTimeout:
                if self._kill_on_timeout:
                    self.proc.kill()
                self.onTimeout()
            else:
                self.onFinish()

        if not hasattr(self, 'proc'):
            self.run()

        if not self.outThread:
            self.outThread = threading.Thread(target=_processOutput)
            self.outThread.daemon = True
            self.outThread.start()


    def wait(self, timeout=None):
        """
        Waits until all output has been read and the process is
        terminated.

        If timeout is not None, will return after timeout seconds.
        This timeout only causes the wait function to return and
        does not kill the process.

        Returns the process exit code value:
        - None if the process hasn't terminated yet
        - A negative number if the process was killed by signal N (Unix only)
        - '0' if the process ended without failures

        """
        if self.outThread:
            
            
            count = 0
            while self.outThread.isAlive():
                self.outThread.join(timeout=1)
                count += 1
                if timeout and count > timeout:
                    return None

        self.returncode = self.proc.wait()
        return self.returncode

    
    def waitForFinish(self, timeout=None):
        print >> sys.stderr, "MOZPROCESS WARNING: ProcessHandler.waitForFinish() is deprecated, " \
                             "use ProcessHandler.wait() instead"
        return self.wait(timeout=timeout)


    

    if isWin:
        
        PeekNamedPipe = ctypes.windll.kernel32.PeekNamedPipe
        GetLastError = ctypes.windll.kernel32.GetLastError

        def _readWithTimeout(self, f, timeout):
            if timeout is None:
                
                return (f.readline(), False)
            x = msvcrt.get_osfhandle(f.fileno())
            l = ctypes.c_long()
            done = time.time() + timeout
            while time.time() < done:
                if self.PeekNamedPipe(x, None, 0, None, ctypes.byref(l), None) == 0:
                    err = self.GetLastError()
                    if err == 38 or err == 109: 
                        return ('', False)
                    else:
                        raise OSError("readWithTimeout got error: %d", err)
                if l.value > 0:
                    
                    
                    return (f.readline(), False)
                time.sleep(0.01)
            return ('', True)

    else:
        
        def _readWithTimeout(self, f, timeout):
            while True:
                try:
                    (r, w, e) = select.select([f], [], [], timeout)
                except:
                    
                    return ('', True)

                if len(r) == 0:
                    return ('', True)

                output = os.read(f.fileno(), 4096)
                if not output:
                    output = self.read_buffer
                    self.read_buffer = ''
                    return (output, False)
                self.read_buffer += output
                if '\n' not in self.read_buffer:
                    time.sleep(0.01)
                    continue
                tmp = self.read_buffer.split('\n')
                lines, self.read_buffer = tmp[:-1], tmp[-1]
                real_lines = [x for x in lines if x != '']
                if not real_lines:
                    time.sleep(0.01)
                    continue
                break
            return ('\n'.join(lines), False)

    @property
    def pid(self):
        return self.proc.pid





class StoreOutput(object):
    """accumulate stdout"""

    def __init__(self):
        self.output = []

    def __call__(self, line):
        self.output.append(line)

class StreamOutput(object):
    """pass output to a stream and flush"""

    def __init__(self, stream):
        self.stream = stream

    def __call__(self, line):
        try:
            self.stream.write(line + '\n')
        except UnicodeDecodeError:
            
            
            self.stream.write(line.decode('iso8859-1') + '\n')
        self.stream.flush()

class LogOutput(StreamOutput):
    """pass output to a file"""

    def __init__(self, filename):
        self.file_obj = open(filename, 'a')
        StreamOutput.__init__(self, self.file_obj)

    def __del__(self):
        if self.file_obj is not None:
            self.file_obj.close()




class ProcessHandler(ProcessHandlerMixin):
    """
    Convenience class for handling processes with default output handlers.

    If no processOutputLine keyword argument is specified, write all
    output to stdout.  Otherwise, the function or the list of functions
    specified by this argument will be called for each line of output;
    the output will not be written to stdout automatically.

    If storeOutput==True, the output produced by the process will be saved
    as self.output.

    If logfile is not None, the output produced by the process will be
    appended to the given file.
    """

    def __init__(self, cmd, logfile=None, stream=None, storeOutput=True, **kwargs):
        kwargs.setdefault('processOutputLine', [])
        if callable(kwargs['processOutputLine']):
            kwargs['processOutputLine'] = [kwargs['processOutputLine']]

        if logfile:
            logoutput = LogOutput(logfile)
            kwargs['processOutputLine'].append(logoutput)

        if stream:
            streamoutput = StreamOutput(stream)
            kwargs['processOutputLine'].append(streamoutput)

        
        if not kwargs['processOutputLine']:
            kwargs['processOutputLine'].append(StreamOutput(sys.stdout))

        self.output = None
        if storeOutput:
            storeoutput = StoreOutput()
            self.output = storeoutput.output
            kwargs['processOutputLine'].append(storeoutput)

        ProcessHandlerMixin.__init__(self, cmd, **kwargs)
